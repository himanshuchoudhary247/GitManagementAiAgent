# agents/file_manager.py

import os
import logging
from utils.filesystem import create_directory, move_file
from utils.cli_utils import print_with_breaker

class FileManager:
    def __init__(self, repo_path: str):
        self.repo_path = os.path.abspath(repo_path)

    def get_all_files(self) -> list:
        """
        Retrieve all files in the repository excluding specific system files.
        """
        file_list = []
        for root, dirs, files in os.walk(self.repo_path):
            for file in files:
                if file not in ['change_log.json', 'README.md', 'user_guide.md']:
                    file_path = os.path.join(root, file)
                    file_list.append(file_path)
        return file_list

    def determine_target_directory(self, file_path: str, new_structure: dict) -> str:
        """
        Determine the target directory based on the new directory structure.
        """
        file_name = os.path.basename(file_path)
        for directory, files in new_structure.items():
            if file_name in files:
                return os.path.join(self.repo_path, directory)
        return os.path.join(self.repo_path, 'misc')  # Default directory

    def arrange_files_dynamic(self, new_structure: dict):
        """
        Arrange files based on the new directory structure.
        """
        try:
            files = self.get_all_files()
            for file_path in files:
                target_dir = self.determine_target_directory(file_path, new_structure)
                if not os.path.exists(target_dir):
                    create_directory(target_dir)
                move_file(file_path, target_dir)
            logging.info("Files have been arranged into the new directory structure.")
            print_with_breaker("Files have been arranged into the new directory structure.")
        except Exception as e:
            logging.error(f"Error arranging files: {e}")
            print_with_breaker(f"Error arranging files: {e}")

    def arrange_files(self):
        """
        Arrange files into predefined directories based on file types or other criteria.
        """
        try:
            # Define a default directory structure if needed
            default_structure = {
                'src': [],
                'tests': [],
                'docs': [],
                'configs': [],
                'backup': [],
                'integrity': [],
                'utils': [],
                'agents': [],
                'misc': []
            }
            files = self.get_all_files()
            for file_path in files:
                file_name = os.path.basename(file_path)
                # Example criteria for categorizing files
                if file_name.endswith('.py'):
                    default_structure['src'].append(file_name)
                elif file_name.startswith('test_'):
                    default_structure['tests'].append(file_name)
                elif file_name.endswith('.md'):
                    default_structure['docs'].append(file_name)
                elif file_name.endswith('.json'):
                    default_structure['configs'].append(file_name)
                else:
                    default_structure['misc'].append(file_name)
            self.arrange_files_dynamic(default_structure)
        except Exception as e:
            logging.error(f"Error arranging files with default structure: {e}")
            print_with_breaker(f"Error arranging files with default structure: {e}")

    def print_file_details(self, filepath: str):
        """
        Prints the file name, extension (without dot), and MIME type.
        """
        try:
            filename = filepath.name
            file_extension = filepath.suffix.lstrip('.')  # Remove leading dot
            mime_type = self.get_mime_type(filepath)
            print(f"File: {filename}, Extension: {file_extension}, MIME Type: {mime_type}")
        except Exception as e:
            logging.error(f"Error printing file details: {e}")
            print_with_breaker(f"Error: Failed to print file details: {e}")

    def get_mime_type(self, filepath: str) -> str:
        """
        Determines the MIME type of the file.
        """
        import mimetypes
        mime_type, _ = mimetypes.guess_type(str(filepath))
        return mime_type or "Unknown"
