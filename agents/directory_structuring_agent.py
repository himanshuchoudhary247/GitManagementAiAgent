# agents/directory_structuring_agent.py

import logging
import os
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response  # Removed correct_json import

class DirectoryStructuringAgent:
    def __init__(self, llama3_client: Llama3Client, repo_path: str):
        self.llama3_client = llama3_client
        self.repo_path = os.path.abspath(repo_path)

    def analyze_codebase(self) -> dict:
        """
        Parses the repository to understand the current structure and modules.
        """
        try:
            files_structure = {}
            for root, dirs, files in os.walk(self.repo_path):
                relative_path = os.path.relpath(root, self.repo_path)
                files_structure[relative_path] = files
            logging.debug(f"Current Files Structure: {files_structure}")
            return files_structure
        except Exception as e:
            logging.error(f"Error analyzing codebase: {e}")
            print_with_breaker(f"Error: Failed to analyze codebase: {e}")
            return {}

    def design_directory_structure(self, user_query: str) -> dict:
        """
        Designs a new directory structure based on the user query and current codebase.
        """
        try:
            files_structure = self.analyze_codebase()
            prompt = (
                f"User Query: {user_query}\n"
                f"Current Files Structure: {files_structure}\n\n"
                "Based on the above query and existing files, design an optimized directory structure for the repository. "
                "Provide the structure in JSON format with directories as keys and lists of files as values. "
                "Ensure that the structure follows best practices for maintainability and scalability. "
                "No extra text."
            )
            logging.debug(f"DirectoryStructuringAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)
            logging.debug(f"DirectoryStructuringAgent Response: {response}")

            structure_json = extract_json_from_response(response)
            if structure_json and isinstance(structure_json, dict):
                logging.info(f"Proposed Directory Structure: {structure_json}")
                print_with_breaker(f"Proposed Directory Structure: {structure_json}")
                return structure_json
            else:
                logging.warning("Failed to extract directory structure from the response.")
                print_with_breaker("Warning: Failed to extract directory structure from the response.")
                return {}
        except Exception as e:
            logging.error(f"Error designing directory structure: {e}")
            print_with_breaker(f"Error: Failed to design directory structure: {e}")
            return {}

    def propose_restructuring(self, user_query: str) -> dict:
        """
        Main method to propose a new directory structure.
        """
        return self.design_directory_structure(user_query)
