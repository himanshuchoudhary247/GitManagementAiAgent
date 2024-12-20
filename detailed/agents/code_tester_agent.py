# agents/code_tester_agent.py

import os
import logging
import subprocess

class CodeTesterAgent:
    def __init__(self, repo_path, memory):
        self.repo_path = repo_path
        self.memory = memory

    def run_tests(self):
        try:
            logging.info("Starting test execution using pytest.")
            print("Starting test execution using pytest.")

            result = subprocess.run(['pytest', self.repo_path], capture_output=True, text=True)

            if result.returncode == 0:
                logging.info("All tests passed successfully.")
                print("All tests passed successfully.")
                self.memory.set('test_results', 'All tests passed successfully.')
            else:
                logging.error("Some tests failed.")
                print("Error: Some tests failed.")
                self.memory.set('test_results', result.stdout + "\n" + result.stderr)

        except FileNotFoundError:
            logging.error("pytest is not installed or not found in PATH.")
            print("Error: pytest is not installed or not found in PATH.")
        except Exception as e:
            logging.error(f"Unexpected error during test execution: {e}")
            print(f"Error: Unexpected error during test execution: {e}")
