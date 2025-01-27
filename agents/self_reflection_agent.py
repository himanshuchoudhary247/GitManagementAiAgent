# agents/self_reflection_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class SelfReflectionAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def reflect_on_changes(self, code_changes: list):
        """
        Reflects on the changes made to improve future operations.

        Parameters:
        - code_changes (list): A list of code changes made.
        """
        try:
            prompt = (
                f"The following code changes were made:\n{code_changes}\n\n"
                "Reflect on these changes and suggest any improvements or best practices that could enhance future operations."
            )
            logging.debug(f"SelfReflectionAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.3)
            logging.debug(f"SelfReflectionAgent Response: {response}")

            reflection = response.strip()
            print_with_breaker(f"Self-Reflection: {reflection}")
            logging.info(f"Self-Reflection: {reflection}")
        except Exception as e:
            logging.error(f"Error in SelfReflectionAgent: {e}")
            print_with_breaker(f"Error: Failed to perform self-reflection: {e}")
