# coordinator.py

from agents.query_understanding_agent import QueryUnderstandingAgent
from agents.plan_agent import PlanAgent
from agents.context_retrieval_agent import ContextRetrievalAgent
from agents.intermediate_processing_agent import IntermediateProcessingAgent
from agents.answer_generation_agent import AnswerGenerationAgent
from agents.code_writing_agent import CodeWritingAgent
from agents.self_reflection_agent import SelfReflectionAgent
from agents.undo_agent import UndoAgent
from agents.code_validation_agent import CodeValidationAgent
from agents.code_completer_agent import CodeCompleterAgent
from memory_node import MemoryNode
from utils.llama3_client import Llama3Client
from utils.github_handler import GitHubHandler
from utils.code_utils import find_function_definitions, extract_json_from_response
from utils import config  # Updated import
import logging
import sys
import os

class Coordinator:
    def __init__(self, repo_path, use_gitrepo=False):
        # Initialize Logging
        logging.basicConfig(
            filename='agentic_rag_system.log',
            level=logging.DEBUG,  # Set to DEBUG for more detailed logs
            format='%(asctime)s:%(levelname)s:%(message)s'
        )

        # Initialize Memory Nodes for each Agent
        self.query_understanding_memory = MemoryNode("QueryUnderstandingAgent")
        self.plan_memory = MemoryNode("PlanAgent")
        self.context_retrieval_memory = MemoryNode("ContextRetrievalAgent")
        self.intermediate_processing_memory = MemoryNode("IntermediateProcessingAgent")
        self.answer_generation_memory = MemoryNode("AnswerGenerationAgent")
        self.code_writing_memory = MemoryNode("CodeWritingAgent")
        self.reflection_memory = MemoryNode("SelfReflectionAgent")
        self.code_validation_memory = MemoryNode("CodeValidationAgent")
        self.code_completer_memory = MemoryNode("CodeCompleterAgent")

        # Initialize Llama 3 Client
        try:
            self.llama3_client = Llama3Client()
        except ValueError as ve:
            logging.error(f"Initialization Error: {ve}")
            sys.exit(f"Initialization Error: {ve}")

        # Check write permissions for the repository path
        if not os.access(repo_path, os.W_OK):
            logging.error(f"No write permission for the directory: {repo_path}")
            print(f"Error: No write permission for the directory: {repo_path}")
            print("Please provide a directory with write permissions.")
            sys.exit(1)
        else:
            logging.info(f"Write permission confirmed for the directory: {repo_path}")

        self.use_gitrepo = use_gitrepo

        # Initialize GitHub Handler if GitHub interactions are enabled and use_gitrepo is True
        if self.use_gitrepo and config.GITHUB_TOKEN and config.GITHUB_REPO_NAME:
            self.github_handler = GitHubHandler()
        else:
            self.github_handler = None
            if self.use_gitrepo:
                logging.warning("GitHub token or repository name not provided. GitHub integrations will be disabled.")
            else:
                logging.info("GitHub integrations are disabled as use_gitrepo is set to False.")

        # Initialize Agents with their respective memories
        self.query_understanding_agent = QueryUnderstandingAgent(self.llama3_client, self.query_understanding_memory)
        self.plan_agent = PlanAgent(self.llama3_client, self.plan_memory)
        self.context_retrieval_agent = ContextRetrievalAgent(self.llama3_client, self.context_retrieval_memory)
        self.intermediate_processing_agent = IntermediateProcessingAgent(self.llama3_client, self.intermediate_processing_memory)
        self.answer_generation_agent = AnswerGenerationAgent(self.llama3_client, self.answer_generation_memory)
        self.code_writing_agent = CodeWritingAgent(repo_path, self.code_writing_memory, use_gitrepo=self.use_gitrepo)
        self.self_reflection_agent = SelfReflectionAgent(self.llama3_client, self.reflection_memory)
        self.undo_agent = UndoAgent(repo_path)
        self.code_validation_agent = CodeValidationAgent(self.code_validation_memory, repo_path)
        self.code_completer_agent = CodeCompleterAgent(self.llama3_client, self.code_completer_memory, repo_path)

        self.repo_path = repo_path

    def process_requirement(self, requirement):
        if not requirement:
            logging.error("No requirement provided to process.")
            print("Error: No requirement provided.")
            return

        if requirement.lower() == "undo changes":
            logging.info("Received request to undo changes.")
            print("Processing undo changes...")
            self.undo_agent.execute()
            return

        print("Starting requirement processing...")
        logging.info("Starting requirement processing...")

        # Step 1: Understand the Query
        self.query_understanding_agent.execute(requirement)

        # Retrieve Objectives from Memory
        objectives = self.query_understanding_memory.get('objectives', [])

        if not objectives:
            logging.error("No objectives parsed from the query.")
            print("Error: No objectives parsed from the query.")
            return

        # Step 2: Create Plan
        self.plan_agent.execute(objectives)

        # Retrieve Plan from Memory
        plan = self.plan_memory.get('plan', [])

        if not plan:
            logging.error("No plan was generated.")
            print("Error: No plan was generated.")
            return

        # Step 3: Execute Plan
        for sub_plan in plan:
            if not isinstance(sub_plan, dict):
                logging.warning(f"Expected sub_plan to be a dict, got {type(sub_plan)}. Skipping.")
                print(f"Warning: Expected sub_plan to be a dict, got {type(sub_plan)}. Skipping.")
                continue

            sub_objective = sub_plan.get('objective', '')
            tasks = sub_plan.get('tasks', [])
            if not sub_objective:
                logging.warning("Sub-plan missing 'objective'. Skipping.")
                print("Warning: Sub-plan missing 'objective'. Skipping.")
                continue

            print(f"Executing sub-objective: {sub_objective}")
            logging.info(f"Executing sub-objective: {sub_objective}")

            # Step 3.1: Retrieve Context
            self.context_retrieval_agent.execute([sub_objective])

            # Retrieve Relevant Functions from Memory
            relevant_functions = self.context_retrieval_memory.get('relevant_functions', [])

            # Step 3.2: Intermediate Processing
            self.intermediate_processing_agent.execute([sub_objective], relevant_functions, self.repo_path)

            # Retrieve Additional Context from Memory
            additional_context = self.intermediate_processing_memory.get('additional_context', [])

            # Step 3.3: Generate Answer (Code Changes)
            self.answer_generation_agent.execute([sub_objective], relevant_functions, additional_context, self.repo_path)

            # Retrieve Code Changes from Memory
            code_changes = self.answer_generation_memory.get('code_changes', [])

            if not code_changes:
                logging.warning(f"No code changes generated for sub-objective: {sub_objective}")
                print(f"Warning: No code changes generated for sub-objective: {sub_objective}")
                continue

            # Step 3.4: Write Code Changes
            self.code_writing_agent.execute(code_changes)

            # Step 3.5: Self Reflection
            self.self_reflection_agent.execute(code_changes)

        # Step 4: Validate Code for Completeness
        self.code_validation_agent.execute()

        # Retrieve Incomplete Functions from Memory
        incomplete_functions = self.code_validation_memory.get('incomplete_functions', [])

        if incomplete_functions:
            # Step 5: Complete Incomplete Functions
            self.code_completer_agent.execute()
        else:
            logging.info("All functions are complete.")

        # Step 6: Update README.md
        self.update_readme()

        # Step 7: Commit and Push Changes if GitHub is configured
        if self.use_gitrepo and self.github_handler:
            commit_message = f"Update based on requirement: {requirement}"
            try:
                self.github_handler.commit_changes(
                    branch_name='auto-update-branch',
                    commit_message=commit_message
                )
                self.github_handler.create_pull_request(
                    title="Automated Update",
                    body=f"This pull request was automatically generated based on the requirement: {requirement}",
                    head='auto-update-branch',
                    base='main'
                )
            except Exception as e:
                logging.error(f"GitHub Integration Error: {e}")
                print(f"Error during GitHub operations: {e}")

        print("Requirement processing completed successfully.")
        logging.info("Requirement processing completed successfully.")

    def update_readme(self):
        """
        Attempts to create or update a 'README.md' file in the repo_path
        describing the latest enhancements.
        """
        try:
            readme_path = os.path.join(self.repo_path, 'README.md')
            readme_content = (
                "# Agentic RAG System Updates\n\n"
                "## Recent Enhancements\n\n"
                "- Implemented BaseAgent class for all agents\n"
                "- Added code change tracking and undo functionality\n"
                "- Improved logging and error handling\n"
                "- Introduced CodeValidationAgent and CodeCompleterAgent to ensure code completeness\n\n"
                "*Auto-generated by the system*\n"
            )
            mode = 'a' if os.path.exists(readme_path) else 'w'
            with open(readme_path, mode) as f:
                f.write(readme_content + "\n")
            logging.info(f"README.md updated at {readme_path}")
            print(f"README.md updated at {readme_path}")
        except Exception as e:
            logging.error(f"Failed to update README.md: {e}")
            print(f"Error: Failed to update README.md: {e}")

    def map_repository(self):
        """
        Maps the repository by scanning all Python files and extracting their functions.
        Returns a dictionary with file paths as keys and lists of function names as values.
        """
        try:
            import ast

            repo_map = {}
            for root, dirs, files in os.walk(self.repo_path):
                for file in files:
                    if file.endswith('.py'):
                        file_path = os.path.join(root, file)
                        try:
                            with open(file_path, 'r') as f:
                                tree = ast.parse(f.read(), filename=file_path)
                            functions = [node.name for node in ast.walk(tree) if isinstance(node, ast.FunctionDef)]
                            relative_path = os.path.relpath(file_path, self.repo_path)
                            repo_map[relative_path] = functions
                            logging.info(f"Mapped {len(functions)} functions in '{relative_path}'.")
                        except Exception as e:
                            logging.error(f"Failed to parse '{file_path}': {e}")
            self.code_writing_agent.memory.set('repo_map', repo_map)
            logging.info("Repository mapping completed.")
            print("Repository mapping completed.")
            return repo_map
        except Exception as e:
            logging.error(f"Error in mapping repository: {e}")
            print(f"Error: Failed to map the repository: {e}")
            return {}

