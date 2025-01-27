# agents/code_completer_agent.py

import os
from utils.base_agent import BaseAgent
from utils.code_utils import extract_json_from_response, correct_json
import logging

class CodeCompleterAgent(BaseAgent):
    def __init__(self, llama3_client, memory, repo_path):
        super().__init__(llama3_client, memory, name="CodeCompleterAgent")
        self.repo_path = os.path.abspath(repo_path)
        self.max_retries = 3

    def execute(self):
        """
        Completes incomplete functions identified by CodeValidationAgent.
        """
        incomplete_functions = self.memory.get('incomplete_functions', [])
        if not incomplete_functions:
            self.log_info("No incomplete functions to complete.")
            return

        completed_functions = []

        for func in incomplete_functions:
            file = func['file']
            function_name = func['function']
            try:
                # Generate prompt based on function name and file context
                prompt = self.build_completion_prompt(file, function_name)
                self.log_info(f"Generating completion for function '{function_name}' in '{file}'.")
                response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.7)
                if not response:
                    self.log_warning(f"Empty response while completing function '{function_name}' in '{file}'.")
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
            except Exception as e:
                self.log_error(f"Error completing function '{function_name}' in '{file}': {e}")

        if completed_functions:
            self.memory.set('completed_functions', completed_functions)
            self.log_info(f"Completed {len(completed_functions)} functions.")
        else:
            self.log_warning("No functions were completed.")

    def build_completion_prompt(self, file, function_name):
        """
        Constructs a prompt to generate the complete function implementation.
        
        :param file: Relative file path.
        :param function_name: Name of the incomplete function.
        :return: Prompt string.
        """
        prompt = (
            f"File: {file}\n"
            f"Function Name: {function_name}\n\n"
            "The above function is incomplete. Please provide a complete implementation for it. "
            "Include proper error handling, adhere to best coding practices, and ensure it performs a meaningful task."
        )
        return prompt

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
            else:
                self.log_warning(f"Function '{function_name}' not found in '{file}'. Skipping completion.")
        except Exception as e:
            self.log_error(f"Failed to apply completion for function '{function_name}' in '{file}': {e}")
