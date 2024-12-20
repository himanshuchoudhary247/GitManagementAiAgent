# agents/intermediate_processing_agent.py

import logging
import json
from utils.code_utils import extract_json_from_response

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
                "Respond **only** in valid JSON format with any additional files or functions needed. "
                "Do not include any additional text, explanations, or surrounding context."
            )
            logging.debug(f"IntermediateProcessingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

            # Debugging: Print the raw response
            print(f"Raw Response: {response}")
            logging.debug(f"Raw Response: {response}")

            if not response:
                logging.error("Received empty response from LLM.")
                print("Error: Received empty response from LLM.")
                return

            # Extract JSON from the response
            additional_context_json = extract_json_from_response(response)
            if not additional_context_json:
                logging.error("Failed to extract JSON from the response.")
                print("Error: Failed to extract JSON from the response.")
                return

            # Handle both dict and list
            if isinstance(additional_context_json, dict):
                additional_context = additional_context_json.get('additional_context', [])
            elif isinstance(additional_context_json, list):
                additional_context = additional_context_json
            else:
                additional_context = []

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
            data = extract_json_from_response(response)
            additional_context = data.get('additional_context', [])
            return additional_context
        except Exception as e:
            logging.error(f"Error parsing response in IntermediateProcessingAgent: {e}")
            print(f"Error: Failed to parse additional context: {e}")
            return []
