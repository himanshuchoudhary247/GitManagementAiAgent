# agents/repo_mapper_agent.py

import os
import ast
import logging

class RepoMapperAgent:
    def __init__(self, repo_path, memory):
        self.repo_path = repo_path
        self.memory = memory

    def map_repo(self):
        repo_map = {}
        for root, dirs, files in os.walk(self.repo_path):
            for file in files:
                if file.endswith('.py'):
                    file_path = os.path.join(root, file)
                    try:
                        with open(file_path, 'r') as f:
                            tree = ast.parse(f.read(), filename=file_path)
                        functions = [node.name for node in ast.walk(tree) if isinstance(node, ast.FunctionDef)]
                        relative_path = os.path.relpath(file_path, self.repo_path)
                        repo_map[relative_path] = functions
                        logging.info(f"Mapped {len(functions)} functions in '{relative_path}'.")
                    except Exception as e:
                        logging.error(f"Failed to parse '{file_path}': {e}")
        self.memory.set('repo_map', repo_map)
        logging.info("Repository mapping completed.")
        print("Repository mapping completed.")

    def get_repo_map(self):
        return self.memory.get('repo_map', {})
