# agents/undo_agent.py

import os
from utils.change_tracker import ChangeTracker
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker

class UndoAgent(BaseAgent):
    def __init__(self, repo_path, centralized_memory):
        super().__init__(llama3_client=None, centralized_memory=centralized_memory, name="UndoAgent")  # llama3_client not needed here
        self.repo_path = os.path.abspath(repo_path)
        self.change_tracker = ChangeTracker(self.repo_path)

    def execute(self):
        """
        Undoes all tracked changes by reverting files to their previous states.
        """
        changes = self.centralized_memory.get('UndoAgent', 'changes', [])
        if not changes:
            changes = self.change_tracker.get_changes()

        if not changes:
            self.log_info("No changes to undo.")
            print_with_breaker("No changes to undo.")
            return

        # Iterate in reverse order to undo the latest changes first
        for change in reversed(changes):
            file_path = os.path.join(self.repo_path, change['file'])
            action = change['action']
            content_before = change['content_before']

            try:
                if os.path.exists(file_path):
                    with open(file_path, 'w') as f:
                        f.write(content_before)
                    self.log_info(f"Reverted {action} on '{change['file']}'.")
                    print_with_breaker(f"Reverted {action} on '{change['file']}'.")
                else:
                    self.log_warning(f"File '{change['file']}' does not exist. Skipping undo.")
                    print_with_breaker(f"Warning: File '{change['file']}' does not exist. Skipping undo.")
            except Exception as e:
                self.log_error(f"Failed to undo change on '{change['file']}': {e}")
                print_with_breaker(f"Error: Failed to undo change on '{change['file']}'.")

        # Clear the change log after undoing
        self.centralized_memory.set('UndoAgent', 'changes', [])
        self.change_tracker.clear_change_log()
        self.log_info("All changes have been undone and change log cleared.")
        print_with_breaker("All changes have been undone and change log cleared.")
