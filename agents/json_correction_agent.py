# agents/json_correction_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class JSONCorrectionAgent:
    def __init__(self, llama3_client: Llama3Client):
        self.llama3_client = llama3_client

    def correct_json(self, malformed_json: str) -> dict:
        """
        Attempts to correct malformed JSON using the LLM.

        Parameters:
        - malformed_json (str): The JSON string that needs correction.

        Returns:
        - dict or None: The corrected JSON object if successful, else None.
        """
        try:
            prompt = (
                f"The following text is supposed to be a JSON, but it's malformed:\n\n{malformed_json}\n\n"
                "Please correct the JSON so that it is valid and properly formatted. "
                "Respond with only the corrected JSON."
            )
            logging.debug(f"JSONCorrectionAgent Prompt: {prompt}")
            corrected_response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)
            logging.debug(f"JSONCorrectionAgent Response: {corrected_response}")

            corrected_json = extract_json_from_response(corrected_response)
            if corrected_json:
                logging.info("JSON successfully corrected.")
                print_with_breaker("JSON successfully corrected.")
                return corrected_json
            else:
                logging.warning("Failed to extract corrected JSON from the response.")
                print_with_breaker("Warning: Failed to extract corrected JSON from the response.")
                return None
        except Exception as e:
            logging.error(f"Error in JSONCorrectionAgent: {e}")
            print_with_breaker(f"Error: JSON correction failed: {e}")
            return None
