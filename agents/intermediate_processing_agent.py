# agents/intermediate_processing_agent.py

import logging
import json

class IntermediateProcessingAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def process(self, objectives, relevant_functions, repo_path):
        try:
            prompt = (
                f"Objectives: {json.dumps(objectives, indent=2)}\n\n"
                f"Relevant Functions: {json.dumps(relevant_functions, indent=2)}\n\n"
                f"Repository Path: {repo_path}\n\n"
                "Please analyze the above information and identify any dependencies or additional context required to achieve the objectives. "
                "Respond in JSON format with any additional files or functions needed."
            )
            logging.debug(f"IntermediateProcessingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

            if not response:
                logging.error("Failed to process intermediate data.")
                print("Error: Failed to process intermediate data.")
                return

            # Parse JSON response
            additional_context = self.parse_response(response)
            self.memory.set('additional_context', additional_context)
            logging.info(f"Additional Context: {additional_context}")
            print(f"Additional Context: {additional_context}")

        except Exception as e:
            logging.error(f"Error in IntermediateProcessingAgent: {e}")
            print(f"Error: Failed to process intermediate data: {e}")

    def parse_response(self, response):
        """
        Parses the JSON response into additional context.
        Expected format:
        {
            "additional_context": [
                {"file": "auth/utils.py", "function": "encrypt_password"},
                ...
            ]
        }
        """
        try:
            data = json.loads(response)
            additional_context = data.get('additional_context', [])
            return additional_context
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error in IntermediateProcessingAgent: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing response in IntermediateProcessingAgent: {e}")
            print(f"Error: Failed to parse additional context: {e}")
            return []