# from agents.query_understanding_agent import QueryUnderstandingAgent
# from agents.plan_agent import PlanAgent
# from agents.context_retrieval_agent import ContextRetrievalAgent
# from agents.intermediate_processing_agent import IntermediateProcessingAgent
# from agents.answer_generation_agent import AnswerGenerationAgent
# from agents.code_writing_agent import CodeWritingAgent
# from agents.self_reflection_agent import SelfReflectionAgent
# from agents.undo_agent import UndoAgent
# from agents.project_initialization_agent import ProjectInitializationAgent
# from agents.execution_agent import ExecutionAgent
# from agents.directory_structuring_agent import DirectoryStructuringAgent
# from agents.file_manager import FileManager
# from agents.json_correction_agent import JSONCorrectionAgent
# from agents.error_correction_agent import ErrorCorrectionAgent  # New Import
# from memory_node import MemoryNode
# from utils.llama3_client import Llama3Client
# from utils.github_handler import GitHubHandler
# from utils.code_utils import find_function_definitions, extract_json_from_response
# from utils.cli_utils import print_with_breaker
# from utils import config
# import logging
# import sys
# import os
# import json

# class Coordinator:
#     def __init__(self, repo_path: str, use_gitrepo: bool = False):
#         # Initialize Logging
#         logging.basicConfig(
#             filename='agentic_rag_system.log',
#             level=logging.DEBUG,  # Set to DEBUG for more detailed logs
#             format='%(asctime)s:%(levelname)s:%(message)s'
#         )

