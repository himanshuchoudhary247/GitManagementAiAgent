# agents/query_understanding_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import query_understanding_prompt
from agents.json_parse_agent import JSONParseAgent

class QueryUnderstandingAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="QueryUnderstandingAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, requirement):
        """
        Understands the user requirement by breaking it down into actionable objectives.

        :param requirement: User's requirement as a string.
        """
        try:
            prompt = query_understanding_prompt(requirement)
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)
            if not response:
                self.log_warning("Received empty response from LLM while understanding query.")
                print_with_breaker("Warning: Received empty response from LLM while understanding query.")
                return
            # Parse JSON response
            parsed_response = self.json_parse_agent.execute(response)
            if parsed_response and 'objectives' in parsed_response:
                objectives = parsed_response['objectives']
                self.centralized_memory.set('QueryUnderstandingAgent', 'objectives', objectives)
                self.log_info(f"Parsed objectives: {objectives}")
                print_with_breaker(f"Parsed objectives: {objectives}")
            else:
                self.log_warning("LLM response does not contain 'objectives'.")
                print_with_breaker("Warning: LLM response does not contain 'objectives'.")
        except Exception as e:
            self.log_error(f"Error during query understanding: {e}")
            print_with_breaker(f"Error: {e}")
