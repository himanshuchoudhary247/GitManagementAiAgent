# agents/query_understanding_agent.py

import logging
import json

class QueryUnderstandingAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def understand_query(self, user_query):
        try:
            prompt = (
                f"User Query: {user_query}\n\n"
                "Please parse the above query and extract the key objectives. "
                "Respond in JSON format with a list of objectives."
            )
            logging.debug(f"QueryUnderstandingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=150, temperature=0.5)

            if not response:
                logging.error("Failed to understand the query.")
                print("Error: Failed to understand the query.")
                return

            # Parse JSON response
            objectives = self.parse_response(response)
            self.memory.set('objectives', objectives)
            logging.info(f"Parsed Objectives: {objectives}")
            print(f"Parsed Objectives: {objectives}")

        except Exception as e:
            logging.error(f"Error in QueryUnderstandingAgent: {e}")
            print(f"Error: Failed to understand the query: {e}")

    def parse_response(self, response):
        """
        Parses the JSON response into a list of objectives.
        Expected format:
        {
            "objectives": [
                "Objective 1",
                "Objective 2",
                ...
            ]
        }
        """
        try:
            data = json.loads(response)
            objectives = data.get('objectives', [])
            return objectives
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error in QueryUnderstandingAgent: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing response in QueryUnderstandingAgent: {e}")
            print(f"Error: Failed to parse objectives: {e}")
            return []