#         # Initialize Memory Nodes for each Agent
#         self.query_understanding_memory = MemoryNode("QueryUnderstandingAgent")
#         self.plan_memory = MemoryNode("PlanAgent")
#         self.context_retrieval_memory = MemoryNode("ContextRetrievalAgent")
#         self.intermediate_processing_memory = MemoryNode("IntermediateProcessingAgent")
#         self.answer_generation_memory = MemoryNode("AnswerGenerationAgent")
#         self.code_writing_memory = MemoryNode("CodeWritingAgent")
#         self.reflection_memory = MemoryNode("SelfReflectionAgent")

#         # Initialize Llama 3 Client
#         try:
#             self.llama3_client = Llama3Client()
#         except ValueError as ve:
#             logging.error(f"Initialization Error: {ve}")
#             sys.exit(f"Initialization Error: {ve}")

#         # Check write permissions for the repository path
#         if not os.access(repo_path, os.W_OK):
#             logging.error(f"No write permission for the directory: {repo_path}")
#             print_with_breaker(f"Error: No write permission for the directory: {repo_path}")
#             print_with_breaker("Please provide a directory with write permissions.")
#             sys.exit(1)
#         else:
#             logging.info(f"Write permission confirmed for the directory: {repo_path}")

#         self.use_gitrepo = use_gitrepo

