# agents/plan_agent.py

import logging
from utils.cli_utils import print_with_breaker
from utils.llama3_client import Llama3Client
from utils.code_utils import extract_json_from_response

class PlanAgent:
    def __init__(self, llama3_client: Llama3Client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def create_plan(self, objectives: list) -> str:
        """
        Creates a plan based on the provided objectives.

        Parameters:
        - objectives (list): List of objectives to achieve.

        Returns:
        - str: Raw response from the LLM.
        """
        try:
            prompt = (
                f"Objectives: {objectives}\n\n"
                "Create a detailed plan to achieve these objectives. "
                "Break down each objective into actionable tasks. "
                "Respond in valid JSON format with a key `plan` containing a list of sub-plans. "
                "Each sub-plan should have an `objective` and a list of `tasks`. "
                "No extra text."
            )
            logging.debug(f"PlanAgent Prompt: {prompt}")
            response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)
            logging.debug(f"PlanAgent Response: {response}")

            plan_json = extract_json_from_response(response)
            if plan_json and isinstance(plan_json, dict):
                self.memory.set('plan', plan_json.get('plan', []))
                logging.info(f"Generated plan: {plan_json.get('plan', [])}")
                print_with_breaker(f"Generated plan: {plan_json.get('plan', [])}")
            else:
                logging.warning("Failed to extract plan from the response.")
                print_with_breaker("Warning: Failed to extract plan from the response.")

            return response  # Return the raw response for potential correction

        except Exception as e:
            logging.error(f"Error in PlanAgent: {e}")
            print_with_breaker(f"Error: Failed to create plan: {e}")
            return ""
