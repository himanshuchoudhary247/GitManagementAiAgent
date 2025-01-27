# agents/intermediate_processing_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import intermediate_processing_prompt
from agents.json_parse_agent import JSONParseAgent

class IntermediateProcessingAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="IntermediateProcessingAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, sub_objectives, relevant_functions, repo_path):
        """
        Processes the context to prepare for code generation.

        :param sub_objectives: List of sub-objectives.
        :param relevant_functions: List of relevant functions.
        :param repo_path: Path to the repository.
        """
        try:
            for sub_objective in sub_objectives:
                context = self.centralized_memory.get('ContextRetrievalAgent', f'context_{sub_objective}', [])
                prompt = intermediate_processing_prompt(sub_objective, relevant_functions, repo_path)
                response = self.llama3_client.generate(prompt, max_tokens=700, temperature=0.3)
                if not response:
                    self.log_warning(f"Received empty response from LLM during intermediate processing for '{sub_objective}'.")
                    print_with_breaker(f"Warning: Received empty response from LLM during intermediate processing for '{sub_objective}'.")
                    continue
                # Parse JSON response
                parsed_response = self.json_parse_agent.execute(response)
                if parsed_response and 'additional_context' in parsed_response:
                    additional_context = parsed_response['additional_context']
                    self.centralized_memory.set('IntermediateProcessingAgent', f'additional_context_{sub_objective}', additional_context)
                    self.log_info(f"Processed additional context for '{sub_objective}': {additional_context}")
                    print_with_breaker(f"Processed additional context for '{sub_objective}': {additional_context}")
                else:
                    self.log_warning(f"LLM response does not contain 'additional_context' for '{sub_objective}'.")
                    print_with_breaker(f"Warning: LLM response does not contain 'additional_context' for '{sub_objective}'.")
        except Exception as e:
            self.log_error(f"Error during intermediate processing: {e}")
            print_with_breaker(f"Error: {e}")
