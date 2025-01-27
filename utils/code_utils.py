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
    lines = content.split('\n')
    start_line = None
    end_line = None
    for i, line in enumerate(lines):
        if line.strip().startswith(f"def {function_name}("):
            start_line = i
            for j in range(i + 1, len(lines)):
                if lines[j].strip().startswith("def "):
                    end_line = j
                    break
            if end_line is None:
                end_line = len(lines)
            break
    return start_line, end_line

def extract_json_from_response(response):
    try:
        return json.loads(response)
    except json.JSONDecodeError:
        # Attempt to extract JSON from code blocks
        json_code_blocks = re.findall(r'```(?:json)?\s*(\{.*?\}|\[.*?\])\s*```', response, re.DOTALL)
        for json_block in json_code_blocks:
            try:
                return json.loads(json_block)
            except json.JSONDecodeError:
                continue
        # Attempt to find any JSON-like patterns
        json_matches = re.findall(r'\{.*?\}|\[.*?\]', response, re.DOTALL)
        for match in json_matches:
            try:
                return json.loads(match)
            except json.JSONDecodeError:
                continue
    return {}

def correct_json(response):
    """
    Attempts to correct common JSON formatting issues in the response.
    """
    try:
        corrected = response

        # Replace single quotes with double quotes
        corrected = corrected.replace("'", '"')

        # Remove trailing commas
        corrected = re.sub(r',\s*([\]}])', r'\1', corrected)

        # Ensure keys are quoted
        corrected = re.sub(r'(\w+):', r'"\1":', corrected)

        # Remove hyphens before keys and values
        corrected = re.sub(r'-\s*"', '"', corrected)
        corrected = re.sub(r'\s*-\s*', '', corrected)

        # Fix missing quotes around keys in nested objects
        corrected = re.sub(r'(\w+):\s*\{', r'"\1": {', corrected)
        corrected = re.sub(r'(\w+):\s*\[', r'"\1": [', corrected)

        # Additional corrections can be added here

        return corrected
    except Exception as e:
        # If any error occurs during correction, return original response
        return response

def is_python_file(file_path):
    return file_path.endswith('.py')
