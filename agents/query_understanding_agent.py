# agents/query_understanding_agent.py

import logging
import json
from utils.code_utils import extract_json_from_response

class QueryUnderstandingAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def understand_query(self, user_query):
        try:
            prompt = (
                f"User Query: {user_query}\n\n"
                "Please parse the above query and extract the key objectives. "
                "Respond **only** in valid JSON format with a key `objectives` containing a list of objectives. "
                "Do not include any additional text, explanations, or surrounding context."
            )
            logging.debug(f"QueryUnderstandingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=150, temperature=0.5)

            # Debugging: Print the raw response
            print(f"Raw Response: {response}")
            logging.debug(f"Raw Response: {response}")

            if not response:
                logging.error("Received empty response from LLM.")
                print("Error: Received empty response from LLM.")
                return

            # Extract JSON from the response
            objectives_json = extract_json_from_response(response)
            if not objectives_json:
                logging.error("Failed to extract JSON from the response.")
                print("Error: Failed to extract JSON from the response.")
                return

            # Handle both dict and list
            if isinstance(objectives_json, dict):
                objectives = objectives_json.get('objectives', [])
            elif isinstance(objectives_json, list):
                objectives = objectives_json
            else:
                objectives = []

            self.memory.set('objectives', objectives)
            logging.info(f"Parsed Objectives: {objectives}")
            print(f"Parsed Objectives: {objectives}")

        except Exception as e:
            logging.error(f"Error in QueryUnderstandingAgent: {e}")
            print(f"Error: Failed to understand the query: {e}")
