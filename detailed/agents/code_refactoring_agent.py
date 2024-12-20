# agents/code_refactoring_agent.py

import os
import logging
import json
from utils.code_utils import find_function_definitions

class CodeRefactoringAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def refactor_code(self):
        try:
            # Retrieve all functions from memory
            repo_map = self.memory.get('repo_map', {})
            if not repo_map:
                logging.info("No repository map available for refactoring.")
                print("No repository map available for refactoring.")
                return

            for file, functions in repo_map.items():
                file_path = os.path.join(self.repo_path, file)
                for function in functions:
                    self.refactor_function(file_path, function)

        except Exception as e:
            logging.error(f"Error in code refactoring: {e}")
            print(f"Error: Failed to refactor code: {e}")

    def refactor_function(self, file_path, function_name):
        try:
            with open(file_path, 'r') as f:
                content = f.read()

            # Extract function code
            lines = content.split('\n')
            start_line, end_line = self.find_function_definitions(content, function_name)
            if start_line is None or end_line is None:
                logging.warning(f"Function '{function_name}' not found in '{file_path}'. Skipping refactoring.")
                return

            function_code = '\n'.join(lines[start_line:end_line])

            # Construct the prompt for refactoring
            prompt = (
                f"Original Function:\n{function_code}\n\n"
                f"Requirement: Refactor the above function to improve its readability and performance without changing its functionality. "
                f"Ensure that the refactored code follows PEP 8 standards."
            )
            logging.debug(f"Refactoring prompt for '{function_name}': {prompt}")

            # Generate the refactored code using Llama 3
            refactored_code = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)

            if not refactored_code:
                logging.error(f"Failed to refactor function '{function_name}' in '{file_path}'.")
                print(f"Error: Failed to refactor function '{function_name}' in '{file_path}'.")
                return

            # Replace the old function code with the refactored code
            updated_lines = lines[:start_line] + refactored_code.split('\n') + lines[end_line:]
            updated_content = '\n'.join(updated_lines)

            # Write the updated content back to the file
            with open(file_path, 'w') as f:
                f.write(updated_content)

            logging.info(f"Refactored function '{function_name}' in '{file_path}'.")
            print(f"Refactored function '{function_name}' in '{file_path}'.")

        except Exception as e:
            logging.error(f"Unexpected error while refactoring function '{function_name}' in '{file_path}': {e}")
            print(f"Error: Unexpected error while refactoring function '{function_name}' in '{file_path}': {e}")

    def find_function_definitions(self, content, function_name):
        """
        Finds the start and end line numbers of a function definition in the given content.
        Returns a tuple (start_line, end_line). If not found, returns (None, None).
        """
        lines = content.split('\n')
        start_line = None
        end_line = None
        for i, line in enumerate(lines):
            if line.strip().startswith(f"def {function_name}("):
                start_line = i
                # Find the end of the function by looking for the next function or end of file
                for j in range(i + 1, len(lines)):
                    if lines[j].strip().startswith("def "):
                        end_line = j
                        break
                if end_line is None:
                    end_line = len(lines)
                break
        return start_line, end_line
