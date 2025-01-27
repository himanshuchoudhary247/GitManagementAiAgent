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
from agents.repository_mapping_agent import RepositoryMappingAgent
from utils.llama3_client import Llama3Client
from utils.github_handler import GitHubHandler
from utils import config
from utils.centralized_memory import CentralizedMemory
from utils.plan_tracker import PlanTracker  # Import PlanTracker
from utils.cli_utils import print_with_breaker
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

        # Initialize Centralized Memory
        self.centralized_memory = CentralizedMemory(repo_path)

        # Initialize Plan Tracker
        self.plan_tracker = PlanTracker(repo_path)

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
            try:
                self.github_handler = GitHubHandler()
            except Exception as e:
                logging.error(f"GitHub Handler Initialization Error: {e}")
                self.github_handler = None
        else:
            self.github_handler = None
            if self.use_gitrepo:
                logging.warning("GitHub token or repository name not provided. GitHub integrations will be disabled.")
            else:
                logging.info("GitHub integrations are disabled as use_gitrepo is set to False.")

        # Initialize Agents with Centralized Memory
        self.repository_mapping_agent = RepositoryMappingAgent(self.centralized_memory, repo_path)
        self.query_understanding_agent = QueryUnderstandingAgent(self.llama3_client, self.centralized_memory)
        self.plan_agent = PlanAgent(self.llama3_client, self.centralized_memory)
        self.context_retrieval_agent = ContextRetrievalAgent(self.llama3_client, self.centralized_memory)
        self.intermediate_processing_agent = IntermediateProcessingAgent(self.llama3_client, self.centralized_memory)
        self.answer_generation_agent = AnswerGenerationAgent(self.llama3_client, self.centralized_memory)
        self.code_validation_agent = CodeValidationAgent(self.llama3_client, self.centralized_memory, repo_path)
        self.code_completer_agent = CodeCompleterAgent(self.llama3_client, self.centralized_memory, repo_path)
        self.code_writing_agent = CodeWritingAgent(repo_path, self.centralized_memory, use_gitrepo=self.use_gitrepo)
        self.self_reflection_agent = SelfReflectionAgent(self.llama3_client, self.centralized_memory)
        self.undo_agent = UndoAgent(repo_path, self.centralized_memory)

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

        # Step 0: Map the Repository
        self.repository_mapping_agent.execute()

        # Retrieve Repository Map from Centralized Memory
        repository_map = self.centralized_memory.get('RepositoryMappingAgent', 'repository_map', {})
        if not repository_map:
            logging.error("Failed to map the repository.")
            print("Error: Failed to map the repository.")
            return

        # Step 1: Understand the Query
        self.query_understanding_agent.execute(requirement)

        # Retrieve Objectives from Memory
        objectives = self.centralized_memory.get('QueryUnderstandingAgent', 'objectives', [])

        if not objectives:
            logging.error("No objectives parsed from the query.")
            print("Error: No objectives parsed from the query.")
            return

        # Step 2: Create Plan
        self.plan_agent.execute(objectives)

        # Retrieve Plan from Memory
        plan = self.centralized_memory.get('PlanAgent', 'plan', [])

        if not plan:
            logging.error("No plan was generated.")
            print("Error: No plan was generated.")
            return

        # Add the plan to PlanTracker
        plan_name = "Main Plan"
        sub_plans = [sub_plan.get('objective', '') for sub_plan in plan]
        self.plan_tracker.add_plan(plan_name, sub_plans)

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
            relevant_functions = self.centralized_memory.get('ContextRetrievalAgent', 'relevant_functions', [])

            # Enhance Context with Repository Map
            enhanced_context = {
                'relevant_functions': relevant_functions,
                'repository_map': repository_map
            }

            # Step 3.2: Intermediate Processing
            self.intermediate_processing_agent.execute([sub_objective], relevant_functions, self.repo_path)

            # Retrieve Additional Context from Memory
            additional_context = self.centralized_memory.get('IntermediateProcessingAgent', 'additional_context', [])

            # Step 3.3: Generate Answer (Code Changes)
            self.answer_generation_agent.execute([sub_objective], relevant_functions, additional_context, self.repo_path)

            # Step 3.4: Validate Code Completeness
            self.code_validation_agent.execute()

            # Retrieve Incomplete Functions from Memory
            incomplete_functions = self.centralized_memory.get('CodeValidationAgent', 'incomplete_functions', [])

            # Handle Incomplete Functions
            retry_count = 0
            max_retries = 3
            while incomplete_functions and retry_count < max_retries:
                logging.info(f"Attempt {retry_count + 1} to complete incomplete functions.")
                print_with_breaker(f"Attempt {retry_count + 1} to complete incomplete functions.")

                # Create Sub-Plan to Complete Incomplete Functions
                for func in incomplete_functions:
                    plan_name = f"Complete Function: {func['function']}"
                    sub_plan_objective = f"Complete the function '{func['function']}' in '{func['file']}'"
                    self.plan_tracker.add_plan(plan_name, [sub_plan_objective])

                    # Execute Sub-Plan
                    self.process_sub_plan(plan_name, sub_plan_objective)

                # Re-validate Code Completeness
                self.code_validation_agent.execute()
                incomplete_functions = self.centralized_memory.get('CodeValidationAgent', 'incomplete_functions', [])

                retry_count += 1

            if incomplete_functions:
                logging.warning(f"Failed to complete all functions after {max_retries} attempts.")
                print_with_breaker(f"Warning: Failed to complete all functions after {max_retries} attempts.")
            else:
                logging.info("All functions are complete.")
                print_with_breaker("All functions are complete.")

        # Step 4: Update README.md
        self.update_readme()

        # Step 5: Commit and Push Changes if GitHub is configured
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

    def process_sub_plan(self, plan_name, sub_plan_objective):
        """
        Processes a single sub-plan to complete a function.

        :param plan_name: Name of the plan.
        :param sub_plan_objective: Objective of the sub-plan.
        """
        print(f"Processing sub-plan: {sub_plan_objective}")
        logging.info(f"Processing sub-plan: {sub_plan_objective}")

        # Step 3.1: Retrieve Context
        self.context_retrieval_agent.execute([sub_plan_objective])

        # Retrieve Relevant Functions from Memory
        relevant_functions = self.centralized_memory.get('ContextRetrievalAgent', 'relevant_functions', [])

        # Enhance Context with Repository Map
        repository_map = self.centralized_memory.get('RepositoryMappingAgent', 'repository_map', {})
        enhanced_context = {
            'relevant_functions': relevant_functions,
            'repository_map': repository_map
        }

        # Step 3.2: Intermediate Processing
        self.intermediate_processing_agent.execute([sub_plan_objective], relevant_functions, self.repo_path)

        # Retrieve Additional Context from Memory
        additional_context = self.centralized_memory.get('IntermediateProcessingAgent', 'additional_context', [])

        # Step 3.3: Generate Answer (Code Changes)
        self.answer_generation_agent.execute([sub_plan_objective], relevant_functions, additional_context, self.repo_path)

        # Step 3.4: Validate Code Completeness
        self.code_validation_agent.execute()

        # Retrieve Incomplete Functions from Memory
        incomplete_functions = self.centralized_memory.get('CodeValidationAgent', 'incomplete_functions', [])

        # Handle Incomplete Functions
        if incomplete_functions:
            logging.info(f"Incomplete functions detected during sub-plan '{sub_plan_objective}' execution.")
            print_with_breaker(f"Incomplete functions detected during sub-plan '{sub_plan_objective}' execution.")
        else:
            logging.info(f"Sub-plan '{sub_plan_objective}' completed successfully.")
            print_with_breaker(f"Sub-plan '{sub_plan_objective}' completed successfully.")

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
                "- Introduced CodeValidationAgent and CodeCompleterAgent to ensure code completeness\n"
                "- Mapped repository structure for better context awareness\n"
                "- Added PlanTracker for tracking plans and sub-plans\n"
                "- Implemented user confirmation before writing to files\n"
                "- Enabled user feedback for code improvements\n\n"
                "*Auto-generated by the system*\n"
            )
            mode = 'a' if os.path.exists(readme_path) else 'w'
            with open(readme_path, mode) as f:
                f.write(readme_content + "\n")
            logging.info(f"README.md updated at {readme_path}")
            print_with_breaker(f"README.md updated at {readme_path}")
        except Exception as e:
            logging.error(f"Failed to update README.md: {e}")
            print_with_breaker(f"Error: Failed to update README.md: {e}")
