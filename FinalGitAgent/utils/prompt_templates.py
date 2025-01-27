# utils/prompt_templates.py
def code_completion_prompt(file, function_name):
    """
    Constructs a prompt to generate the complete function implementation in JSON format.

    :param file: Relative file path.
    :param function_name: Name of the incomplete function.
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Read the context of the incomplete function in the specified file.
    - Provide a fully implemented version of the function.
    - Return the result strictly in valid JSON format.
    - Do not include additional explanations outside the JSON.
    - Your JSON must include the following keys: "action", "file", and "code".
        - "action" can be "add" or "update".
        - "file" should be the same file path given in the prompt.
        - "code" should contain the complete Python function.

    Example usage:
        prompt = code_completion_prompt("utils/helpers.py", "my_function")
        print(prompt)

    Example valid JSON response:
    {
        "action": "update",
        "file": "utils/helpers.py",
        "code": \"\"\"
def my_function():
    # Implementation details here
    pass
\"\"\"
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are a helpful coding assistant. The following is an incomplete function in the file {file}:

def {function_name}():
    pass  # TODO: Implement this function

Please provide a complete implementation for the function '{function_name}' based on the context of the file.
Output your response strictly in the following JSON format:

{{
    "action": "add" or "update",
    "file": "{file}",
    "code": """
def {function_name}():
    # Complete implementation here
    pass
