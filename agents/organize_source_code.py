# agents/organize_source_code.py

import os
from filesystem import create_directory, move_file
from utils.cli_utils import print_with_breaker
import logging

def organize_source_code_directories(repo_path):
    """
    Organizes the source code directories by moving files into appropriate folders.
    """
    try:
        # Define the directory structure
        directory_structure = {
            'src': ['main.py', 'file_manager.py', 'filesystem.py'],
            'tests': ['test_main.py'],
            'docs': ['README.md', 'documentation.py'],
            'backup': ['backup.py'],
            'integrity': ['integrity.py'],
            'utils': ['cli_utils.py', 'change_tracker.py', 'memory_node.py'],
            'agents': ['code_writing_agent.py', 'execution_agent.py', 'undo_agent.py', 'project_initialization_agent.py', 'self_reflection_agent.py']
        }

        for directory, files in directory_structure.items():
            dir_path = os.path.join(repo_path, directory)
            create_directory(dir_path)
            for file in files:
                source_path = os.path.join(repo_path, file)
                if os.path.exists(source_path):
                    move_file(source_path, dir_path)
        logging.info("Source code directories organized successfully.")
        print_with_breaker("Source code directories organized successfully.")
    except Exception as e:
        logging.error(f"Error organizing source code directories: {e}")
        print_with_breaker(f"Error organizing source code directories: {e}")
