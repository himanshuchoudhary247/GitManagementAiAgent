# utils/code_utils.py

import json
import re
import logging
import ast

def extract_json_from_response(response):
    """
    Attempts to extract JSON object from a given string.

    :param response: Raw response string from the LLM.
    :return: Parsed JSON object or None if extraction fails.
    """
    try:
        # Regex to find the first JSON object in the response
        json_str = re.search(r'\{.*\}', response, re.DOTALL)
        if json_str:
            return json.loads(json_str.group())
        else:
            logging.warning("No JSON object found in the response.")
            return None
    except json.JSONDecodeError as e:
        logging.warning(f"JSON decoding failed: {e}")
        return None

def correct_json(response):
    """
    Attempts to correct common JSON formatting issues in the response.

    :param response: Raw response string from the LLM.
    :return: Corrected JSON string or None if correction fails.
    """
    try:
        # Attempt to fix single quotes to double quotes
        response = response.replace("'", '"')
        # Remove trailing commas
        response = re.sub(r',\s*}', '}', response)
        response = re.sub(r',\s*]', ']', response)
        return response
    except Exception as e:
        logging.warning(f"Failed to correct JSON: {e}")
        return None

def find_function_definitions(file_content, function_name):
    """
    Finds the start and end line numbers of a given function in the file content.

    :param file_content: Content of the Python file as a string.
    :param function_name: Name of the function to find.
    :return: Tuple of (start_line, end_line). Returns (None, None) if not found.
    """
    try:
        tree = ast.parse(file_content)
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef) and node.name == function_name:
                start_line = node.lineno - 1  # Zero-based indexing
                # To find end line, find the last line in the function
                # Assumption: AST nodes have 'end_lineno' (Python 3.8+)
                if hasattr(node, 'end_lineno'):
                    end_line = node.end_lineno
                else:
                    # Fallback for older Python versions
                    end_line = find_end_lineno(file_content.split('\n'), node)
                return (start_line, end_line)
        logging.warning(f"Function '{function_name}' not found in the provided content.")
        return (None, None)
    except Exception as e:
        logging.warning(f"Error parsing file content for function '{function_name}': {e}")
        return (None, None)

def find_end_lineno(lines, node):
    """
    Finds the end line number of a function node in the AST for Python versions < 3.8.

    :param lines: List of lines from the file.
    :param node: AST FunctionDef node.
    :return: End line number.
    """
    # This is a naive implementation; for more accurate results, consider using asttokens or similar libraries
    start = node.lineno - 1
    for i in range(start + 1, len(lines)):
        if lines[i].startswith(('def ', 'class ', '@')):
            return i
    return len(lines)

def is_python_file(file_path):
    """
    Checks if a file is a Python file based on its extension.

    :param file_path: Path to the file.
    :return: True if Python file, False otherwise.
    """
    return file_path.endswith('.py')
