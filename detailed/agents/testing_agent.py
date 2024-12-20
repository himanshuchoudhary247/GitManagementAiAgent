# agents/testing_agent.py

import os
import logging
import json
from utils.code_utils import find_function_definitions

class TestingAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def add_tests(self):
        actions = self.memory.get('interpreted_requirement', [])
        relevant_functions = self.memory.get('relevant_functions', [])
        for action in actions:
            if action.get('action_type').lower() == 'add' or action.get('action_type').lower() == 'update':
                file_path = os.path.join(self.repo_path, action.get('file'))
                function_name = action.get('function')

                try:
                    test_file_path = self.get_test_file_path(file_path)
                    os.makedirs(os.path.dirname(test_file_path), exist_ok=True)

                    if os.path.exists(test_file_path):
                        logging.info(f"Updating existing test file: {test_file_path}")
                        print(f"Updating existing test file: {test_file_path}")
                        with open(test_file_path, 'r') as f:
                            content = f.read()
                    else:
                        logging.info(f"Creating new test file: {test_file_path}")
                        print(f"Creating new test file: {test_file_path}")
                        content = ""

                    # Generate the test function using Llama 3
                    prompt = (
                        f"Existing Code:\n{self.get_function_code(file_path, function_name)}\n\n"
                        f"Requirement: Write a unit test for the function `{function_name}` using pytest."
                    )
                    test_function_code = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

                    if not test_function_code:
                        logging.error(f"Failed to generate test for function '{function_name}' in '{test_file_path}'.")
                        print(f"Error: Failed to generate test for function '{function_name}' in '{test_file_path}'.")
                        continue

                    # Append the test function to the test file
                    updated_content = content + "\n\n" + test_function_code

                    # Write the updated content back to the test file
                    with open(test_file_path, 'w') as f:
                        f.write(updated_content)

                    logging.info(f"Added test for function '{function_name}' to '{test_file_path}'.")
                    print(f"Added test for function '{function_name}' to '{test_file_path}'.")

                except PermissionError:
                    logging.error(f"Permission denied when trying to write to '{test_file_path}'.")
                    print(f"Error: Permission denied when trying to write to '{test_file_path}'. Please check your write permissions.")
                except Exception as e:
                    logging.error(f"Unexpected error while adding test for function '{function_name}' to '{test_file_path}': {e}")
                    print(f"Error: Unexpected error while adding test for function '{function_name}' to '{test_file_path}': {e}")

    def get_test_file_path(self, file_path):
        """
        Converts a source file path to its corresponding test file path.
        Example: src/auth/login.py -> tests/auth/test_login.py
        """
        parts = file_path.split(os.sep)
        if parts[0] == 'src':
            parts[0] = 'tests'
        else:
            parts.insert(0, 'tests')
        filename = parts[-1]
        test_filename = f"test_{filename}"
        parts[-1] = test_filename
        return os.path.join(*parts)

    def get_function_code(self, file_path, function_name):
        """
        Retrieves the code of the specified function from the given file.
        """
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            start_line, end_line = find_function_definitions(content, function_name)
            if start_line is None or end_line is None:
                logging.error(f"Function '{function_name}' not found in '{file_path}'.")
                return ""
            lines = content.split('\n')
            function_code = '\n'.join(lines[start_line:end_line])
            return function_code
        except Exception as e:
            logging.error(f"Failed to retrieve function code from '{file_path}': {e}")
            return ""
