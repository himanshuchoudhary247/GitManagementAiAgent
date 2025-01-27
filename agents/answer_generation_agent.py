# agents/answer_generation_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class AnswerGenerationAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def generate_answer(self, objectives: list, relevant_functions: list, additional_context: list, repo_path: str):
        """
        Generates answers or code changes based on the objectives and context.
        
        Parameters:
        - objectives (list): List of objectives to achieve.
        - relevant_functions (list): List of relevant functions or modules.
        - additional_context (list): Additional context or dependencies.
        - repo_path (str): Path to the repository.
        """
        try:
            prompt = (
                f"Objectives: {objectives}\n"
                f"Relevant Functions: {relevant_functions}\n"
                f"Additional Context: {additional_context}\n"
                f"Repository Path: {repo_path}\n\n"
                "Based on the above information, generate the necessary code changes required to achieve the objectives. "
                "Provide the code changes in JSON format with a key `code_changes` containing a list of changes. "
                "Each change should have an `action`, `file`, and `code`. "
                "No extra text."
            )
            logging.debug(f"AnswerGenerationAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)
            logging.debug(f"AnswerGenerationAgent Response: {response}")

            changes_json = extract_json_from_response(response)
            if changes_json and isinstance(changes_json, dict):
                self.memory.set('code_changes', changes_json.get('code_changes', []))
                logging.info(f"Generated code changes: {changes_json.get('code_changes', [])}")
                print_with_breaker(f"Generated code changes: {changes_json.get('code_changes', [])}")
            else:
                logging.warning("Failed to extract code changes from the response.")
                print_with_breaker("Warning: Failed to extract code changes from the response.")

        except Exception as e:
            logging.error(f"Error in AnswerGenerationAgent: {e}")
            print_with_breaker(f"Error: Failed to generate answer: {e}")
