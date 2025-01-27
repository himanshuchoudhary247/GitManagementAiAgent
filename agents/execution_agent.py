# agents/execution_agent.py

import os
import subprocess
import logging
from utils.cli_utils import print_with_breaker
from agents.error_correction_agent import ErrorCorrectionAgent  # New Import

class ExecutionAgent:
    def __init__(self, error_correction_agent: ErrorCorrectionAgent):
        self.error_correction_agent = error_correction_agent

    def execute_code(self, code: str, explanation: str = ""):
        """
        Executes the provided code after user confirmation.

        Parameters:
        - code (str): The code snippet to execute.
        - explanation (str): Explanation of what the code does.
        """
        try:
            print_with_breaker("--------------------------------------------------")
            print_with_breaker("Execution Request")
            print_with_breaker(f"Code to Execute:\n{code}")
            print_with_breaker(f"Explanation:\n{explanation}")
            print_with_breaker("--------------------------------------------------\n")

            confirmation = input("Do you want to execute this code? (yes/no): ").strip().lower()
            if confirmation != 'yes':
                print_with_breaker("Execution cancelled by user.")
                logging.info("Code execution cancelled by user.")
                return

            # Execute the code safely
            result = subprocess.run(['python', '-c', code], capture_output=True, text=True, check=True)
            print_with_breaker(f"Execution Output:\n{result.stdout}")
            logging.info(f"Code executed successfully. Output: {result.stdout}")

        except subprocess.CalledProcessError as e:
            print_with_breaker(f"Error during code execution: {e.stderr}")
            logging.error(f"Error during code execution: {e.stderr}")

            # Check if it's a SyntaxError
            if 'SyntaxError' in e.stderr:
                logging.info("Detected SyntaxError. Invoking ErrorCorrectionAgent.")
                corrected_code = self.error_correction_agent.correct_code(code, e.stderr)
                if corrected_code:
                    print_with_breaker("Attempting to execute corrected code...")
                    logging.info("Attempting to execute corrected code.")
                    self.execute_code(corrected_code, explanation="Retrying with corrected code.")
                else:
                    print_with_breaker("Error: Failed to correct the code.")
                    logging.error("Failed to correct the code.")

        except FileNotFoundError as e:
            print_with_breaker(f"File Not Found Error during code execution: {e}")
            logging.error(f"File Not Found Error during code execution: {e}")

        except Exception as e:
            print_with_breaker(f"Unexpected error during code execution: {e}")
            logging.error(f"Unexpected error during code execution: {e}")
