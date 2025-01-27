# agents/context_retrieval_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class ContextRetrievalAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def retrieve_context(self, objectives: list, additional_info: dict):
        """
        Retrieves relevant context based on the objectives and additional information.
        
        Parameters:
        - objectives (list): List of objectives to retrieve context for.
        - additional_info (dict): Additional information to aid in context retrieval.
        """
        try:
            prompt = (
                f"Objectives: {objectives}\n"
                f"Additional Information: {additional_info}\n\n"
                "Retrieve relevant context, functions, and modules that are necessary to achieve these objectives. "
                "Provide the context in JSON format with a key `relevant_functions` containing a list of functions or modules."
                "No extra text."
            )
            logging.debug(f"ContextRetrievalAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.3)
            logging.debug(f"ContextRetrievalAgent Response: {response}")

            context_json = extract_json_from_response(response)
            if context_json and isinstance(context_json, dict):
                self.memory.set('relevant_functions', context_json.get('relevant_functions', []))
                logging.info(f"Retrieved context: {context_json.get('relevant_functions', [])}")
                print_with_breaker(f"Retrieved context: {context_json.get('relevant_functions', [])}")
            else:
                logging.warning("Failed to extract context from the response.")
                print_with_breaker("Warning: Failed to extract context from the response.")

        except Exception as e:
            logging.error(f"Error in ContextRetrievalAgent: {e}")
            print_with_breaker(f"Error: Failed to retrieve context: {e}")
