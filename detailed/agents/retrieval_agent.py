# agents/retrieval_agent.py

import logging
import json

class RetrievalAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def retrieve_relevant_functions(self, requirement, context):
        """
        Retrieve relevant functions from the codebase based on the requirement and context.
        This function uses the Llama 3 client to perform Retrieval-Augmented Generation.
        """
        try:
            # Construct the prompt for retrieval
            prompt = (
                f"Context: {json.dumps(context, indent=2)}\n\n"
                f"Requirement: {requirement}\n\n"
                "Based on the above context and requirement, list the functions that are relevant to fulfilling the requirement. "
                "Provide the file path and function name for each relevant function. "
                "Respond in JSON format as a list of objects with 'file' and 'function' keys."
            )
            logging.debug(f"Retrieval prompt: {prompt}")

            # Generate the response using Llama 3
            response = self.llama3_client.generate(prompt)

            if not response:
                logging.error("Failed to retrieve relevant functions.")
                print("Error: Failed to retrieve relevant functions.")
                return

            # Parse the JSON response
            relevant_functions = self.parse_relevant_functions(response)
            self.memory.set('relevant_functions', relevant_functions)
            logging.info(f"Retrieved relevant functions: {relevant_functions}")
            print(f"Retrieved relevant functions: {relevant_functions}")

        except Exception as e:
            logging.error(f"Error in retrieving relevant functions: {e}")
            print(f"Error: Failed to retrieve relevant functions: {e}")

    def parse_relevant_functions(self, response):
        """
        Parses the JSON response from the LLM into structured relevant functions.
        Expected response format:
        [
            {
                "file": "auth/login.py",
                "function": "login_user"
            },
            {
                "file": "utils/helpers.py",
                "function": "validate_token"
            }
        ]
        """
        try:
            relevant_functions = json.loads(response)
            # Validate the structure
            for func in relevant_functions:
                if not all(k in func for k in ("file", "function")):
                    raise ValueError("Invalid function format.")
            return relevant_functions
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing relevant functions: {e}")
            print(f"Error: Failed to parse relevant functions from response: {e}")
            return []
