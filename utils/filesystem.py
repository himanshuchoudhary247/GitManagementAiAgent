# utils/filesystem.py

import os
import shutil
import logging
from utils.cli_utils import print_with_breaker

def create_directory(path: str):
    """
    Creates a directory if it does not exist.

    Parameters:
    - path (str): The path of the directory to create.
    """
    try:
        os.makedirs(path, exist_ok=True)
        logging.info(f"Directory created or already exists: {path}")
    except Exception as e:
        logging.error(f"Failed to create directory {path}: {e}")
        print_with_breaker(f"Error: Failed to create directory {path}: {e}")

def move_file(source: str, destination: str):
    """
    Moves a file from source to destination.

    Parameters:
    - source (str): The source file path.
    - destination (str): The destination directory path.
    """
    try:
        if not os.path.exists(source):
            logging.error(f"Source file does not exist: {source}")
            print_with_breaker(f"Error: Source file does not exist: {source}")
            return
        shutil.move(source, destination)
        logging.info(f"Moved file from {source} to {destination}")
    except Exception as e:
        logging.error(f"Failed to move file from {source} to {destination}: {e}")
        print_with_breaker(f"Error: Failed to move file from {source} to {destination}: {e}")
