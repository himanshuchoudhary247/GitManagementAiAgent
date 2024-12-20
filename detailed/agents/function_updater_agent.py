# agents/function_updater_agent.py

import os
import logging
import json
from utils.code_utils import find_function_definitions

class FunctionUpdaterAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def update_functions(self):
        actions = self.memory.get('interpreted_requirement', [])
        relevant_functions = self.memory.get('relevant_functions', [])
        for action in actions:
            if action.get('action_type').lower() == 'update':
                file_path = os.path.join(self.repo_path, action.get('file'))
                function_name = action.get('function')

                try:
                    if not os.path.exists(file_path):
                        logging.error(f"File '{file_path}' does not exist. Cannot update function '{function_name}'.")
                        print(f"Error: File '{file_path}' does not exist. Cannot update function '{function_name}'.")
                        continue

                    logging.info(f"Updating function '{function_name}' in file '{file_path}'.")
                    print(f"Updating function '{function_name}' in file '{file_path}'.")

                    with open(file_path, 'r') as f:
                        content = f.read()

                    # Find the function definition range
                    start_line, end_line = find_function_definitions(content, function_name)
                    if start_line is None or end_line is None:
                        logging.error(f"Function '{function_name}' not found in '{file_path}'.")
                        print(f"Error: Function '{function_name}' not found in '{file_path}'.")
                        continue

                    # Extract the existing function code
                    lines = content.split('\n')
                    old_function_code = '\n'.join(lines[start_line:end_line])

                    # Gather context from relevant functions
                    context = self.gather_context(relevant_functions, file_path)

                    # Generate the updated function code using Llama 3
                    prompt = (
                        f"Context:\n{context}\n\n"
                        f"Existing Function:\n{old_function_code}\n\n"
                        f"Requirement: Update the function `{function_name}` to include logging for authentication attempts."
                    )
                    updated_function_code = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

                    if not updated_function_code:
                        logging.error(f"Failed to generate updated code for function '{function_name}' in '{file_path}'.")
                        print(f"Error: Failed to generate updated code for function '{function_name}' in '{file_path}'.")
                        continue

                    # Replace the old function code with the updated code
                    updated_lines = lines[:start_line] + updated_function_code.split('\n') + lines[end_line:]
                    updated_content = '\n'.join(updated_lines)

                    # Write the updated content back to the file
                    with open(file_path, 'w') as f:
                        f.write(updated_content)

                    logging.info(f"Updated function '{function_name}' in '{file_path}'.")
                    print(f"Updated function '{function_name}' in '{file_path}'.")

                except PermissionError:
                    logging.error(f"Permission denied when trying to write to '{file_path}'.")
                    print(f"Error: Permission denied when trying to write to '{file_path}'. Please check your write permissions.")
                except Exception as e:
                    logging.error(f"Unexpected error while updating function '{function_name}' in '{file_path}': {e}")
                    print(f"Error: Unexpected error while updating function '{function_name}' in '{file_path}': {e}")

    def gather_context(self, relevant_functions, target_file_path):
        """
        Gathers relevant context from other functions to assist in updating the target function.
        """
        context = ""
        for func in relevant_functions:
            if func['file'] != os.path.relpath(target_file_path, self.repo_path):
                func_file_path = os.path.join(self.repo_path, func['file'])
                try:
                    with open(func_file_path, 'r') as f:
                        content = f.read()
                    start_line, end_line = find_function_definitions(content, func['function'])
                    if start_line is not None and end_line is not None:
                        function_code = '\n'.join(content.split('\n')[start_line:end_line])
                        context += f"\nFunction from {func['file']}:\n{function_code}\n"
                except Exception as e:
                    logging.error(f"Failed to gather context from '{func['file']}': {e}")
        return context
