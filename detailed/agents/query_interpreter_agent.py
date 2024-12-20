# agents/query_interpreter_agent.py

import logging
import json

class QueryInterpreterAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def interpret_requirement(self, requirement, context):
        try:
            prompt = (
                f"Context: {json.dumps(context, indent=2)}\n\n"
                f"Requirement: {requirement}\n\n"
                "Please extract the specific actions to be taken, such as adding or updating functions, including the file paths and function names. "
                "Respond in JSON format with each action containing 'action_type', 'file', and 'function'."
            )
            logging.debug(f"Interpreting requirement with prompt: {prompt}")
            response = self.llama3_client.generate(prompt)

            if not response:
                logging.error("Failed to interpret requirement.")
                print("Error: Failed to interpret requirement.")
                return

            # Parse the JSON response
            actions = self.parse_actions(response)
            self.memory.set('interpreted_requirement', actions)
            logging.info(f"Interpreted actions: {actions}")
            print(f"Interpreted actions: {actions}")

        except Exception as e:
            logging.error(f"Error in interpreting requirement: {e}")
            print(f"Error: Failed to interpret requirement: {e}")

    def parse_actions(self, response):
        """
        Parses the JSON response from the LLM into structured actions.
        Expected response format:
        [
            {
                "action_type": "Add",
                "file": "auth/login.py",
                "function": "authenticate_user"
            },
            {
                "action_type": "Update",
                "file": "utils/helpers.py",
                "function": "calculate_sum"
            }
        ]
        """
        try:
            actions = json.loads(response)
            # Validate the structure
            for action in actions:
                if not all(k in action for k in ("action_type", "file", "function")):
                    raise ValueError("Invalid action format.")
            return actions
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing actions: {e}")
            print(f"Error: Failed to parse actions from response: {e}")
            return []