#         # Initialize GitHub Handler if GitHub interactions are enabled and use_gitrepo is True
#         if self.use_gitrepo and config.GITHUB_TOKEN and config.GITHUB_REPO_NAME:
#             self.github_handler = GitHubHandler()
#         else:
#             self.github_handler = None
#             if self.use_gitrepo:
#                 logging.warning("GitHub token or repository name not provided. GitHub integrations will be disabled.")
#                 print_with_breaker("Warning: GitHub token or repository name not provided. GitHub integrations will be disabled.")
#             else:
#                 logging.info("GitHub integrations are disabled as use_gitrepo is set to False.")

#         # Initialize Agents with their respective memories
#         self.error_correction_agent = ErrorCorrectionAgent(self.llama3_client)
#         self.query_understanding_agent = QueryUnderstandingAgent(self.llama3_client, self.query_understanding_memory)
#         self.plan_agent = PlanAgent(self.llama3_client, self.plan_memory)
#         self.context_retrieval_agent = ContextRetrievalAgent(self.llama3_client, self.context_retrieval_memory)
#         self.intermediate_processing_agent = IntermediateProcessingAgent(self.llama3_client, self.intermediate_processing_memory)
#         self.answer_generation_agent = AnswerGenerationAgent(self.llama3_client, self.answer_generation_memory)
#         self.code_writing_agent = CodeWritingAgent(repo_path, self.code_writing_memory, self.error_correction_agent, use_gitrepo=self.use_gitrepo)
#         self.self_reflection_agent = SelfReflectionAgent(self.llama3_client, self.reflection_memory)
#         self.undo_agent = UndoAgent(repo_path)
#         self.project_initialization_agent = ProjectInitializationAgent(repo_path)
#         self.error_correction_agent = ErrorCorrectionAgent(self.llama3_client)  # Initialize ErrorCorrectionAgent
#         self.execution_agent = ExecutionAgent(self.error_correction_agent)  # Pass ErrorCorrectionAgent to ExecutionAgent
#         self.directory_structuring_agent = DirectoryStructuringAgent(self.llama3_client, repo_path)
#         self.file_manager = FileManager(repo_path)
#         self.json_correction_agent = JSONCorrectionAgent(self.llama3_client)

#         self.repo_path = repo_path

