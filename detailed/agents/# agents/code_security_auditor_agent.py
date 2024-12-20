# agents/code_security_auditor_agent.py

import logging
import json

class CodeSecurityAuditorAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def audit_security(self):
        try:
            # Retrieve all functions from memory
            repo_map = self.memory.get('repo_map', {})
            if not repo_map:
                logging.info("No repository map available for security auditing.")
                print("No repository map available for security auditing.")
                return

            security_issues = []

            for file, functions in repo_map.items():
                file_path = os.path.join(self.repo_path, file)
                for function in functions:
                    issues = self.audit_function(file_path, function)
                    if issues:
                        security_issues.extend(issues)

            self.memory.set('security_issues', security_issues)
            logging.info(f"Security issues found: {security_issues}")
            print(f"Security issues found: {security_issues}")

        except Exception as e:
            logging.error(f"Error in security auditing: {e}")
            print(f"Error: Failed to audit security: {e}")

    def audit_function(self, file_path, function_name):
        try:
            with open(file_path, 'r') as f:
                content = f.read()

            # Extract function code
            lines = content.split('\n')
            start_line, end_line = self.find_function_definitions(content, function_name)
            if start_line is None or end_line is None:
                logging.warning(f"Function '{function_name}' not found in '{file_path}'. Skipping auditing.")
                return []

            function_code = '\n'.join(lines[start_line:end_line])

            # Construct the prompt for security auditing
            prompt = (
                f"Function Code:\n{function_code}\n\n"
                f"Requirement: Audit the above function for security vulnerabilities. "
                f"List any potential security issues and provide recommendations to fix them. "
                f"Respond in JSON format with 'file', 'function', 'issues', and 'recommendations' keys."
            )
            logging.debug(f"Security audit prompt for '{function_name}': {prompt}")

            # Generate the response using Llama 3
            audit_feedback = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.2)

            if not audit_feedback:
                logging.error(f"Failed to audit function '{function_name}' in '{file_path}'.")
                print(f"Error: Failed to audit function '{function_name}' in '{file_path}'.")
                return []

            # Parse the JSON response
            feedback = self.parse_feedback(audit_feedback)
            if feedback:
                return feedback
            else:
                return []

        except Exception as e:
            logging.error(f"Unexpected error while auditing function '{function_name}' in '{file_path}': {e}")
            print(f"Error: Unexpected error while auditing function '{function_name}' in '{file_path}': {e}")
            return []

    def parse_feedback(self, response):
        """
        Parses the JSON response from the LLM into structured security feedback.
        Expected response format:
        [
            {
                "file": "auth/login.py",
                "function": "login_user",
                "issues": ["Issue 1", "Issue 2"],
                "recommendations": ["Recommendation 1", "Recommendation 2"]
            },
            ...
        ]
        """
        try:
            feedback = json.loads(response)
            # Validate the structure
            for item in feedback:
                if not all(k in item for k in ("file", "function", "issues", "recommendations")):
                    raise ValueError("Invalid feedback format.")
            return feedback
        except json.JSONDecodeError as e:
            logging.error(f"JSON decode error: {e}")
            print(f"Error: Invalid JSON format in response: {e}")
            return []
        except Exception as e:
            logging.error(f"Error parsing feedback: {e}")
            print(f"Error: Failed to parse feedback from response: {e}")
            return []

    def find_function_definitions(self, content, function_name):
        """
        Finds the start and end line numbers of a function definition in the given content.
        Returns a tuple (start_line, end_line). If not found, returns (None, None).
        """
        lines = content.split('\n')
        start_line = None
        end_line = None
        for i, line in enumerate(lines):
            if line.strip().startswith(f"def {function_name}("):
                start_line = i
                # Find the end of the function by looking for the next function or end of file
                for j in range(i + 1, len(lines)):
                    if lines[j].strip().startswith("def "):
                        end_line = j
                        break
                if end_line is None:
                    end_line = len(lines)
                break
        return start_line, end_line
