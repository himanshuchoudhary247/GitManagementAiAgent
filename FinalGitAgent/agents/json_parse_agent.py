# agents/json_parse_agent.py

import json
import logging
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker

class JSONParseAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory):
        super().__init__(llama3_client, centralized_memory, name="JSONParseAgent")
    
    def execute(self, raw_string):
        """
        Attempts to parse a raw string into JSON. If parsing fails, uses the LLM to correct the JSON format.

        :param raw_string: The raw string output from an agent.
        :return: Parsed JSON object or None if unable to parse.
        """
        try:
            parsed_json = json.loads(raw_string)
            self.log_info("Successfully parsed JSON.")
            print_with_breaker("Successfully parsed JSON.")
            return parsed_json
        except json.JSONDecodeError as e:
            self.log_warning(f"JSON parsing failed: {e}")
            print_with_breaker(f"Warning: JSON parsing failed: {e}")
            corrected_json = self.correct_json(raw_string)
            if corrected_json:
                return corrected_json
            else:
                self.log_error("Failed to correct JSON.")
                print_with_breaker("Error: Failed to correct JSON.")
                return None
    
    def correct_json(self, raw_string):
        """
        Uses the LLM to correct malformed JSON strings.

        :param raw_string: The raw string that failed to parse.
        :return: Corrected JSON object or None.
        """
        try:
            prompt = f"""
You are a JSON parsing assistant. The following string is intended to be a JSON object but contains errors. Correct the JSON syntax and return the properly formatted JSON.

Raw String:
{raw_string}

Return only the corrected JSON without any additional text.
"""
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.2)
            if not response:
                self.log_warning("Received empty response while attempting to correct JSON.")
                print_with_breaker("Warning: Received empty response while attempting to correct JSON.")
                return None
            try:
                corrected_json = json.loads(response)
                self.log_info("Successfully corrected and parsed JSON.")
                print_with_breaker("Successfully corrected and parsed JSON.")
                return corrected_json
            except json.JSONDecodeError as e:
                self.log_error(f"Failed to parse corrected JSON: {e}")
                print_with_breaker(f"Error: Failed to parse corrected JSON: {e}")
                return None
        except Exception as e:
            self.log_error(f"Error during JSON correction: {e}")
            print_with_breaker(f"Error: {e}")
            return None
