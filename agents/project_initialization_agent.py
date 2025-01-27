# agents/project_initialization_agent.py

import os
import logging
from utils.cli_utils import print_with_breaker

class ProjectInitializationAgent:
    def __init__(self, repo_path):
        self.repo_path = os.path.abspath(repo_path)
        self.default_directory_structure = {
            "src": {
                "__init__.py": "",
                "main.py": "# Main application entry point\n"
            },
            "tests": {
                "__init__.py": "",
                "test_main.py": "# Tests for main application\n"
            },
            "docs": {
                "user_guide.md": "# User Guide\n\nRefer to the [User Guide](user_guide.md) for detailed instructions."
            },
            "configs": {},
            "backup": {},
            "integrity": {},
            "utils": {
                "cli_utils.py": "def print_with_breaker(message):\n    print('\\n' + '-'*50)\n    print(message)\n    print('-'*50 + '\\n')",
                "change_tracker.py": "",
                "memory_node.py": ""
            },
            "agents": {
                "code_writing_agent.py": "",
                "execution_agent.py": "",
                "undo_agent.py": "",
                "project_initialization_agent.py": "",
                "self_reflection_agent.py": ""
            }
        }

    def is_repo_initialized(self):
        # Check if the repo has any files excluding the change_log.json and README.md
        for root, dirs, files in os.walk(self.repo_path):
            for file in files:
                if file not in ['change_log.json', 'README.md', 'user_guide.md']:
                    return True
        return False

    def initialize_project(self):
        if self.is_repo_initialized():
            logging.info("Repository already has code. Skipping initialization.")
            print_with_breaker("Repository already has code. Skipping initialization.")
            return

        logging.info("Initializing new project with directory structure.")
        print_with_breaker("Initializing new project with directory structure.")

        self.create_directories(self.default_directory_structure, self.repo_path)

        logging.info("Project initialization completed.")
        print_with_breaker("Project initialization completed.")

    def create_directories(self, structure, current_path):
        for name, content in structure.items():
            path = os.path.join(current_path, name)
            if isinstance(content, dict):
                os.makedirs(path, exist_ok=True)
                self.create_directories(content, path)
            else:
                # content is the file content
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, 'w') as f:
                    f.write(content)
                logging.info(f"Created file '{path}'.")
                print_with_breaker(f"Created file '{path}'.")
