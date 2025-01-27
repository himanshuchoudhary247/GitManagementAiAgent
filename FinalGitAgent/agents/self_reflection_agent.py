# agents/self_reflection_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import self_reflection_prompt
from agents.json_parse_agent import JSONParseAgent

class SelfReflectionAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="SelfReflectionAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, code_changes):
        """
        Reflects on the code changes made and identifies potential improvements.

        :param code_changes: List of code changes.
        """
        if not code_changes:
            self.log_info("No code changes to reflect on.")
            print_with_breaker("No code changes to reflect on.")
            return
        try:
            prompt = self_reflection_prompt(code_changes)
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)
            if not response:
                self.log_warning("Received empty response from LLM during self-reflection.")
                print_with_breaker("Warning: Received empty response from LLM during self-reflection.")
                return
            # Parse JSON response
            parsed_response = self.json_parse_agent.execute(response)
            if parsed_response and 'reflection' in parsed_response:
                reflection = parsed_response['reflection']
                self.centralized_memory.set('SelfReflectionAgent', 'reflection', reflection)
                self.log_info(f"Reflection: {reflection}")
                print_with_breaker(f"Reflection: {reflection}")
            else:
                self.log_warning("LLM response does not contain 'reflection'.")
                print_with_breaker("Warning: LLM response does not contain 'reflection'.")
        except Exception as e:
            self.log_error(f"Error during self-reflection: {e}")
            print_with_breaker(f"Error: {e}")
