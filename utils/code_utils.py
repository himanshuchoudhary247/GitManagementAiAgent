# utils/code_utils.py

import ast
import os
import json
import re

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

def extract_json_from_response(response):
    """
    Extracts the first valid JSON object or array found in the response string.
    Returns the JSON object/list if found, else returns an empty dictionary.
    """
    try:
        # Attempt to parse the entire response
        return json.loads(response)
    except json.JSONDecodeError:
        # Look for JSON within code blocks marked as json
        json_code_blocks = re.findall(r'```json\s*(\{.*?\}|\[.*?\])\s*```', response, re.DOTALL)
        for json_block in json_code_blocks:
            try:
                return json.loads(json_block)
            except json.JSONDecodeError:
                continue
        # If no JSON code blocks, look for any JSON object or array
        json_matches = re.findall(r'\{.*?\}|\[.*?\]', response, re.DOTALL)
        for match in json_matches:
            try:
                return json.loads(match)
            except json.JSONDecodeError:
                continue
    return {}