#     def process_requirement(self, requirement: str):
#         if not requirement:
#             logging.error("No requirement provided to process.")
#             print_with_breaker("Error: No requirement provided.")
#             return

#         # Check if the command is to undo changes
#         if requirement.lower() == "undo changes":
#             logging.info("Received request to undo changes.")
#             print_with_breaker("Processing undo changes...")
#             self.undo_agent.undo_changes()
#             return

#         # Check if the command is to restructure directories
#         if requirement.lower().startswith("restructure directories"):
#             # Extract any specific instructions if provided
#             user_query = requirement[len("restructure directories"):].strip() or "Optimize the directory structure for better maintainability."
#             logging.info(f"Received directory restructuring request: {user_query}")
#             print_with_breaker("Processing directory restructuring...")
#             try:
#                 # Invoke DirectoryStructuringAgent
#                 proposed_structure = self.directory_structuring_agent.propose_restructuring(user_query)
#                 if not proposed_structure:
#                     logging.error("Directory restructuring proposal failed.")
#                     print_with_breaker("Error: Failed to propose a new directory structure.")
#                     return

#                 # Execute restructuring using FileManager
#                 self.file_manager.arrange_files_dynamic(proposed_structure)

#                 # Log the restructuring action
#                 logging.info("Directory restructuring completed successfully.")
#                 print_with_breaker("Directory restructuring completed successfully.")

#             except Exception as e:
#                 logging.error(f"Error during directory restructuring workflow: {e}")
#                 print_with_breaker(f"Error: Directory restructuring failed: {e}")
#             return

#         # Initialize project if needed
#         self.project_initialization_agent.initialize_project()

#         print_with_breaker("Starting requirement processing...")
#         logging.info("Starting requirement processing...")

#         try:
#             # Step 1: Understand the Query
#             self.query_understanding_agent.understand_query(requirement)

#             # Retrieve Objectives from Memory
#             objectives = self.query_understanding_memory.get('objectives', [])

#             if not objectives:
#                 logging.error("No objectives parsed from the query.")
#                 print_with_breaker("Error: No objectives parsed from the query.")
#                 return

#             # Step 2: Create Plan
#             plan_response = self.plan_agent.create_plan(objectives)

#             # Attempt to extract Plan from Memory
#             plan = self.plan_memory.get('plan', [])

#             if not plan:
#                 logging.error("No plan was generated.")
#                 print_with_breaker("Error: No plan was generated.")

#                 # Attempt to correct the malformed JSON from PlanAgent
#                 logging.info("Attempting to correct the malformed JSON from PlanAgent.")
#                 corrected_json = self.json_correction_agent.correct_json(plan_response)
#                 if corrected_json and isinstance(corrected_json, dict):
#                     corrected_plan = corrected_json.get('plan', [])
#                     if corrected_plan:
#                         self.plan_memory.set('plan', corrected_plan)
#                         logging.info(f"Corrected and extracted plan: {corrected_plan}")
#                         print_with_breaker(f"Corrected and extracted plan: {corrected_plan}")
#                         plan = corrected_plan
#                     else:
#                         logging.error("Corrected JSON does not contain 'plan'.")
#                         print_with_breaker("Error: Corrected JSON does not contain 'plan'.")
#                         return
#                 else:
#                     logging.error("Failed to correct the malformed JSON from PlanAgent.")
#                     print_with_breaker("Error: Failed to correct the malformed JSON from PlanAgent.")
#                     return

#             # Execute the plan
#             for sub_plan in plan:
#                 self.execute_sub_plan(sub_plan)

#             # Step 4: Update README.md
#             self.update_readme()

