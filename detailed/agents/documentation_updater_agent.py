# agents/documentation_updater_agent.py

import os
import logging
import json

class DocumentationUpdaterAgent:
    def __init__(self, llama3_client, repo_path, memory):
        self.llama3_client = llama3_client
        self.repo_path = repo_path
        self.memory = memory

    def update_documentation(self):
        try:
            # Retrieve all changes from memory
            changes = self.memory.get('code_changes', {})
            if not changes:
                logging.info("No code changes to update documentation.")
                print("No code changes to update documentation.")
                return

            # Construct the prompt for documentation update
            prompt = (
                f"Based on the following code changes, update the project documentation to reflect these changes.\n\n"
                f"Changes:\n{json.dumps(changes, indent=2)}\n\n"
                "Provide the updated documentation in Markdown format."
            )
            logging.debug(f"Documentation update prompt: {prompt}")

            # Generate the response using Llama 3
            updated_docs = self.llama3_client.generate(prompt, max_tokens=1000, temperature=0.3)

            if not updated_docs:
                logging.error("Failed to update documentation.")
                print("Error: Failed to update documentation.")
                return

            # Write the updated documentation to README.md
            readme_path = os.path.join(self.repo_path, 'README.md')
            with open(readme_path, 'w') as f:
                f.write(updated_docs)

            logging.info("Documentation updated successfully.")
            print("Documentation updated successfully.")

        except PermissionError:
            logging.error(f"Permission denied when trying to write to 'README.md'.")
            print(f"Error: Permission denied when trying to write to 'README.md'. Please check your write permissions.")
        except Exception as e:
            logging.error(f"Unexpected error while updating documentation: {e}")
            print(f"Error: Unexpected error while updating documentation: {e}")
