# agents/self_reflection_agent.py

import logging

class SelfReflectionAgent:
    def __init__(self, llama3_client, memory):
        self.llama3_client = llama3_client
        self.memory = memory
        self.max_retries = 2

    def reflect_on_changes(self, code_changes):
        if not code_changes:
            logging.info("No code changes to reflect on.")
            return
        attempt = 0
        reflection = ""
        while attempt < self.max_retries and not reflection:
            try:
                reflection_prompt = self.build_reflection_prompt(code_changes)
                logging.debug(f"SelfReflectionAgent Prompt (attempt {attempt+1}): {reflection_prompt}")
                response = self.llama3_client.generate(reflection_prompt, max_tokens=300, temperature=0.7)
                logging.debug(f"SelfReflectionAgent Raw Response (attempt {attempt+1}): {response}")
                if not response:
                    logging.warning("Received empty reflection response from LLM.")
                else:
                    reflection = self.parse_reflection(response)
            except Exception as e:
                logging.error(f"SelfReflectionAgent Error (attempt {attempt+1}): {e}")
            attempt += 1

        if reflection:
            self.memory.set('reflection', reflection)
            logging.info(f"Reflection: {reflection}")
            print(f"Reflection: {reflection}")
        else:
            logging.error("No reflection generated after fallback attempts.")
            print("Error: No reflection generated after fallback attempts.")

    def build_reflection_prompt(self, code_changes):
        changes_str = ""
        for change in code_changes:
            changes_str += f"Action: {change.get('action')}, File: {change.get('file')}, Code: {change.get('code')}\n\n"
        return (
            f"Here are the recent code changes:\n\n{changes_str}\n\n"
            "Reflect on whether these changes are sufficient to address the objectives. "
            "List any potential improvements, missing steps, or additional plans needed in plain text. "
            "Do not include any JSON."
        )

    def parse_reflection(self, response):
        try:
            # Assuming the response is plain text
            return response.strip()
        except Exception as e:
            logging.warning(f"SelfReflectionAgent parse error: {e}")
            return ""