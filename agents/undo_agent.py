# agents/undo_agent.py

import json
import os
from datetime import datetime
import logging
from utils.cli_utils import print_with_breaker

class UndoAgent:
    def __init__(self, repo_path: str):
        self.repo_path = os.path.abspath(repo_path)
        self.log_file = os.path.join(self.repo_path, 'change_log.json')
        self.ensure_log_file()

    def ensure_log_file(self):
        if not os.path.exists(self.log_file):
            with open(self.log_file, 'w') as f:
                json.dump([], f, indent=4)

    def undo_changes(self):
        """
        Reverts the last set of changes made to the codebase.
        """
        try:
            with open(self.log_file, 'r') as f:
                changes = json.load(f)
            
            if not changes:
                print("No changes to undo.")
                logging.info("No changes to undo.")
                return
            
            # Revert the last change entry
            last_change = changes.pop()
            action = last_change.get('action')
            file = last_change.get('file')
            content_before = last_change.get('content_before')
            
            if action and file:
                file_path = os.path.join(self.repo_path, file)
                if content_before is not None:
                    with open(file_path, 'w') as f:
                        f.write(content_before)
                    print(f"Reverted changes in {file}.")
                    logging.info(f"Reverted changes in {file}.")
                else:
                    # Handle file deletion or movement if needed
                    pass  # Implement as per system requirements
            
            # Update the log file
            with open(self.log_file, 'w') as f:
                json.dump(changes, f, indent=4)
            
            print("Undo completed successfully.")
            logging.info("Undo completed successfully.")
        
        except Exception as e:
            logging.error(f"Failed to undo changes: {e}")
            print(f"Error: Failed to undo changes: {e}")
