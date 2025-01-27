# agents/code_validation_agent.py

import ast
import logging
import os
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import function_completeness_prompt
from agents.json_parse_agent import JSONParseAgent

class CodeValidationAgent(BaseAgent):
    def __init__(self, llama3_client, centralized_memory, repo_path):
        super().__init__(llama3_client, centralized_memory, name="CodeValidationAgent")
        self.repo_path = os.path.abspath(repo_path)
        self.max_retries = 2
        self.json_parse_agent = JSONParseAgent(llama3_client, centralized_memory)
    
    def execute(self):
        """
        Scans the repository for functions, analyzes their completeness using an LLM, and logs incomplete functions.
        Outputs the results in JSON format.
        """
        incomplete_functions = []
        try:
            for root, dirs, files in os.walk(self.repo_path):
                for file in files:
                    if file.endswith('.py'):
                        file_path = os.path.join(root, file)
                        functions = self.parse_functions(file_path)
                        for func in functions:
                            is_complete = self.analyze_function_completeness(func['code'])
                            if not is_complete['status'] == 'Complete':
                                incomplete_functions.append({
                                    'file': os.path.relpath(file_path, self.repo_path),
                                    'function': func['name'],
                                    'suggestions': is_complete.get('suggestions', '')
                                })
            output = {
                "incomplete_functions": incomplete_functions
            }
            # Ensure output is JSON
            output_json = json.dumps(output)
            self.centralized_memory.set('CodeValidationAgent', 'incomplete_functions', incomplete_functions)
            self.log_info(f"Found {len(incomplete_functions)} incomplete functions.")
            print_with_breaker(f"Found {len(incomplete_functions)} incomplete functions.")
            return output
        except Exception as e:
            self.log_error(f"Error during code validation: {e}")
            print_with_breaker(f"Error: {e}")
            return None

    def parse_functions(self, file_path):
        """
        Parses a Python file to extract all function definitions.

        :param file_path: Path to the Python file.
        :return: List of dictionaries containing function names and their code.
        """
        functions = []
        try:
            with open(file_path, 'r') as f:
                file_content = f.read()
            tree = ast.parse(file_content, filename=file_path)
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef):
                    func_code = self.get_function_code(file_content, node)
                    functions.append({
                        'name': node.name,
                        'code': func_code
                    })
        except SyntaxError as e:
            self.log_warning(f"Syntax error in '{file_path}': {e}")
        except Exception as e:
            self.log_warning(f"Failed to parse '{file_path}': {e}")
        return functions

    def get_function_code(self, file_content, node):
        """
        Extracts the complete code of a function from the file content.

        :param file_content: Full content of the Python file.
        :param node: AST FunctionDef node.
        :return: Function code as a string.
        """
        lines = file_content.split('\n')
        start_line = node.lineno - 1  # AST is 1-indexed
        if hasattr(node, 'end_lineno'):
            end_line = node.end_lineno
        else:
            # Fallback if end_lineno is not available (Python < 3.8)
            end_line = self.find_end_line(lines, start_line)
        func_lines = lines[start_line:end_line]
        return '\n'.join(func_lines)

    def find_end_line(self, lines, start_line):
        """
        Finds the end line of a function by detecting the next function or class definition.

        :param lines: List of all lines in the file.
        :param start_line: Line number where the function starts.
        :return: Line number where the function ends.
        """
        for i in range(start_line + 1, len(lines)):
            stripped = lines[i].strip()
            if stripped.startswith('def ') or stripped.startswith('class '):
                return i
        return len(lines)

    def analyze_function_completeness(self, function_code):
        """
        Uses an LLM to determine if a function is complete.

        :param function_code: The complete code of the function.
        :return: Dictionary with 'status' and 'suggestions'.
        """
        try:
            prompt = function_completeness_prompt(function_code)
            response = self.llama3_client.generate(prompt, max_tokens=150, temperature=0.2)
            if not response:
                self.log_warning("Received empty response from LLM while analyzing function completeness.")
                print_with_breaker("Warning: Received empty response from LLM while analyzing function completeness.")
                return {"status": "Incomplete", "suggestions": "No response received."}
            # Attempt to parse JSON response
            parsed_response = self.json_parse_agent.execute(response)
            if parsed_response:
                return parsed_response
            else:
                # If parsing failed, return incomplete with no suggestions
                return {"status": "Incomplete", "suggestions": "Unable to parse LLM response."}
        except Exception as e:
            self.log_error(f"Error during function completeness analysis: {e}")
            print_with_breaker(f"Error: {e}")
            return {"status": "Incomplete", "suggestions": "Exception occurred during analysis."}
