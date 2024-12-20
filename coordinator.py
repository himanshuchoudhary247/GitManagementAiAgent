# coordinator.py

from agents.query_understanding_agent import QueryUnderstandingAgent
from agents.context_retrieval_agent import ContextRetrievalAgent
from agents.intermediate_processing_agent import IntermediateProcessingAgent
from agents.answer_generation_agent import AnswerGenerationAgent
from agents.code_writing_agent import CodeWritingAgent
from memory_node import MemoryNode
from utils.llama3_client import Llama3Client
from utils.github_handler import GitHubHandler
from utils.code_utils import find_function_definitions, extract_json_from_response
from utils import config  # Updated import
import logging
import sys
import os
import json

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
        self.context_retrieval_memory = MemoryNode("ContextRetrievalAgent")
        self.intermediate_processing_memory = MemoryNode("IntermediateProcessingAgent")
        self.answer_generation_memory = MemoryNode("AnswerGenerationAgent")
        self.code_writing_memory = MemoryNode("CodeWritingAgent")

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
        self.context_retrieval_agent = ContextRetrievalAgent(self.llama3_client, self.context_retrieval_memory)
        self.intermediate_processing_agent = IntermediateProcessingAgent(self.llama3_client, self.intermediate_processing_memory)
        self.answer_generation_agent = AnswerGenerationAgent(self.llama3_client, self.answer_generation_memory)
        self.code_writing_agent = CodeWritingAgent(repo_path, self.code_writing_memory, use_gitrepo=self.use_gitrepo)

        self.repo_path = repo_path

    def process_requirement(self, requirement):
        if not requirement:
            logging.error("No requirement provided to process.")
            print("Error: No requirement provided.")
            return

        print("Starting requirement processing...")
        logging.info("Starting requirement processing...")

        # Step 1: Understand the Query
        self.query_understanding_agent.understand_query(requirement)

        # Retrieve Objectives from Memory
        objectives = self.query_understanding_memory.get('objectives', [])

        if not objectives:
            logging.error("No objectives parsed from the query.")
            print("Error: No objectives parsed from the query.")
            return

        # Step 2: Retrieve Relevant Context from Repository Map
        # First, map the repository
        repo_map = self.map_repository()
        if not repo_map:
            logging.error("Failed to map the repository.")
            print("Error: Failed to map the repository.")
            return

        # Step 3: Retrieve Relevant Functions
        self.context_retrieval_agent.retrieve_context(objectives, repo_map)

        # Retrieve Relevant Functions from Memory
        relevant_functions = self.context_retrieval_memory.get('relevant_functions', [])

        if not relevant_functions:
            logging.error("No relevant functions retrieved.")
            print("Error: No relevant functions retrieved.")
            return

        # Step 4: Intermediate Processing
        self.intermediate_processing_agent.process(objectives, relevant_functions, self.repo_path)

        # Retrieve Additional Context from Memory
        additional_context = self.intermediate_processing_memory.get('additional_context', [])

        # Step 5: Generate Answer
        self.answer_generation_agent.generate_answer(objectives, relevant_functions, additional_context, self.repo_path)

        # Retrieve Code Changes from Memory
        code_changes = self.answer_generation_memory.get('code_changes', [])

        if not code_changes:
            logging.error("No code changes generated.")
            print("Error: No code changes generated.")
            return

        # Step 6: Write Code Changes
        self.code_writing_agent.write_code_changes(code_changes)

        # Optional: Commit changes and create pull request if GitHub is configured and use_gitrepo is True
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
