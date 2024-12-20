# agents/code_reviewer_agent.py

import logging
import json

class CodeReviewerAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory

    def review_code(self):
        try:
            # Retrieve all changes from memory
            changes = self.memory.get('code_changes', {})
            if not changes:
                logging.info("No code changes to review.")
                print("No code changes to review.")
                return

            # Construct the prompt for code review
            prompt = (
                f"Please review the following code changes for best practices, security vulnerabilities, and optimization opportunities.\n\n"
                f"Changes:\n{json.dumps(changes, indent=2)}\n\n"
                "Provide your feedback in JSON format with 'file', 'issues', and 'suggestions' keys."
            )
            logging.debug(f"Code review prompt: {prompt}")

            # Generate the response using Llama 3
            response = self.llama3_client.generate(prompt, max_tokens=500, temperature=0.3)

            if not response:
                logging.error("Failed to review code changes.")
                print("Error: Failed to review code changes.")
                return

            # Parse the JSON response
            review_feedback = self.parse_feedback(response)
            self.memory.set('code_review_feedback', review_feedback)
            logging.info(f"Code review feedback: {review_feedback}")
            print(f"Code review feedback: {review_feedback}")

        except Exception as e:
            logging.error(f"Error in code review: {e}")
            print(f"Error: Failed to review code changes: {e}")

    def parse_feedback(self, response):
        """
        Parses the JSON response from the LLM into structured feedback.
        Expected response format:
        [
            {
                "file": "auth/login.py",
                "issues": ["Issue 1", "Issue 2"],
                "suggestions": ["Suggestion 1", "Suggestion 2"]
            },
            ...
        ]
        """
        try:
            feedback = json.loads(response)
            # Validate the structure
            for item in feedback:
                if not all(k in item for k in ("file", "issues", "suggestions")):
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
