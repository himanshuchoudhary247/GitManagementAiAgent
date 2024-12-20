# agents/context_retrieval_agent.py

import logging
import json
from utils.code_utils import extract_json_from_response

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
                "Respond **only** in valid JSON format with a key `relevant_functions` containing a list of objects, each with `file` and `function` keys. "
                "Do not include any additional text, explanations, or surrounding context."
            )
            logging.debug(f"ContextRetrievalAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

            # Debugging: Print the raw response
            print(f"Raw Response: {response}")
            logging.debug(f"Raw Response: {response}")

            if not response:
                logging.error("Received empty response from LLM.")
                print("Error: Received empty response from LLM.")
                return

            # Extract JSON from the response
            relevant_functions_json = extract_json_from_response(response)
            if not relevant_functions_json:
                logging.error("Failed to extract JSON from the response.")
                print("Error: Failed to extract JSON from the response.")
                return

            # Handle both dict and list
            if isinstance(relevant_functions_json, dict):
                relevant_functions = relevant_functions_json.get('relevant_functions', [])
            elif isinstance(relevant_functions_json, list):
                relevant_functions = relevant_functions_json
            else:
                relevant_functions = []

            self.memory.set('relevant_functions', relevant_functions)
            logging.info(f"Retrieved Relevant Functions: {relevant_functions}")
            print(f"Retrieved Relevant Functions: {relevant_functions}")

        except Exception as e:
            logging.error(f"Error in ContextRetrievalAgent: {e}")
            print(f"Error: Failed to retrieve context: {e}")
