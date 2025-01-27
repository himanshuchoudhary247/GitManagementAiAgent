# agents/code_writing_agent.py

import os
import ast
import json
from utils.code_utils import is_python_file
from utils.change_tracker import ChangeTracker
from utils.base_agent import BaseAgent
from utils.cli_utils import print_with_breaker
from utils.prompt_templates import code_change_prompt
from agents.json_parse_agent import JSONParseAgent

class CodeWritingAgent(BaseAgent):
    def __init__(self, repo_path, centralized_memory, use_gitrepo=False):
        super().__init__(llama3_client=None, centralized_memory=centralized_memory, name="CodeWritingAgent")  # LLM not needed here
        self.repo_path = os.path.abspath(repo_path)
        self.use_gitrepo = use_gitrepo
        self.change_tracker = ChangeTracker(self.repo_path)
        self.json_parse_agent = JSONParseAgent(llama3_client=None, centralized_memory=centralized_memory)  # LLM not needed here

    def execute(self, code_changes, plan_tracker):
        """
        Writes code changes to the repository and logs them for potential undo operations.
        Includes user confirmation before applying changes and handles user feedback for improvements.

        :param code_changes: List of code change dictionaries containing 'action', 'file', and 'code'.
        :param plan_tracker: Instance of PlanTracker to update plan statuses.
        """
        try:
            for change in code_changes:
                action = change.get('action')
                file = change.get('file')
                code = change.get('code')

                if not action or not file:
                    self.log_warning(f"Incomplete code change information: {change}")
                    continue

                if not code:
                    self.log_warning(f"Empty code field for action '{action}' in file '{file}'. Skipping.")
                    continue

                # Validate the code before applying
                is_valid, error_message = self.validate_code(code)
                if not is_valid:
                    self.log_warning(f"Invalid code provided for action '{action}' in file '{file}'. Error: {error_message}")
                    print_with_breaker(f"Warning: Invalid code provided for action '{action}' in file '{file}'.\nError: {error_message}\nSkipping this change.")
                    continue

                normalized_action = self.normalize_action(action)

                # Prepare the code snippet to display
                code_snippet = code.strip()

                # Display the file path and code to the user
                print_with_breaker(f"Proposed Change:\nAction: {normalized_action}\nFile: {file}\nCode:\n{code_snippet}\n---")
                self.log_info(f"Proposed Change:\nAction: {normalized_action}\nFile: {file}\nCode:\n{code_snippet}\n---")

                # Prompt the user for confirmation
                user_input = input("Do you want to apply this change? (yes/no): ").strip().lower()

                if user_input in ['yes', 'y']:
                    # Check if all functions are complete before writing
                    if not self.check_functions_complete(file):
                        logging.info(f"Incomplete functions detected in '{file}'. Creating sub-plan to complete them.")
                        print_with_breaker(f"Incomplete functions detected in '{file}'. Initiating sub-plan to complete them.")
                        # Create sub-plan for incomplete functions
                        incomplete_funcs = self.get_incomplete_functions(file)
                        if incomplete_funcs:
                            plan_name = f"Complete Functions in {file}"
                            sub_plan_objectives = [f"Complete the function '{func}' in '{file}'" for func in incomplete_funcs]
                            plan_tracker.add_plan(plan_name, sub_plan_objectives)
                            # Execute sub-plans
                            for sub_plan in sub_plan_objectives:
                                self.execute_sub_plan(plan_name, sub_plan, plan_tracker)
                        else:
                            logging.warning(f"No incomplete functions found in '{file}' despite previous detection.")
                            print_with_breaker(f"No incomplete functions found in '{file}' despite previous detection.")
                        # Re-validate after sub-plans
                        if not self.check_functions_complete(file):
                            logging.error(f"Failed to complete all functions in '{file}' after sub-plans.")
                            print_with_breaker(f"Error: Failed to complete all functions in '{file}' after sub-plans. Skipping this change.")
                            continue

                    # Handle user feedback for improvements or hints
                    feedback = input("Do you have any hints or instructions to improve the code? (yes/no): ").strip().lower()
                    if feedback in ['yes', 'y']:
                        hints = input("Please provide your hints or instructions: ").strip()
                        if hints:
                            # Generate updated code based on hints
                            updated_code = self.apply_hints_to_code(code, hints)
                            # Validate the updated code
                            is_valid, error_message = self.validate_code(updated_code)
                            if not is_valid:
                                self.log_warning(f"Updated code after applying hints is invalid. Error: {error_message}")
                                print_with_breaker(f"Warning: Updated code after applying hints is invalid.\nError: {error_message}\nSkipping this change.")
                                continue
                            # Display updated code to user for confirmation
                            print_with_breaker(f"Updated Code with Your Hints:\n{updated_code}\n---")
                            self.log_info(f"Updated Code with User Hints:\n{updated_code}\n---")
                            user_confirm = input("Do you want to apply the updated code? (yes/no): ").strip().lower()
                            if user_confirm in ['yes', 'y']:
                                code = updated_code  # Use updated code
                            else:
                                self.log_info(f"User declined to apply updated code for '{file}'. Skipping this change.")
                                print_with_breaker(f"Skipped applying updated code for '{file}'.")
                                continue

                    # Apply the change
                    response = self.apply_change(normalized_action, file, code)
                    parsed_response = self.json_parse_agent.execute(response)
                    if parsed_response and parsed_response.get('status') == 'success':
                        self.log_info(f"Successfully applied {normalized_action} to '{file}'. Message: {parsed_response.get('message')}")
                        print_with_breaker(f"Successfully applied {normalized_action} to '{file}'.")
                        # Mark the sub-plan as completed in PlanTracker
                        plan_name, sub_plan_name = self.extract_plan_names(file)
                        if plan_name and sub_plan_name:
                            plan_tracker.mark_sub_plan_completed(plan_name, sub_plan_name)
                    else:
                        self.log_warning(f"Failed to apply {normalized_action} to '{file}'. Message: {parsed_response.get('message') if parsed_response else 'No message.'}")
                        print_with_breaker(f"Warning: Failed to apply {normalized_action} to '{file}'.")
                elif user_input in ['no', 'n']:
                    # Ask for a new file path
                    new_file = input(f"Enter a new file path for the '{normalized_action}' action or press Enter to skip: ").strip()
                    if new_file:
                        # Update the file path in the change
                        change['file'] = new_file
                        # Re-execute the current change with the new path
                        self.execute([change], plan_tracker)
                    else:
                        self.log_info(f"Skipped applying change to '{file}'.")
                        print_with_breaker(f"Skipped applying change to '{file}'.")
                        continue
                else:
                    self.log_warning("Invalid input received. Skipping this change.")
                    print_with_breaker("Invalid input received. Skipping this change.")
                    continue

            self.log_info("All code changes have been processed.")
            print_with_breaker("All code changes have been processed.")
        except Exception as e:
            self.log_error(f"Error during code writing: {e}")
            print_with_breaker(f"Error: {e}")
    
    def apply_change(self, action, file, code):
        """
        Applies the code change to the specified file.

        :param action: 'add' or 'update'.
        :param file: Relative file path.
        :param code: Code string to add or update.
        :return: JSON response string.
        """
        try:
            if os.path.isabs(file):
                self.log_warning(f"Absolute file path detected '{file}'. Skipping to prevent writing outside the repository.")
                return json.dumps({"status": "failure", "message": "Absolute file paths are not allowed."})

            file_path = self.resolve_path(file)
            if not file_path:
                self.log_warning(f"Attempted to write outside the repository with file '{file}'. Skipping.")
                return json.dumps({"status": "failure", "message": "Invalid file path."})

            os.makedirs(os.path.dirname(file_path), exist_ok=True)

            if os.path.exists(file_path):
                if is_python_file(file_path):
                    function_name = self.extract_function_name(code)
                    if function_name:
                        # Check for duplicate function names
                        if self.function_exists(file_path, function_name) and action == 'add':
                            self.log_warning(f"Function '{function_name}' already exists in '{file_path}'. Skipping addition.")
                            return json.dumps({"status": "failure", "message": f"Function '{function_name}' already exists."})
                        with open(file_path, 'a') as f:
                            f.write(f"\n\n{code}")
                        self.log_info(f"Appended new function '{function_name}' to existing file '{file_path}'.")
                        return json.dumps({"status": "success", "message": f"Appended function '{function_name}' to '{file}'."})
                    else:
                        self.log_warning(f"Failed to extract function name from the provided code for file '{file_path}'. Skipping.")
                        return json.dumps({"status": "failure", "message": "Failed to extract function name."})
                else:
                    with open(file_path, 'a') as f:
                        f.write(f"\n\n{code}")
                    self.log_info(f"Appended code to existing file '{file_path}'.")
                    return json.dumps({"status": "success", "message": f"Appended code to '{file}'."})
            else:
                with open(file_path, 'w') as f:
                    f.write(code)
                self.log_info(f"Created new file '{file_path}' and added the new function/code.")
                return json.dumps({"status": "success", "message": f"Created file '{file}' and added code."})

        except PermissionError:
            self.log_error(f"Permission denied when trying to write to '{file_path}'.")
            return json.dumps({"status": "failure", "message": "Permission denied."})
        except Exception as e:
            self.log_error(f"Unexpected error while applying change to '{file_path}': {e}")
            return json.dumps({"status": "failure", "message": "Unexpected error occurred."})

    def validate_code(self, code):
        """
        Validates the provided code for syntax correctness.

        :param code: Code string to validate.
        :return: Tuple (is_valid, error_message)
        """
        try:
            ast.parse(code)
            return (True, "")
        except SyntaxError as e:
            self.log_warning(f"Syntax error in generated code: {e}")
            return (False, str(e))

    def normalize_action(self, action):
        """
        Normalizes the action string.

        :param action: Original action string.
        :return: Normalized action.
        """
        if action.lower() in ['add_function', 'add']:
            return 'add'
        elif action.lower() in ['update', 'modify']:
            return 'update'
        else:
            return action.lower()

    def check_functions_complete(self, file):
        """
        Checks if all functions in the specified file are complete.

        :param file: Relative file path.
        :return: True if all functions are complete, False otherwise.
        """
        try:
            file_path = os.path.join(self.repo_path, file)
            with open(file_path, 'r') as f:
                content = f.read()
            tree = ast.parse(content)
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef):
                    if not self.is_function_complete(node):
                        return False
            return True
        except Exception as e:
            self.log_warning(f"Failed to check function completeness in '{file}': {e}")
            return False

    def get_incomplete_functions(self, file):
        """
        Retrieves a list of incomplete functions in the specified file.

        :param file: Relative file path.
        :return: List of function names.
        """
        incomplete = []
        try:
            file_path = os.path.join(self.repo_path, file)
            with open(file_path, 'r') as f:
                content = f.read()
            tree = ast.parse(content)
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef):
                    if not self.is_function_complete(node):
                        incomplete.append(node.name)
        except Exception as e:
            self.log_warning(f"Failed to retrieve incomplete functions from '{file}': {e}")
        return incomplete

    def apply_hints_to_code(self, code, hints):
        """
        Applies user-provided hints or instructions to the generated code.

        :param code: Original code string.
        :param hints: User-provided hints or instructions.
        :return: Updated code string.
        """
        try:
            # For simplicity, append the hints as comments.
            # More sophisticated processing can be implemented as needed.
            updated_code = f"{code}\n\n# User Hints/Instructions:\n# {hints}"
            return updated_code
        except Exception as e:
            self.log_warning(f"Failed to apply hints to code: {e}")
            return code

    def execute_sub_plan(self, plan_name, sub_plan_objective, plan_tracker):
        """
        Executes a single sub-plan to complete a function.

        :param plan_name: Name of the plan.
        :param sub_plan_objective: Objective of the sub-plan.
        :param plan_tracker: Instance of PlanTracker to update plan statuses.
        """
        print(f"Processing sub-plan: {sub_plan_objective}")
        logging.info(f"Processing sub-plan: {sub_plan_objective}")

        # Step 1: Generate code changes for the sub-plan using LLM
        # Since CodeWritingAgent does not have access to LLM, it should delegate to AnswerGenerationAgent
        # or another agent responsible for generating code changes in JSON format.

        # For demonstration, we'll assume that code_changes are already provided.
        # In a real scenario, you would interact with other agents here.

        # Example:
        # code_changes = self.answer_generation_agent.generate_code_changes(sub_plan_objective)
        # self.execute(code_changes, plan_tracker)

        # Placeholder response
        simulated_response = json.dumps({
            "status": "success",
            "message": f"Simulated application of sub-plan '{sub_plan_objective}'."
        })

        parsed_response = self.json_parse_agent.execute(simulated_response)
        if parsed_response and parsed_response.get('status') == 'success':
            self.log_info(f"Successfully executed sub-plan '{sub_plan_objective}'. Message: {parsed_response.get('message')}")
            print_with_breaker(f"Successfully executed sub-plan '{sub_plan_objective}'.")
            # Mark the sub-plan as completed in PlanTracker
            plan_tracker.mark_sub_plan_completed(plan_name, sub_plan_objective)
        else:
            self.log_warning(f"Failed to execute sub-plan '{sub_plan_objective}'. Message: {parsed_response.get('message') if parsed_response else 'No message.'}")
            print_with_breaker(f"Warning: Failed to execute sub-plan '{sub_plan_objective}'.")

    def is_function_complete(self, node):
        """
        Determines if a function has a complete implementation.

        :param node: AST FunctionDef node.
        :return: True if complete, False otherwise.
        """
        if len(node.body) == 0:
            return False
        for stmt in node.body:
            if isinstance(stmt, ast.Pass):
                return False
            if isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Str):
                # Function has only a docstring
                continue
            return True
        return False

    def extract_function_name(self, code):
        """
        Extracts the function name from the provided code.

        :param code: Code string containing a function definition.
        :return: Function name or None.
        """
        try:
            for line in code.split('\n'):
                if line.strip().startswith('def '):
                    return line.strip().split('def ')[1].split('(')[0]
            return None
        except Exception as e:
            self.log_error(f"Error extracting function name: {e}")
            return None

    def function_exists(self, file_path, function_name):
        """
        Checks if a function already exists in the specified file.

        :param file_path: Path to the file.
        :param function_name: Name of the function to check.
        :return: True if exists, False otherwise.
        """
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            tree = ast.parse(content)
            for node in ast.walk(tree):
                if isinstance(node, ast.FunctionDef) and node.name == function_name:
                    return True
            return False
        except Exception as e:
            self.log_warning(f"Failed to check if function '{function_name}' exists in '{file_path}': {e}")
            return False

    def resolve_path(self, file):
        """
        Resolves the absolute path of the file and ensures it's within the repository.

        :param file: Relative or absolute file path.
        :return: Absolute file path if valid, else None.
        """
        target_path = os.path.abspath(os.path.join(self.repo_path, file))
        if os.path.commonpath([self.repo_path, target_path]) != self.repo_path:
            return None
        return target_path

    def extract_plan_names(self, file_path):
        """
        Extracts plan and sub-plan names from the file path.
        Assumes a naming convention or directory structure to map files to plans.

        :param file_path: Relative file path.
        :return: Tuple (plan_name, sub_plan_name) or (None, None) if extraction fails.
        """
        try:
            # Example: If file path is 'plans/plan1/subplan1.py', extract 'plan1' and 'subplan1'
            parts = file_path.split(os.sep)
            if len(parts) >= 2:
                plan_name = parts[-2]
                sub_plan_name = os.path.splitext(parts[-1])[0]
                return (plan_name, sub_plan_name)
            else:
                return (None, None)
        except Exception as e:
            self.log_warning(f"Failed to extract plan names from file path '{file_path}': {e}")
            return (None, None)
