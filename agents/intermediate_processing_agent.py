# agents/intermediate_processing_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response  # Added import

class IntermediateProcessingAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def process(self, objectives: list, relevant_functions: list, repo_path: str):
        """
        Processes the objectives and relevant functions to prepare for answer generation.
        
        Parameters:
        - objectives (list): List of objectives to process.
        - relevant_functions (list): List of relevant functions or modules.
        - repo_path (str): Path to the repository.
        """
        try:
            prompt = (
                f"Objectives: {objectives}\n"
                f"Relevant Functions: {relevant_functions}\n"
                f"Repository Path: {repo_path}\n\n"
                "Process the objectives and relevant functions to identify additional context or dependencies required for answer generation. "
                "Provide the additional context in JSON format with a key `additional_context` containing relevant information."
                "No extra text."
            )
            logging.debug(f"IntermediateProcessingAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.3)
            logging.debug(f"IntermediateProcessingAgent Response: {response}")

            context_json = extract_json_from_response(response)
            if context_json and isinstance(context_json, dict):
                self.memory.set('additional_context', context_json.get('additional_context', []))
                logging.info(f"Retrieved additional context: {context_json.get('additional_context', [])}")
                print_with_breaker(f"Retrieved additional context: {context_json.get('additional_context', [])}")
            else:
                logging.warning("Failed to extract additional context from the response.")
                print_with_breaker("Warning: Failed to extract additional context from the response.")

        except Exception as e:
            logging.error(f"Error in IntermediateProcessingAgent: {e}")
            print_with_breaker(f"Error: Failed to process objectives: {e}")