"""
}}
'''


def self_reflection_prompt(code_changes):
    """
    Constructs a prompt to generate self-reflection based on code changes in JSON format.

    :param code_changes: List of code changes. Each item is a dict with keys "action", "function", and "file".
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Reflect on the listed code changes.
    - Suggest best practices, improvements, or optimizations.
    - Return strictly valid JSON with the key "reflection".

    Example usage:
        changes = [
            {"action": "update", "function": "my_func", "file": "app/main.py"},
            {"action": "add", "function": "helper_func", "file": "utils/helper.py"}
        ]
        prompt = self_reflection_prompt(changes)
        print(prompt)

    Example valid JSON response:
    {
        "reflection": "The changes improve modularity, but consider adding error handling and docstrings..."
    }
    ---------------------------------------------------------------------
    """
    changes = "\n".join([
        f"- {change['action'].capitalize()} function '{change['function']}' in file '{change['file']}'."
        for change in code_changes
    ])
    return f'''
You have made the following changes to the codebase:

{changes}

Please reflect on these changes and suggest any potential improvements or best practices that could enhance the code quality and maintainability.
Output your response strictly in the following JSON format:

{{
    "reflection": "Your reflection and suggestions here."
}}
'''


def query_understanding_prompt(requirement):
    """
    Constructs a prompt to break down user requirements into actionable objectives in JSON format.

    :param requirement: User's requirement (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Break down the requirement into logical objectives and sub-objectives.
    - Output strictly valid JSON with the key "objectives", which is a list of strings.
    - Do not include any additional text outside the JSON.

    Example usage:
        prompt = query_understanding_prompt("I need a user authentication system with JWT support.")
        print(prompt)

    Example valid JSON response:
    {
        "objectives": [
            "Implement user login",
            "Set up JWT-based authentication",
            "Provide refresh token functionality"
        ]
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are a helpful assistant. The user has provided the following requirement:

"{requirement}"

Please break down this requirement into clear, actionable objectives and sub-objectives that can be used to plan the implementation.
Output your response strictly in the following JSON format:

{{
    "objectives": [
        "Objective 1",
        "Objective 2",
        ...
    ]
}}
'''


def function_completeness_prompt(function_code):
    """
    Constructs a prompt to determine if a function is complete in JSON format.

    :param function_code: The complete code of the function (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Analyze the given function code.
    - Determine if it is complete and free of errors.
    - If incomplete or erroneous, suggest how to fix or complete it.
    - Output strictly valid JSON with:
        - "status": "Complete" or "Incomplete"
        - "suggestions": Provide suggestions if "Incomplete".

    Example usage:
        code_snippet = \"\"\"def some_function(x): return x*x\"\"\"
        prompt = function_completeness_prompt(code_snippet)
        print(prompt)

    Example valid JSON response:
    {
        "status": "Complete",
        "suggestions": ""
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are a code analysis assistant. Analyze the following Python function and determine if it is complete and free of errors. If the function is incomplete or contains errors, provide suggestions to complete or fix it.

Function Code:
{function_code}

Is this function complete and error-free? Respond strictly in the following JSON format:

{{
    "status": "Complete" or "Incomplete",
    "suggestions": "Your suggestions here if incomplete."
}}
'''


def plan_creation_prompt(objectives):
    """
    Constructs a prompt to create a plan based on given objectives in JSON format.

    :param objectives: List of objectives (list of strings).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Each objective should have a list of steps.
    - Return strictly valid JSON with the key "plan", which is a list of dictionaries.
      Each dictionary in "plan" contains:
        - "objective": The objective text
        - "steps": A list of steps
    - Do not include text outside the JSON.

    Example usage:
        prompt = plan_creation_prompt(["Implement login", "Set up logging"])
        print(prompt)

    Example valid JSON response:
    {
        "plan": [
            {
                "objective": "Implement login",
                "steps": [
                    "Create user model",
                    "Add login route",
                    "Validate credentials"
                ]
            },
            {
                "objective": "Set up logging",
                "steps": [
                    "Install logging library",
                    "Configure log levels",
                    "Integrate log outputs"
                ]
            }
        ]
    }
    ---------------------------------------------------------------------
    """
    objectives_formatted = "\n".join([f"- {obj}" for obj in objectives])
    return f'''
You are a planning assistant. Based on the following objectives, create a detailed plan outlining the steps required to achieve each objective. Ensure that the plan is clear, actionable, and logically structured.

Objectives:
{objectives_formatted}

Please provide the plan strictly in the following JSON format:

{{
    "plan": [
        {{
            "objective": "Objective 1",
            "steps": [
                "Step 1",
                "Step 2",
                ...
            ]
        }},
        {{
            "objective": "Objective 2",
            "steps": [
                "Step 1",
                "Step 2",
                ...
            ]
        }},
        ...
    ]
}}
'''


def context_retrieval_prompt(sub_objective):
    """
    Constructs a prompt to retrieve context related to a sub-objective in JSON format.

    :param sub_objective: The sub-objective for which context is needed (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Provide any relevant code snippets, references, or resources related to the sub-objective.
    - Return strictly valid JSON with the key "context", which is a list of strings.

    Example usage:
        prompt = context_retrieval_prompt("Implement a function to parse configuration files")
        print(prompt)

    Example valid JSON response:
    {
        "context": [
            "Relevant config libraries: configparser, PyYAML",
            "Existing config parsing utilities in config_utils.py"
        ]
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are a context retrieval assistant. Provide relevant information, code snippets, and resources that will help in accomplishing the following sub-objective:

"{sub_objective}"

Ensure that the context is directly related to the sub-objective and can aid in its implementation.
Output your response strictly in the following JSON format:

{{
    "context": [
        "Context 1",
        "Context 2",
        ...
    ]
}}
'''


def intermediate_processing_prompt(sub_objective, relevant_functions, repo_path):
    """
    Constructs a prompt for intermediate processing based on the sub-objective and relevant functions in JSON format.

    :param sub_objective: The sub-objective being processed (string).
    :param relevant_functions: List of relevant functions (list of strings).
    :param repo_path: Path to the repository (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Summarize how the relevant functions and existing repo structure can help achieve the sub-objective.
    - Provide any additional context or steps needed before proceeding to code generation.
    - Return strictly valid JSON with the key "additional_context", which is a list of strings.

    Example usage:
        relevant_funcs = ["parse_config", "load_env_vars"]
        prompt = intermediate_processing_prompt("Implement config loading", relevant_funcs, "/my/project")
        print(prompt)

    Example valid JSON response:
    {
        "additional_context": [
            "Ensure config files are present",
            "Consider storing credentials as environment variables"
        ]
    }
    ---------------------------------------------------------------------
    """
    functions_formatted = "\n".join([f"- {func}" for func in relevant_functions])
    return f'''
You are an intermediate processing assistant. Based on the sub-objective and the following relevant functions, process the information to prepare for code generation.

Sub-Objective:
"{sub_objective}"

Relevant Functions:
{functions_formatted}

Repository Path:
"{repo_path}"

Please provide any additional context or processing needed to accomplish the sub-objective effectively.
Output your response strictly in the following JSON format:

{{
    "additional_context": [
        "Additional Context 1",
        "Additional Context 2",
        ...
    ]
}}
'''


def answer_generation_prompt(sub_objective, relevant_functions, additional_context, repo_path):
    """
    Constructs a prompt to generate answers or code changes based on the sub-objective and context in JSON format.

    :param sub_objective: The sub-objective being addressed (string).
    :param relevant_functions: List of relevant functions (list of strings).
    :param additional_context: Additional context from intermediate processing (list of strings).
    :param repo_path: Path to the repository (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Use the provided context, relevant functions, and sub-objective to propose code changes.
    - Return strictly valid JSON under the key "code_changes", which is a list of changes.
    - Each item in "code_changes" must include: "action", "file", and "code".
      - "action" can be "add" or "update".
      - "file" is the relative path to the file.
      - "code" is the Python code to be added or updated.

    Example usage:
        relevant_funcs = ["parse_config", "load_env_vars"]
        additional_ctx = ["We already have a config.yaml in the repo."]
        prompt = answer_generation_prompt("Implement config loading", relevant_funcs, additional_ctx, "/my/project")
        print(prompt)

    Example valid JSON response:
    {
        "code_changes": [
            {
                "action": "add",
                "file": "utils/config_loader.py",
                "code": \"\"\"
def load_config():
    # Implementation here
    pass
\"\"\"
            },
            {
                "action": "update",
                "file": "app/main.py",
                "code": \"\"\"
# Updated code in main.py
def main():
    config = load_config()
    pass
\"\"\"
            }
        ]
    }
    ---------------------------------------------------------------------
    """
    functions_formatted = "\n".join([f"- {func}" for func in relevant_functions])
    additional_context_formatted = "\n".join(additional_context)
    return f'''
You are a code generation assistant. Based on the following sub-objective and context, generate the necessary code changes to accomplish the task.

Sub-Objective:
"{sub_objective}"

Relevant Functions:
{functions_formatted}

Additional Context:
{additional_context_formatted}

Repository Path:
"{repo_path}"

Please provide the code changes strictly in the following JSON format:

{{
    "code_changes": [
        {{
            "action": "add" or "update",
            "file": "relative/path/to/file.py",
            "code": """
def new_function():
    # Implementation here
    pass
"""
        }},
        ...
    ]
}}
'''


def code_change_prompt(action, file, code):
    """
    Constructs a prompt for code changes to be applied in JSON format.

    :param action: The action to perform (e.g., "add", "update").
    :param file: The target file path (string).
    :param code: The code to add or update (string).
    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Confirm the code change as instructed.
    - Ensure best practices are maintained.
    - Return strictly valid JSON with "status" and "message" keys.

    Example usage:
        prompt = code_change_prompt("update", "app/main.py", "def new_main(): pass")
        print(prompt)

    Example valid JSON response:
    {
        "status": "success",
        "message": "The code was successfully updated."
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are a code change assistant. Perform the following action on the codebase:

Action: {action.capitalize()}
File: {file}

Code:
{code}

Please ensure that the changes adhere to best practices and maintain code integrity.
Output your response strictly in the following JSON format:

{{
    "status": "success" or "failure",
    "message": "Detailed message here."
}}
'''


def undo_changes_prompt():
    """
    Constructs a prompt to undo recent changes in JSON format.

    :return: Prompt string optimized for LLM usage.

    ---------------------------------------------------------------------
    Instructions for the LLM:
    - Revert the most recent code changes to restore the previous stable state.
    - Return strictly valid JSON with "status" and "message" keys.

    Example usage:
        prompt = undo_changes_prompt()
        print(prompt)

    Example valid JSON response:
    {
        "status": "success",
        "message": "Reverted the most recent commit. Changes undone."
    }
    ---------------------------------------------------------------------
    """
    return f'''
You are an undo assistant. Revert the most recent changes made to the codebase, restoring it to its previous stable state. Ensure that all files are correctly restored without introducing new issues.
Output your response strictly in the following JSON format:

{{
    "status": "success" or "failure",
    "message": "Detailed message here."
}}
'''



# def code_completion_prompt(file, function_name):
#     """
#     Constructs a prompt to generate the complete function implementation in JSON format.

#     :param file: Relative file path.
#     :param function_name: Name of the incomplete function.
#     :return: Prompt string.
#     """
#     return f'''
# You are a helpful assistant. The following is an incomplete function in the file {file}:

# def {function_name}():
#     pass  # TODO: Implement this function

# Please provide a complete implementation for the function '{function_name}' based on the context of the file.
# Output your response in the following JSON format:

# {{
#     "action": "add" or "update",
#     "file": "{file}",
#     "code": """
# def {function_name}():
#     # Complete implementation here
#     pass
# """
# }}
# '''

# def self_reflection_prompt(code_changes):
#     """
#     Constructs a prompt to generate self-reflection based on code changes in JSON format.

#     :param code_changes: List of code changes.
#     :return: Prompt string.
#     """
#     changes = "\n".join([f"- {change['action'].capitalize()} function '{change['function']}' in file '{change['file']}'." for change in code_changes])
#     return f'''
# You have made the following changes to the codebase:

# {changes}

# Please reflect on these changes and suggest any potential improvements or best practices that could enhance the code quality and maintainability.
# Output your response in the following JSON format:

# {{
#     "reflection": "Your reflection and suggestions here."
# }}
# '''

# def query_understanding_prompt(requirement):
#     """
#     Constructs a prompt to break down user requirements into actionable objectives in JSON format.

#     :param requirement: User's requirement.
#     :return: Prompt string.
#     """
#     return f'''
# You are a helpful assistant. The user has provided the following requirement:

# "{requirement}"

# Please break down this requirement into clear, actionable objectives and sub-objectives that can be used to plan the implementation.
# Output your response in the following JSON format:

# {{
#     "objectives": [
#         "Objective 1",
#         "Objective 2",
#         ...
#     ]
# }}
# '''

# def function_completeness_prompt(function_code):
#     """
#     Constructs a prompt to determine if a function is complete in JSON format.

#     :param function_code: The complete code of the function.
#     :return: Prompt string.
#     """
#     return f'''
# You are a code analysis assistant. Analyze the following Python function and determine if it is complete and free of errors. If the function is incomplete or contains errors, provide suggestions to complete or fix it.

# Function Code:
# {function_code}

# Is this function complete and error-free? Respond with the following JSON format:

# {{
#     "status": "Complete" or "Incomplete",
#     "suggestions": "Your suggestions here if incomplete."
# }}
# '''

# def plan_creation_prompt(objectives):
#     """
#     Constructs a prompt to create a plan based on given objectives in JSON format.

#     :param objectives: List of objectives.
#     :return: Prompt string.
#     """
#     objectives_formatted = "\n".join([f"- {obj}" for obj in objectives])
#     return f'''
# You are a planning assistant. Based on the following objectives, create a detailed plan outlining the steps required to achieve each objective. Ensure that the plan is clear, actionable, and logically structured.

# Objectives:
# {objectives_formatted}

# Please provide the plan in the following JSON format:

# {{
#     "plan": [
#         {{
#             "objective": "Objective 1",
#             "steps": [
#                 "Step 1",
#                 "Step 2",
#                 ...
#             ]
#         }},
#         {{
#             "objective": "Objective 2",
#             "steps": [
#                 "Step 1",
#                 "Step 2",
#                 ...
#             ]
#         }},
#         ...
#     ]
# }}
# '''

# def context_retrieval_prompt(sub_objective):
#     """
#     Constructs a prompt to retrieve context related to a sub-objective in JSON format.

#     :param sub_objective: The sub-objective for which context is needed.
#     :return: Prompt string.
#     """
#     return f'''
# You are a context retrieval assistant. Provide relevant information, code snippets, and resources that will help in accomplishing the following sub-objective:

# "{sub_objective}"

# Ensure that the context is directly related to the sub-objective and can aid in its implementation.
# Output your response in the following JSON format:

# {{
#     "context": [
#         "Context 1",
#         "Context 2",
#         ...
#     ]
# }}
# '''

# def intermediate_processing_prompt(sub_objective, relevant_functions, repo_path):
#     """
#     Constructs a prompt for intermediate processing based on the sub-objective and relevant functions in JSON format.

#     :param sub_objective: The sub-objective being processed.
#     :param relevant_functions: List of relevant functions retrieved from the context.
#     :param repo_path: Path to the repository.
#     :return: Prompt string.
#     """
#     functions_formatted = "\n".join([f"- {func}" for func in relevant_functions])
#     return f'''
# You are an intermediate processing assistant. Based on the sub-objective and the following relevant functions, process the information to prepare for code generation.

# Sub-Objective:
# "{sub_objective}"

# Relevant Functions:
# {functions_formatted}

# Repository Path:
# "{repo_path}"

# Please provide any additional context or processing needed to accomplish the sub-objective effectively.
# Output your response in the following JSON format:

# {{
#     "additional_context": [
#         "Additional Context 1",
#         "Additional Context 2",
#         ...
#     ]
# }}
# '''

# def answer_generation_prompt(sub_objective, relevant_functions, additional_context, repo_path):
#     """
#     Constructs a prompt to generate answers or code changes based on the sub-objective and context in JSON format.

#     :param sub_objective: The sub-objective being addressed.
#     :param relevant_functions: List of relevant functions.
#     :param additional_context: Additional context from intermediate processing.
#     :param repo_path: Path to the repository.
#     :return: Prompt string.
#     """
#     functions_formatted = "\n".join([f"- {func}" for func in relevant_functions])
#     additional_context_formatted = "\n".join(additional_context)
#     return f'''
# You are a code generation assistant. Based on the following sub-objective and context, generate the necessary code changes to accomplish the task.

# Sub-Objective:
# "{sub_objective}"

# Relevant Functions:
# {functions_formatted}

# Additional Context:
# {additional_context_formatted}

# Repository Path:
# "{repo_path}"

# Please provide the code changes in the following JSON format:

# {{
#     "code_changes": [
#         {{
#             "action": "add" or "update",
#             "file": "relative/path/to/file.py",
#             "code": """
# def new_function():
#     # Implementation here
#     pass
# """
#         }},
#         ...
#     ]
# }}
# '''

# def code_change_prompt(action, file, code):
#     """
#     Constructs a prompt for code changes to be applied in JSON format.

#     :param action: The action to perform (e.g., add, update).
#     :param file: The target file path.
#     :param code: The code to add or update.
#     :return: Prompt string.
#     """
#     return f'''
# You are a code change assistant. Perform the following action on the codebase:

# Action: {action.capitalize()}
# File: {file}

# Code:
# {code}

# Please ensure that the changes adhere to best practices and maintain code integrity.
# Output your response in the following JSON format:

# {{
#     "status": "success" or "failure",
#     "message": "Detailed message here."
# }}
# '''

# def undo_changes_prompt():
#     """
#     Constructs a prompt to undo recent changes in JSON format.

#     :return: Prompt string.
#     """
#     return f'''
# You are an undo assistant. Revert the most recent changes made to the codebase, restoring it to its previous stable state. Ensure that all files are correctly restored without introducing new issues.
# Output your response in the following JSON format:

# {{
#     "status": "success" or "failure",
#     "message": "Detailed message here."
# }}
# '''
