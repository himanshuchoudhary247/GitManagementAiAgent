# agents/error_correction_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class ErrorCorrectionAgent:
    def __init__(self, llama3_client: Llama3Client):
        self.llama3_client = llama3_client

    def correct_code(self, code: str, error_message: str) -> str:
        """
        Attempts to correct the provided code based on the error message.

        Parameters:
        - code (str): The faulty code to be corrected.
        - error_message (str): The error message resulting from code execution.

        Returns:
        - str: The corrected code if successful, else an empty string.
        """
        try:
            prompt = (
                f"The following Python code has an error:\n\n{code}\n\n"
                f"Error Message:\n{error_message}\n\n"
                "Please correct the code to fix the error. Provide only the corrected code without any explanations."
            )
            logging.debug(f"ErrorCorrectionAgent Prompt: {prompt}")
            corrected_code = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.3)
            logging.debug(f"ErrorCorrectionAgent Corrected Code: {corrected_code}")

            # Optionally, validate the corrected code
            if self.validate_code(corrected_code):
                logging.info("Code successfully corrected by ErrorCorrectionAgent.")
                print_with_breaker("Code successfully corrected.")
                return corrected_code
            else:
                logging.warning("Corrected code failed validation.")
                print_with_breaker("Warning: Corrected code failed validation.")
                return ""

        except Exception as e:
            logging.error(f"Error in ErrorCorrectionAgent: {e}")
            print_with_breaker(f"Error: Failed to correct code: {e}")
            return ""

    def validate_code(self, code: str) -> bool:
        """
        Validates the corrected code by checking its syntax.

        Parameters:
        - code (str): The code to validate.

        Returns:
        - bool: True if the code is syntactically correct, else False.
        """
        try:
            compile(code, '<string>', 'exec')
            logging.info("Corrected code passed syntax validation.")
            return True
        except SyntaxError as e:
            logging.error(f"SyntaxError during code validation: {e}")
            print_with_breaker(f"SyntaxError during code validation: {e}")
            return False
        except Exception as e:
            logging.error(f"Unexpected error during code validation: {e}")
            print_with_breaker(f"Unexpected error during code validation: {e}")
            return False
