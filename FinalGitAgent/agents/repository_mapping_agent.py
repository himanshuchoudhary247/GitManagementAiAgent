# agents/repository_mapping_agent.py

import os
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker

class RepositoryMappingAgent(BaseAgent):
    def __init__(self, centralized_memory, repo_path):
        super().__init__(llama3_client=None, centralized_memory=centralized_memory, name="RepositoryMappingAgent")
        self.repo_path = os.path.abspath(repo_path)

    def execute(self):
        """
        Maps the repository structure and stores it in centralized memory.
        """
        repository_map = {}
        try:
            for root, dirs, files in os.walk(self.repo_path):
                # Exclude hidden directories and system directories if necessary
                dirs[:] = [d for d in dirs if not d.startswith('.')]
                relative_root = os.path.relpath(root, self.repo_path)
                repository_map[relative_root] = files
            self.centralized_memory.set('RepositoryMappingAgent', 'repository_map', repository_map)
            self.log_info("Repository mapping completed successfully.")
            print_with_breaker("Repository mapping completed successfully.")
        except Exception as e:
            self.log_error(f"Error during repository mapping: {e}")
            print_with_breaker(f"Error: {e}")
