# agents/code_validation_agent.py

import ast
import logging
import os
from utils.base_agent import BaseAgent

class CodeValidationAgent(BaseAgent):
    def __init__(self, memory, repo_path):
        super().__init__(name="CodeValidationAgent")  # No llama3_client needed
        self.memory = memory
        self.repo_path = os.path.abspath(repo_path)
        self.max_retries = 2

    def execute(self):
        """
        Scans the repository for incomplete functions and logs them.
        """
        incomplete_functions = []
        try:
            for root, dirs, files in os.walk(self.repo_path):
                for file in files:
                    if file.endswith('.py'):
                        file_path = os.path.join(root, file)
                        incomplete = self.find_incomplete_functions(file_path)
                        if incomplete:
                            incomplete_functions.extend(incomplete)
            if incomplete_functions:
                self.memory.set('incomplete_functions', incomplete_functions)
                self.log_info(f"Found {len(incomplete_functions)} incomplete functions.")
            else:
                self.log_info("No incomplete functions found.")
        except Exception as e:
            self.log_error(f"Error during code validation: {e}")

    def find_incomplete_functions(self, file_path):
        """
        Parses a Python file to find incomplete functions.
        
        :param file_path: Path to the Python file.
        :return: List of incomplete function details.
        """
        incomplete = []
        try:
            with open(file_path, 'r') as f:
                file_content = f.read()
            tree = ast.parse(file_content, filename=file_path)
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef):
                    # Check if the function has a body other than a docstring or pass
                    if not self.is_function_complete(node):
                        func_name = node.name
                        relative_path = os.path.relpath(file_path, self.repo_path)
                        incomplete.append({
                            'file': relative_path,
                            'function': func_name
                        })
        except Exception as e:
            self.log_warning(f"Failed to parse '{file_path}': {e}")
        return incomplete

    def is_function_complete(self, node):
        """
        Determines if a function has a complete implementation.
        
        :param node: AST FunctionDef node.
        :return: True if complete, False otherwise.
        """
        if len(node.body) == 0:
            return False
        for stmt in node.body:
            if isinstance(stmt, ast.Pass):
                return False
            if isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Str):
                # Function has only a docstring
                continue
            return True
        return False
