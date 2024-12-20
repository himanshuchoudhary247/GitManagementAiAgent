# agents/answer_generation_agent.py

import logging
import json

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
                "Provide the changes in a structured JSON format with 'action', 'file', and 'code' keys."
            )
            logging.debug(f"AnswerGenerationAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.5)

            if not response:
                logging.error("Failed to generate the answer.")
                print("Error: Failed to generate the answer.")
                return

            # Parse JSON response
            code_changes = self.parse_response(response)
            self.memory.set('code_changes', code_changes)
            logging.info(f"Generated Code Changes: {code_changes}")
            print(f"Generated Code Changes: {code_changes}")

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
            data = json.loads(response)
            code_changes = data.get('code_changes', [])
            return code_changes
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error in AnswerGenerationAgent: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing response in AnswerGenerationAgent: {e}")
            print(f"Error: Failed to parse code changes: {e}")
            return []
