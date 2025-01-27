# agents/plan_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import plan_creation_prompt
from agents.json_parse_agent import JSONParseAgent

class PlanAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="PlanAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, objectives):
        """
        Creates a detailed plan based on the given objectives.

        :param objectives: List of objectives.
        """
        try:
            prompt = plan_creation_prompt(objectives)
            response = self.llama3_client.generate(prompt, max_tokens=1000, temperature=0.2)
            if not response:
                self.log_warning("Received empty response from LLM while creating plan.")
                print_with_breaker("Warning: Received empty response from LLM while creating plan.")
                return
            # Parse JSON response
            parsed_response = self.json_parse_agent.execute(response)
            if parsed_response and 'plan' in parsed_response:
                plan = parsed_response['plan']
                self.centralized_memory.set('PlanAgent', 'plan', plan)
                self.log_info(f"Created plan: {plan}")
                print_with_breaker(f"Created plan: {plan}")
            else:
                self.log_warning("LLM response does not contain 'plan'.")
                print_with_breaker("Warning: LLM response does not contain 'plan'.")
        except Exception as e:
            self.log_error(f"Error during plan creation: {e}")
            print_with_breaker(f"Error: {e}")
