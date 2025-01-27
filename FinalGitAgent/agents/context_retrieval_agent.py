# agents/context_retrieval_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import context_retrieval_prompt
from agents.json_parse_agent import JSONParseAgent

class ContextRetrievalAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="ContextRetrievalAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, sub_objectives):
        """
        Retrieves context related to each sub-objective.

        :param sub_objectives: List of sub-objectives.
        """
        try:
            for sub_objective in sub_objectives:
                prompt = context_retrieval_prompt(sub_objective)
                response = self.llama3_client.generate(prompt, max_tokens=700, temperature=0.3)
                if not response:
                    self.log_warning(f"Received empty response from LLM while retrieving context for '{sub_objective}'.")
                    print_with_breaker(f"Warning: Received empty response from LLM while retrieving context for '{sub_objective}'.")
                    continue
                # Parse JSON response
                parsed_response = self.json_parse_agent.execute(response)
                if parsed_response and 'context' in parsed_response:
                    context = parsed_response['context']
                    self.centralized_memory.set('ContextRetrievalAgent', f'context_{sub_objective}', context)
                    self.log_info(f"Retrieved context for '{sub_objective}': {context}")
                    print_with_breaker(f"Retrieved context for '{sub_objective}': {context}")
                else:
                    self.log_warning(f"LLM response does not contain 'context' for '{sub_objective}'.")
                    print_with_breaker(f"Warning: LLM response does not contain 'context' for '{sub_objective}'.")
        except Exception as e:
            self.log_error(f"Error during context retrieval: {e}")
            print_with_breaker(f"Error: {e}")