#             # Step 5: Commit and Push Changes if GitHub is configured
#             if self.use_gitrepo and self.github_handler:
#                 commit_message = f"Update based on requirement: {requirement}"
#                 try:
#                     self.github_handler.commit_changes(
#                         branch_name='auto-update-branch',
#                         commit_message=commit_message
#                     )
#                     self.github_handler.create_pull_request(
#                         title="Automated Update",
#                         body=f"This pull request was automatically generated based on the requirement: {requirement}",
#                         head='auto-update-branch',
#                         base='main'
#                     )
#                 except Exception as e:
#                     logging.error(f"GitHub Integration Error: {e}")
#                     print_with_breaker(f"Error during GitHub operations: {e}")

#             print_with_breaker("Requirement processing completed successfully.")
#             logging.info("Requirement processing completed successfully.")

#         except Exception as e:
#             logging.error(f"Unexpected error during requirement processing: {e}")
#             print_with_breaker(f"Error: An unexpected error occurred: {e}")

#     def rephrase_and_split_query(self, requirement: str) -> list:
#         """
#         Rephrase the original query and split it into multiple sub-queries.
#         """
#         try:
#             prompt = (
#                 f"Original Query: {requirement}\n\n"
#                 "Rephrase the above query to clarify its intent and split it into multiple distinct sub-queries. "
#                 "Respond **only** in valid JSON format with a key `sub_queries` containing a list of sub-queries. "
#                 "No extra text."
#             )
#             logging.debug(f"Coordinator Rephrase Prompt: {prompt}")
#             response = self.llama3_client.generate(prompt, max_tokens=300, temperature=0.5)

#             logging.debug(f"Coordinator Rephrase Response: {response}")

#             sub_queries_json = extract_json_from_response(response)
#             if not sub_queries_json:
#                 # Attempt to correct the JSON
#                 logging.warning("Failed to extract sub_queries, attempting to correct JSON.")
#                 print_with_breaker("Warning: Failed to extract sub_queries, attempting to correct JSON.")
#                 sub_queries_json = self.json_correction_agent.correct_json(response)
#                 if not sub_queries_json:
#                     logging.error("Failed to extract and correct sub_queries from the response.")
#                     print_with_breaker("Error: Failed to extract and correct sub_queries from the response.")
#                     return []

#             if sub_queries_json and isinstance(sub_queries_json, dict):
#                 sub_queries = sub_queries_json.get('sub_queries', [])
#                 logging.info(f"Rephrased and split queries: {sub_queries}")
#                 print_with_breaker(f"Rephrased and split queries: {sub_queries}")
#                 return sub_queries
#             else:
#                 logging.warning("Failed to extract sub_queries from the response.")
#                 print_with_breaker("Warning: Failed to extract sub_queries from the response.")
#                 return []
#         except Exception as e:
#             logging.error(f"Error in rephrasing and splitting query: {e}")
#             print_with_breaker(f"Error: Failed to rephrase and split query: {e}")
#             return []

#     def execute_sub_plan(self, sub_plan: dict):
#         if not isinstance(sub_plan, dict):
#             logging.warning(f"Expected sub_plan to be a dict, got {type(sub_plan)}. Skipping.")
#             print_with_breaker(f"Warning: Expected sub_plan to be a dict, got {type(sub_plan)}. Skipping.")
#             return

#         sub_objective = sub_plan.get('objective', '')
#         tasks = sub_plan.get('tasks', [])
#         if not sub_objective:
#             logging.warning("Sub-objective is missing. Skipping.")
#             print_with_breaker("Warning: Sub-objective is missing. Skipping.")
#             return

#         print_with_breaker(f"Executing sub-objective: {sub_objective}")
#         logging.info(f"Executing sub-objective: {sub_objective}")

#         try:
#             # Step 3.1: Retrieve Context
#             self.context_retrieval_agent.retrieve_context([sub_objective], {})

#             # Retrieve Relevant Functions from Memory
#             relevant_functions = self.context_retrieval_memory.get('relevant_functions', [])

#             # Step 3.2: Intermediate Processing
#             self.intermediate_processing_agent.process([sub_objective], relevant_functions, self.repo_path)

