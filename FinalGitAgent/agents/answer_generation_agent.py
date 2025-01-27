# agents/answer_generation_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import answer_generation_prompt
from agents.json_parse_agent import JSONParseAgent

class AnswerGenerationAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="AnswerGenerationAgent")
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self, sub_objectives, relevant_functions, additional_context, repo_path):
        """
        Generates code changes based on the sub-objective and context.

        :param sub_objectives: List of sub-objectives.
        :param relevant_functions: List of relevant functions.
        :param additional_context: Additional context from intermediate processing.
        :param repo_path: Path to the repository.
        """
        try:
            for sub_objective in sub_objectives:
                context = self.centralized_memory.get('IntermediateProcessingAgent', f'additional_context_{sub_objective}', [])
                prompt = answer_generation_prompt(sub_objective, relevant_functions, context, repo_path)
                response = self.llama3_client.generate(prompt, max_tokens=1000, temperature=0.3)
                if not response:
                    self.log_warning(f"Received empty response from LLM during answer generation for '{sub_objective}'.")
                    print_with_breaker(f"Warning: Received empty response from LLM during answer generation for '{sub_objective}'.")
                    continue
                # Parse JSON response
                parsed_response = self.json_parse_agent.execute(response)
                if parsed_response and 'code_changes' in parsed_response:
                    code_changes = parsed_response['code_changes']
                    self.centralized_memory.set('AnswerGenerationAgent', f'code_changes_{sub_objective}', code_changes)
                    self.log_info(f"Generated code changes for '{sub_objective}': {code_changes}")
                    print_with_breaker(f"Generated code changes for '{sub_objective}': {code_changes}")
                else:
                    self.log_warning(f"LLM response does not contain 'code_changes' for '{sub_objective}'.")
                    print_with_breaker(f"Warning: LLM response does not contain 'code_changes' for '{sub_objective}'.")
        except Exception as e:
            self.log_error(f"Error during answer generation: {e}")
            print_with_breaker(f"Error: {e}")
