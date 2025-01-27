# agents/code_writing_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.code_utils import is_python_file
import os
import shutil
from agents.execution_agent import ExecutionAgent
from agents.error_correction_agent import ErrorCorrectionAgent  # Added import

class CodeWritingAgent:
    def __init__(self, repo_path: str, memory, error_correction_agent: ErrorCorrectionAgent, use_gitrepo: bool = False):
        """
        Initializes the CodeWritingAgent.

        Parameters:
        - repo_path (str): The path to the repository.
        - memory: The memory node associated with this agent.
        - error_correction_agent (ErrorCorrectionAgent): The agent responsible for correcting code errors.
        - use_gitrepo (bool): Flag to determine if Git repository operations should be used.
        """
        self.repo_path = os.path.abspath(repo_path)
        self.memory = memory
        self.error_correction_agent = error_correction_agent
        self.use_gitrepo = use_gitrepo
        self.execution_agent = ExecutionAgent(error_correction_agent)  # Passed error_correction_agent

    def write_code_changes(self, code_changes: list):
        """
        Writes code changes to the repository based on the provided list.

        Parameters:
        - code_changes (list): A list of code changes to apply.
        """
        try:
            for change in code_changes:
                action = change.get('action')
                file = change.get('file')
                code = change.get('code')

                if action in ['create', 'add']:
                    file_path = os.path.join(self.repo_path, file)
                    directory = os.path.dirname(file_path)
                    if not os.path.exists(directory):
                        os.makedirs(directory, exist_ok=True)
                        logging.info(f"Created directory: {directory}")
                        print_with_breaker(f"Created directory: {directory}")

                    with open(file_path, 'w') as f:
                        f.write(code)
                    logging.info(f"Added file: {file_path}")
                    print_with_breaker(f"Added file: {file_path}")

                elif action == 'modify':
                    file_path = os.path.join(self.repo_path, file)
                    if os.path.exists(file_path):
                        with open(file_path, 'a') as f:
                            f.write(code)
                        logging.info(f"Modified file: {file_path}")
                        print_with_breaker(f"Modified file: {file_path}")
                    else:
                        logging.error(f"File not found for modification: {file_path}")
                        print_with_breaker(f"Error: File not found for modification: {file_path}")

                elif action == 'move':
                    source = change.get('source')
                    destination = change.get('destination')
                    if not source or not destination:
                        logging.error("Move action requires 'source' and 'destination' fields.")
                        print_with_breaker("Error: Move action requires 'source' and 'destination' fields.")
                        continue

                    source_path = os.path.join(self.repo_path, source)
                    destination_dir = os.path.join(self.repo_path, destination)
                    destination_path = os.path.join(destination_dir, os.path.basename(source_path))

                    if os.path.exists(source_path):
                        if not os.path.exists(destination_dir):
                            os.makedirs(destination_dir, exist_ok=True)
                            logging.info(f"Created destination directory: {destination_dir}")
                            print_with_breaker(f"Created destination directory: {destination_dir}")

                        shutil.move(source_path, destination_path)
                        logging.info(f"Moved file from {source_path} to {destination_path}")
                        print_with_breaker(f"Moved file from {source_path} to {destination_path}")
                    else:
                        logging.error(f"Source file not found for moving: {source_path}")
                        print_with_breaker(f"Error: Source file not found for moving: {source_path}")

                else:
                    logging.warning(f"Unknown action: {action}")
                    print_with_breaker(f"Warning: Unknown action: {action}")

        except Exception as e:
            logging.error(f"Error writing code changes: {e}")
            print_with_breaker(f"Error: Failed to write code changes: {e}")
