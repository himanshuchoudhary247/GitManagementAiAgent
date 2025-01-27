# agents/code_completer_agent.py

import os
from utils.code_utils import find_function_definitions, is_python_file
from utils.change_tracker import ChangeTracker
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import code_completion_prompt

class CodeCompleterAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory, repo_path):
        super().__init__(llama3_client, centralized_memory, name="CodeCompleterAgent")
        self.repo_path = os.path.abspath(repo_path)
        self.max_retries = 3  # Set maximum retries

    def execute(self):
        """
        Completes incomplete functions identified by CodeValidationAgent.
        Implements a retry mechanism to attempt completion up to a threshold.
        """
        incomplete_functions = self.centralized_memory.get('CodeValidationAgent', 'incomplete_functions', [])
        if not incomplete_functions:
            self.log_info("No incomplete functions to complete.")
            print_with_breaker("No incomplete functions to complete.")
            return

        completed_functions = []
        retry_count = 0

        while incomplete_functions and retry_count < self.max_retries:
            self.log_info(f"Attempt {retry_count + 1} to complete incomplete functions.")
            print_with_breaker(f"Attempt {retry_count + 1} to complete incomplete functions.")

            for func in incomplete_functions:
                file = func['file']
                function_name = func['function']
                try:
                    # Generate prompt based on function name and file context
                    prompt = self.build_completion_prompt(file, function_name)
                    self.log_info(f"Generating completion for function '{function_name}' in '{file}'.")
                    print_with_breaker(f"Generating completion for function '{function_name}' in '{file}'.")
                    response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.7)
                    if not response:
                        self.log_warning(f"Empty response while completing function '{function_name}' in '{file}'.")
                        print_with_breaker(f"Warning: Empty response while completing function '{function_name}' in '{file}'.")
                        continue
                    completed_code = self.parse_completion(response)
                    if completed_code:
                        # Apply the completed code to the file
                        self.apply_completion(file, function_name, completed_code)
                        completed_functions.append({
                            'file': file,
                            'function': function_name,
                            'code': completed_code
                        })
                    else:
                        self.log_warning(f"Failed to parse completion for function '{function_name}' in '{file}'.")
                        print_with_breaker(f"Warning: Failed to parse completion for function '{function_name}' in '{file}'.")
                except Exception as e:
                    self.log_error(f"Error completing function '{function_name}' in '{file}': {e}")
                    print_with_breaker(f"Error: {e}")

            retry_count += 1
            # Re-validate code completeness after attempting completions
            self.centralized_memory.set('CodeCompleterAgent', 'completed_functions', completed_functions)
            self.centralized_memory.save_memory()
            self.code_validation_agent.execute()
            incomplete_functions = self.centralized_memory.get('CodeValidationAgent', 'incomplete_functions', [])

        if completed_functions:
            self.log_info(f"Completed {len(completed_functions)} functions.")
            print_with_breaker(f"Completed {len(completed_functions)} functions.")
        else:
            self.log_warning("No functions were completed.")
            print_with_breaker("Warning: No functions were completed.")

    def build_completion_prompt(self, file, function_name):
        """
        Constructs a prompt to generate the complete function implementation.

        :param file: Relative file path.
        :param function_name: Name of the incomplete function.
        :return: Prompt string.
        """
        return code_completion_prompt(file, function_name)

    def parse_completion(self, response):
        """
        Parses the LLM's response to extract the completed function code.

        :param response: Raw response from the LLM.
        :return: Completed function code as a string.
        """
        # Assuming the LLM returns the complete function code
        return response.strip()

    def apply_completion(self, file, function_name, completed_code):
        """
        Writes the completed function code to the specified file.

        :param file: Relative file path.
        :param function_name: Name of the function to complete.
        :param completed_code: Complete function code.
        """
        try:
            file_path = os.path.join(self.repo_path, file)
            with open(file_path, 'r') as f:
                lines = f.readlines()

            # Find the function definition line
            func_def_line = None
            for idx, line in enumerate(lines):
                if line.strip().startswith(f"def {function_name}("):
                    func_def_line = idx
                    break

            if func_def_line is not None:
                # Replace the incomplete function with the completed code
                # Find the end of the function
                end_line = func_def_line + 1
                while end_line < len(lines) and not lines[end_line].strip().startswith("def "):
                    end_line += 1
                # Replace lines from func_def_line to end_line with completed_code
                completed_code_lines = completed_code.split('\n')
                # Indent the completed code properly
                completed_code_lines = [line + '\n' for line in completed_code_lines]
                lines = lines[:func_def_line] + completed_code_lines + lines[end_line:]
                # Write back to the file
                with open(file_path, 'w') as f:
                    f.writelines(lines)
                self.log_info(f"Completed function '{function_name}' in '{file}'.")
                print_with_breaker(f"Completed function '{function_name}' in '{file}'.")
            else:
                self.log_warning(f"Function '{function_name}' not found in '{file}'. Skipping completion.")
                print_with_breaker(f"Warning: Function '{function_name}' not found in '{file}'. Skipping completion.")
        except Exception as e:
            self.log_error(f"Failed to apply completion for function '{function_name}' in '{file}': {e}")
            print_with_breaker(f"Error: Failed to apply completion for function '{function_name}' in '{file}': {e}")