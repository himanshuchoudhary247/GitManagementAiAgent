# agents/code_formatter_agent.py

import os
import logging
import subprocess

class CodeFormatterAgent:
    def __init__(self, repo_path, memory):
        self.repo_path = repo_path
        self.memory = memory

    def format_code(self):
        try:
            # Use Black for code formatting
            logging.info("Starting code formatting using Black.")
            print("Starting code formatting using Black.")

            subprocess.check_call(['black', self.repo_path])

            logging.info("Code formatting completed successfully.")
            print("Code formatting completed successfully.")

        except subprocess.CalledProcessError as e:
            logging.error(f"Black formatting failed: {e}")
            print(f"Error: Code formatting failed: {e}")
        except Exception as e:
            logging.error(f"Unexpected error during code formatting: {e}")
            print(f"Error: Unexpected error during code formatting: {e}")
