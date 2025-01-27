# utils/json_parser.py

import json
import logging

def parse_json_safe(raw_string):
    """
    Safely parses a JSON string. Returns a tuple (success, data/error message).

    :param raw_string: The JSON string to parse.
    :return: (True, data) if successful, (False, error message) otherwise.
    """
    try:
        data = json.loads(raw_string)
        return (True, data)
    except json.JSONDecodeError as e:
        logging.warning(f"JSON parsing failed: {e}")
        return (False, str(e))
