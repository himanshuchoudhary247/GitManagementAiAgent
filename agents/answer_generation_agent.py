# agents/answer_generation_agent.py

import logging
import json
from utils.code_utils import extract_json_from_response

class AnswerGenerationAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def generate_answer(self, objectives, relevant_functions, additional_context, repo_path):
        try:
            prompt = (
                f"Objectives: {json.dumps(objectives, indent=2)}\n\n"
                f"Relevant Functions: {json.dumps(relevant_functions, indent=2)}\n\n"
                f"Additional Context: {json.dumps(additional_context, indent=2)}\n\n"
                f"Repository Path: {repo_path}\n\n"
                "Based on the above information, generate the necessary code changes to achieve the objectives. "
                "Specify whether to add new functions or update existing ones. "
                "Provide the changes in a structured JSON format with 'action', 'file', and 'code' keys. "
                "Use only 'add' for adding new functions and 'update' for modifying existing ones. "
                "Do not include any additional text, explanations, or surrounding context."
            )
            logging.debug(f"AnswerGenerationAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.5)

            # Debugging: Print the raw response
            print(f"Raw Response: {response}")
            logging.debug(f"Raw Response: {response}")

            if not response:
                logging.error("Received empty response from LLM.")
                print("Error: Received empty response from LLM.")
                return

            # Extract JSON from the response
            code_changes_json = extract_json_from_response(response)
            if not code_changes_json:
                logging.error("Failed to extract JSON from the response.")
                print("Error: Failed to extract JSON from the response.")
                return

            # Handle both dict and list
            if isinstance(code_changes_json, dict):
                code_changes = code_changes_json.get('code_changes', [])
            elif isinstance(code_changes_json, list):
                code_changes = code_changes_json
            else:
                code_changes = []

            # Filter out unsupported actions
            supported_actions = ['add', 'update']
            filtered_code_changes = []
            for change in code_changes:
                action = change.get('action', '').lower()
                if action in supported_actions:
                    filtered_code_changes.append(change)
                else:
                    logging.warning(f"Unsupported action '{action}' detected. Skipping this change.")
                    print(f"Warning: Unsupported action '{action}' detected. Skipping this change.")

            self.memory.set('code_changes', filtered_code_changes)
            logging.info(f"Generated Code Changes: {filtered_code_changes}")
            print(f"Generated Code Changes: {filtered_code_changes}")

        except Exception as e:
            logging.error(f"Error in AnswerGenerationAgent: {e}")
            print(f"Error: Failed to generate the answer: {e}")

    def parse_response(self, response):
        """
        Parses the JSON response into code changes.
        Expected format:
        {
            "code_changes": [
                {
                    "action": "add",
                    "file": "auth/login.py",
                    "code": "def authenticate_user(...): ..."
                },
                {
                    "action": "update",
                    "file": "utils/helpers.py",
                    "code": "def validate_token(...): ..."
                },
                ...
            ]
        }
        """
        try:
            data = extract_json_from_response(response)
            code_changes = data.get('code_changes', [])
            return code_changes
        except Exception as e:
            logging.error(f"Error parsing response in AnswerGenerationAgent: {e}")
            print(f"Error: Failed to parse code changes: {e}")
            return []
