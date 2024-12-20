# agents/context_retrieval_agent.py

import logging
import json

class ContextRetrievalAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def retrieve_context(self, objectives, repo_map):
        try:
            prompt = (
                f"Objectives: {json.dumps(objectives, indent=2)}\n\n"
                f"Repository Map: {json.dumps(repo_map, indent=2)}\n\n"
                "Please identify and list the files and specific functions that are relevant to achieving the above objectives. "
                "Respond in JSON format with each item containing 'file' and 'function' keys."
            )
            logging.debug(f"ContextRetrievalAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

            if not response:
                logging.error("Failed to retrieve context.")
                print("Error: Failed to retrieve context.")
                return

            # Parse JSON response
            relevant_functions = self.parse_response(response)
            self.memory.set('relevant_functions', relevant_functions)
            logging.info(f"Retrieved Relevant Functions: {relevant_functions}")
            print(f"Retrieved Relevant Functions: {relevant_functions}")

        except Exception as e:
            logging.error(f"Error in ContextRetrievalAgent: {e}")
            print(f"Error: Failed to retrieve context: {e}")

    def parse_response(self, response):
        """
        Parses the JSON response into a list of relevant functions.
        Expected format:
        {
            "relevant_functions": [
                {"file": "auth/login.py", "function": "login_user"},
                {"file": "utils/helpers.py", "function": "validate_token"},
                ...
            ]
        }
        """
        try:
            data = json.loads(response)
            relevant_functions = data.get('relevant_functions', [])
            return relevant_functions
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error in ContextRetrievalAgent: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing response in ContextRetrievalAgent: {e}")
            print(f"Error: Failed to parse relevant functions: {e}")
            return []
