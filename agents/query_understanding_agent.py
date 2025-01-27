# agents/query_understanding_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class QueryUnderstandingAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def understand_query(self, query: str):
        """
        Understands and parses the user's query into actionable objectives.
        
        Parameters:
        - query (str): The user's input query.
        """
        try:
            prompt = (
                f"User Query: {query}\n\n"
                "Understand the intent of the above query and extract clear and actionable objectives. "
                "Respond in JSON format with a key `objectives` containing a list of objectives. "
                "No extra text."
            )
            logging.debug(f"QueryUnderstandingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)
            logging.debug(f"QueryUnderstandingAgent Response: {response}")

            objectives_json = extract_json_from_response(response)
            if objectives_json and isinstance(objectives_json, dict):
                self.memory.set('objectives', objectives_json.get('objectives', []))
                logging.info(f"Extracted objectives: {objectives_json.get('objectives', [])}")
                print_with_breaker(f"Extracted objectives: {objectives_json.get('objectives', [])}")
            else:
                logging.warning("Failed to extract objectives from the response.")
                print_with_breaker("Warning: Failed to extract objectives from the response.")

        except Exception as e:
            logging.error(f"Error in QueryUnderstandingAgent: {e}")
            print_with_breaker(f"Error: Failed to understand query: {e}")
