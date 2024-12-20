# utils/code_utils.py

import ast
import os

def extract_functions_from_file(file_path):
    with open(file_path, 'r') as f:
        tree = ast.parse(f.read(), filename=file_path)
    return [node.name for node in ast.walk(tree) if isinstance(node, ast.FunctionDef)]

def find_function_definitions(content, function_name):
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
