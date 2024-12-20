# coordinator.py

from agents.repo_mapper_agent import RepoMapperAgent
from agents.query_interpreter_agent import QueryInterpreterAgent
from agents.retrieval_agent import RetrievalAgent
from agents.file_selector_agent import FileSelectorAgent
from agents.function_updater_agent import FunctionUpdaterAgent
from agents.function_adder_agent import FunctionAdderAgent
from agents.code_reviewer_agent import CodeReviewerAgent
from agents.documentation_updater_agent import DocumentationUpdaterAgent
from agents.testing_agent import TestingAgent
from agents.code_formatter_agent import CodeFormatterAgent
from agents.code_refactoring_agent import CodeRefactoringAgent
from agents.code_security_auditor_agent import CodeSecurityAuditorAgent
from agents.code_tester_agent import CodeTesterAgent
from memory_node import MemoryNode
from utils.llama3_client import Llama3Client
import config
import logging
import sys
import os
import json

class Coordinator:
    def __init__(self, repo_path):
        # Initialize Logging
        logging.basicConfig(
            filename='agentic_rag_system.log',
            level=logging.DEBUG,  # Set to DEBUG for more detailed logs
            format='%(asctime)s:%(levelname)s:%(message)s'
        )

        # Initialize Memory Nodes for each Agent
        self.repo_mapper_memory = MemoryNode("RepoMapperAgent")
        self.query_interpreter_memory = MemoryNode("QueryInterpreterAgent")
        self.retrieval_memory = MemoryNode("RetrievalAgent")
        self.file_selector_memory = MemoryNode("FileSelectorAgent")
        self.function_updater_memory = MemoryNode("FunctionUpdaterAgent")
        self.function_adder_memory = MemoryNode("FunctionAdderAgent")
        self.code_reviewer_memory = MemoryNode("CodeReviewerAgent")
        self.documentation_updater_memory = MemoryNode("DocumentationUpdaterAgent")
        self.testing_memory = MemoryNode("TestingAgent")
        self.code_formatter_memory = MemoryNode("CodeFormatterAgent")
        self.code_refactoring_memory = MemoryNode("CodeRefactoringAgent")
        self.security_auditor_memory = MemoryNode("CodeSecurityAuditorAgent")
        self.code_tester_memory = MemoryNode("CodeTesterAgent")

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

        # Initialize GitHub Handler if GitHub interactions are enabled
        if config.GITHUB_TOKEN and config.GITHUB_REPO_NAME:
            from utils.github_handler import GitHubHandler
            self.github_handler = GitHubHandler()
        else:
            self.github_handler = None
            logging.warning("GitHub token or repository name not provided. GitHub integrations will be disabled.")

        # Initialize Agents with their respective memories
        self.repo_mapper = RepoMapperAgent(repo_path, self.repo_mapper_memory)
        self.query_interpreter = QueryInterpreterAgent(self.llama3_client, self.query_interpreter_memory)
        self.retrieval_agent = RetrievalAgent(self.llama3_client, self.retrieval_memory)
        self.file_selector = FileSelectorAgent(self.file_selector_memory)
        self.function_updater = FunctionUpdaterAgent(self.llama3_client, repo_path, self.function_updater_memory)
        self.function_adder = FunctionAdderAgent(self.llama3_client, repo_path, self.function_adder_memory)
        self.code_reviewer = CodeReviewerAgent(self.llama3_client, self.code_reviewer_memory)
        self.documentation_updater = DocumentationUpdaterAgent(self.llama3_client, repo_path, self.documentation_updater_memory)
        self.testing_agent = TestingAgent(self.llama3_client,repo_path, self.testing_memory)
        self.code_formatter = CodeFormatterAgent(repo_path, self.code_formatter_memory)
        self.code_refactoring = CodeRefactoringAgent(self.llama3_client, repo_path, self.code_refactoring_memory)
        self.security_auditor = CodeSecurityAuditorAgent(self.llama3_client, repo_path, self.security_auditor_memory)
        self.code_tester = CodeTesterAgent(repo_path, self.code_tester_memory)

    def process_requirement(self, requirement):
        if not requirement:
            logging.error("No requirement provided to process.")
            print("Error: No requirement provided.")
            return

        # Store user requirement in memory for later use
        self.query_interpreter_memory.set('user_requirement', requirement)

        # Step 1: Map Repository
        self.repo_mapper.map_repo()

        # Step 2: Interpret Requirement with Context
        context = self.repo_mapper.get_repo_map()
        self.query_interpreter.interpret_requirement(requirement, context)

        # Step 3: Retrieve Relevant Functions using RAG
        self.retrieval_agent.retrieve_relevant_functions(requirement, context)

        # Step 4: Select Relevant Files based on Query and Context
        self.file_selector.select_files()

        # Step 5: Update Existing Functions with Context and Query
        self.function_updater.update_functions()

        # Step 6: Add New Functions with Context and Query
        self.function_adder.add_functions()

        # Step 7: Code Refactoring
        self.code_refactoring.refactor_code()

        # Step 8: Code Security Auditing
        self.security_auditor.audit_security()

        # Step 9: Code Review
        self.code_reviewer.review_code()

        # Step 10: Testing
        self.testing_agent.run_tests()

        # Step 11: Code Formatting
        self.code_formatter.format_code()

        # Step 12: Documentation Update
        self.documentation_updater.update_documentation()

        # Step 13: Running Tests After Changes
        self.code_tester.run_tests()

        # Optional: Iterate if necessary
        self.iterate_agents()

        # Optional: Commit changes and create pull request if GitHub is configured
        if self.github_handler:
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

    def iterate_agents(self):
        """
        Iterate through agents multiple times to refine and complete tasks.
        This method can be enhanced based on specific iteration logic.
        """
        max_iterations = 3
        for iteration in range(max_iterations):
            logging.info(f"Starting iteration {iteration + 1} of agent processing.")
            print(f"Starting iteration {iteration + 1} of agent processing.")

            # Example: Re-interpret requirement with updated context
            requirement = self.query_interpreter_memory.get('user_requirement')
            context = self.repo_mapper.get_repo_map()
            self.query_interpreter.interpret_requirement(requirement, context)

            # Example: Re-retrieve relevant functions based on updated query
            self.retrieval_agent.retrieve_relevant_functions(requirement, context)

            # Example: Re-select files based on updated query
            self.file_selector.select_files()

            # Example: Update functions again if needed
            self.function_updater.update_functions()

            # Example: Add functions again if needed
            self.function_adder.add_functions()

            # Example: Refactor code again if needed
            self.code_refactoring.refactor_code()

            # Check if further iterations are needed based on some condition
            # For simplicity, we'll break after two iterations
            if iteration >= 1:
                break

        logging.info("Agent iterations completed.")
        print("Agent iterations completed.")