#             # Retrieve Additional Context from Memory
#             additional_context = self.intermediate_processing_memory.get('additional_context', [])

#             # Step 3.3: Generate Answer (Code Changes)
#             self.answer_generation_agent.generate_answer([sub_objective], relevant_functions, additional_context, self.repo_path)

#             # Retrieve Code Changes from Memory
#             code_changes = self.answer_generation_memory.get('code_changes', [])

#             if not code_changes:
#                 logging.warning(f"No code changes generated for sub-objective: {sub_objective}")
#                 print_with_breaker(f"Warning: No code changes generated for sub-objective: {sub_objective}")
#                 return

#             # Step 3.4: Write Code Changes
#             self.code_writing_agent.write_code_changes(code_changes)

#             # Step 3.5: Self Reflection
#             self.self_reflection_agent.reflect_on_changes(code_changes)

#             # Step 3.6: Execute Code Changes Safely
#             for change in code_changes:
#                 action = change.get('action')
#                 file = change.get('file')
#                 code = change.get('code')
#                 if action and file and code:
#                     self.execution_agent.execute_code(code, explanation=f"Executing {action} on {file}")
#                 else:
#                     logging.warning(f"Incomplete change information: {change}")
#                     print_with_breaker(f"Warning: Incomplete change information: {change}")

#         except Exception as e:
#             logging.error(f"Error executing sub_plan: {e}")
#             print_with_breaker(f"Error: Failed to execute sub-plan: {e}")

#     def update_readme(self):
#         """
#         Attempts to create or update a 'README.md' file in the repo_path
#         describing the latest enhancements.
#         """
#         try:
#             readme_path = os.path.join(self.repo_path, 'README.md')
#             readme_content = (
#                 "# Agentic RAG System Updates\n\n"
#                 "## New Features\n\n"
#                 "1. **Project Initialization:** Automatically creates a standard directory structure for new projects.\n"
#                 "2. **Enhanced Plan Generation:** Rephrases and splits complex queries to ensure comprehensive planning.\n"
#                 "3. **Safe Code Execution:** Executes system-level code changes only after user confirmation.\n"
#                 "4. **Improved Logging:** Adds breaker lines between log messages for better readability.\n\n"
#                 "## Complex Query Handling with Multiple Sub-Plans\n\n"
#                 "- Fallback logic in all agents\n"
#                 "- Sub-plan generation and reflection steps\n"
#                 "- Capability to create or update files as needed\n"
#                 "- Conditional Git usage\n\n"
#                 "## Testing Enhancements\n\n"
#                 "1. **Directory Initialization:** Verified the creation of the directory structure.\n"
#                 "2. **Plan Generation:** Ensured complex queries are rephrased and split into actionable sub-plans.\n"
#                 "3. **Code Execution Confirmation:** Implemented prompts for user approval before executing critical code changes.\n"
#                 "4. **Undo Functionality:** Enabled reverting changes accurately using the undo feature.\n\n"
#                 "## Security Enhancements\n\n"
#                 "1. **Execution Safety:** Restricted the execution environment to prevent unauthorized operations.\n\n"
#                 "## Error Handling Improvements\n\n"
#                 "1. **Robustness:** Enhanced error handling to manage edge cases and provide informative feedback.\n\n"
#                 "## User Guide\n\n"
#                 "For detailed instructions on using the Agentic RAG System, refer to the [User Guide](docs/user_guide.md).\n\n"
#                 "*Auto-generated by the system*\n"
#             )
#             mode = 'a' if os.path.exists(readme_path) else 'w'
#             with open(readme_path, mode) as f:
#                 f.write(readme_content + "\n")
#             logging.info(f"README.md updated at {readme_path}")
#             print_with_breaker(f"README.md updated at {readme_path}")
#         except Exception as e:
#             logging.error(f"Failed to update README.md: {e}")
#             print_with_breaker(f"Error: Failed to update README.md: {e}")
