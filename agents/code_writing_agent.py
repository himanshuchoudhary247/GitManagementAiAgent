# agents/code_writing_agent.py

import os
import logging
import json
from utils.code_utils import find_function_definitions, extract_json_from_response

class CodeWritingAgent:
    def __init__(self, repo_path, memory, use_gitrepo=False):
        self.repo_path = repo_path
        self.memory = memory
        self.use_gitrepo = use_gitrepo

    def write_code_changes(self, code_changes):
        try:
            for change in code_changes:
                action = change.get('action')
                file = change.get('file')
                code = change.get('code')

                if not action or not file:
                    logging.warning(f"Incomplete code change information: {change}")
                    print(f"Warning: Incomplete code change information: {change}")
                    continue

                # Skip actions with empty 'code' fields
                if not code:
                    logging.warning(f"Empty code field for action '{action}' in file '{file}'. Skipping.")
                    print(f"Warning: Empty code field for action '{action}' in file '{file}'. Skipping.")
                    continue

                # Normalize action names if necessary
                if action.lower() in ['add_function', 'add']:
                    normalized_action = 'add'
                elif action.lower() in ['update', 'modify']:
                    normalized_action = 'update'
                else:
                    normalized_action = action.lower()

                if normalized_action == 'add':
                    self.add_function(file, code)
                elif normalized_action == 'update':
                    self.update_function(file, code)
                else:
                    logging.warning(f"Unknown action '{action}' in code changes.")
                    print(f"Warning: Unknown action '{action}' in code changes.")

            logging.info("All code changes have been written successfully.")
            print("All code changes have been written successfully.")

        except Exception as e:
            logging.error(f"Error in CodeWritingAgent: {e}")
            print(f"Error: Failed to write code changes: {e}")

    def add_function(self, file, code):
        try:
            file_path = os.path.join(self.repo_path, os.path.relpath(file, self.repo_path))
            # Ensure the directory exists
            os.makedirs(os.path.dirname(file_path), exist_ok=True)

            if os.path.exists(file_path):
                with open(file_path, 'a') as f:
                    f.write(f"\n\n{code}")
                logging.info(f"Appended new function to existing file '{file_path}'.")
                print(f"Appended new function to existing file '{file_path}'.")
            else:
                # If the file does not exist, skip adding the function
                logging.warning(f"File '{file_path}' does not exist. Skipping 'add' action.")
                print(f"Warning: File '{file_path}' does not exist. Skipping 'add' action.")

        except PermissionError:
            logging.error(f"Permission denied when trying to write to '{file_path}'.")
            print(f"Error: Permission denied when trying to write to '{file_path}'. Please check your write permissions.")
        except Exception as e:
            logging.error(f"Unexpected error while adding function to '{file_path}': {e}")
            print(f"Error: Unexpected error while adding function to '{file_path}': {e}")

    def update_function(self, file, code):
        try:
            file_path = os.path.join(self.repo_path, os.path.relpath(file, self.repo_path))
            if not os.path.exists(file_path):
                logging.error(f"File '{file_path}' does not exist. Cannot update function.")
                print(f"Error: File '{file_path}' does not exist. Cannot update function.")
                return

            with open(file_path, 'r') as f:
                content = f.read()

            # Extract function name from the new code
            function_name = self.extract_function_name(code)
            if not function_name:
                logging.error(f"Failed to extract function name from the provided code.")
                print(f"Error: Failed to extract function name from the provided code.")
                return

            # Find existing function definition
            start_line, end_line = find_function_definitions(content, function_name)
            if start_line is None or end_line is None:
                logging.error(f"Function '{function_name}' not found in '{file_path}'.")
                print(f"Error: Function '{function_name}' not found in '{file_path}'.")
                return

            # Replace the old function code with the new code
            lines = content.split('\n')
            updated_lines = lines[:start_line] + code.split('\n') + lines[end_line:]
            updated_content = '\n'.join(updated_lines)

            with open(file_path, 'w') as f:
                f.write(updated_content)

            logging.info(f"Updated function '{function_name}' in '{file_path}'.")
            print(f"Updated function '{function_name}' in '{file_path}'.")

        except PermissionError:
            logging.error(f"Permission denied when trying to write to '{file_path}'.")
            print(f"Error: Permission denied when trying to write to '{file_path}'. Please check your write permissions.")
        except Exception as e:
            logging.error(f"Unexpected error while updating function in '{file_path}': {e}")
            print(f"Error: Unexpected error while updating function in '{file_path}': {e}")

    def extract_function_name(self, code):
        """
        Extracts the function name from the function definition.
        Assumes the function definition starts with 'def'.
        """
        try:
            lines = code.split('\n')
            for line in lines:
                if line.strip().startswith('def '):
                    return line.strip().split('def ')[1].split('(')[0]
            return None
        except Exception as e:
            logging.error(f"Error extracting function name: {e}")
            return None

    def parse_response(self, response):
        """
        Not used in CodeWritingAgent but kept for consistency.
        """
        try:
            data = extract_json_from_response(response)
            code_changes = data.get('code_changes', [])
            return code_changes
        except Exception as e:
            logging.error(f"Error parsing response in CodeWritingAgent: {e}")
            print(f"Error: Failed to parse code changes: {e}")
            return []
