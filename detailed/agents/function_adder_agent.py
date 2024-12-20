# agents/function_adder_agent.py

import os
import logging
import json
from utils.code_utils import find_function_definitions

class FunctionAdderAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def add_functions(self):
        actions = self.memory.get('interpreted_requirement', [])
        relevant_functions = self.memory.get('relevant_functions', [])
        for action in actions:
            if action.get('action_type').lower() == 'add':
                file_path = os.path.join(self.repo_path, action.get('file'))
                function_name = action.get('function')

                try:
                    # Ensure the directory exists
                    os.makedirs(os.path.dirname(file_path), exist_ok=True)

                    # Check if file exists
                    if os.path.exists(file_path):
                        logging.info(f"Updating existing file: {file_path}")
                        print(f"Updating existing file: {file_path}")
                        with open(file_path, 'r') as f:
                            content = f.read()
                    else:
                        logging.info(f"Creating new file: {file_path}")
                        print(f"Creating new file: {file_path}")
                        content = ""

                    # Gather context from relevant functions
                    context = self.gather_context(relevant_functions, file_path)

                    # Generate the new function code using Llama 3
                    prompt = (
                        f"Context:\n{context}\n\n"
                        f"Existing Code:\n{content}\n\n"
                        f"Requirement: Write a Python function named `{function_name}` that handles OAuth2 authentication."
                    )
                    new_function_code = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

                    if not new_function_code:
                        logging.error(f"Failed to generate code for function '{function_name}' in '{file_path}'.")
                        print(f"Error: Failed to generate code for function '{function_name}' in '{file_path}'.")
                        continue

                    # Append the new function to the file content
                    updated_content = content + "\n\n" + new_function_code

                    # Write the updated content back to the file
                    with open(file_path, 'w') as f:
                        f.write(updated_content)

                    logging.info(f"Added new function '{function_name}' to '{file_path}'.")
                    print(f"Added new function '{function_name}' to '{file_path}'.")

                except PermissionError:
                    logging.error(f"Permission denied when trying to write to '{file_path}'.")
                    print(f"Error: Permission denied when trying to write to '{file_path}'. Please check your write permissions.")
                except Exception as e:
                    logging.error(f"Unexpected error while adding function '{function_name}' to '{file_path}': {e}")
                    print(f"Error: Unexpected error while adding function '{function_name}' to '{file_path}': {e}")

    def gather_context(self, relevant_functions, target_file_path):
        """
        Gathers relevant context from other functions to assist in adding the new function.
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
