# tests/test_file_manager_dynamic.py

import unittest
import os
import shutil
from agents.file_manager import FileManager

class TestFileManagerDynamic(unittest.TestCase):
    def setUp(self):
        # Setup a temporary repository directory with files
        self.test_repo_path = 'test_repo'
        os.makedirs(self.test_repo_path, exist_ok=True)
        # Create sample files
        self.sample_files = {
            'file1.py': "# file1.py",
            'file2.py': "# file2.py",
            'test_file1.py': "# test_file1.py",
            'test_file2.py': "# test_file2.py",
            'README.md': "# README",
            'config.json': "{}",
            'helper.py': "# helper functions",
            'agent1.py': "# agent1",
            'agent2.py': "# agent2",
            'file3.py': "# file3.py"
        }
        for file, content in self.sample_files.items():
            with open(os.path.join(self.test_repo_path, file), 'w') as f:
                f.write(content)
        self.file_manager = FileManager(self.test_repo_path)

    def tearDown(self):
        # Remove the temporary repository directory after tests
        shutil.rmtree(self.test_repo_path)

    def test_arrange_files_dynamic(self):
        # Define a proposed directory structure
        proposed_structure = {
            "src": ["file1.py", "file2.py"],
            "tests": ["test_file1.py", "test_file2.py"],
            "docs": ["README.md"],
            "configs": ["config.json"],
            "utils": ["helper.py"],
            "agents": ["agent1.py", "agent2.py"],
            "misc": ["file3.py"]
        }

        # Arrange the files
        self.file_manager.arrange_files_dynamic(proposed_structure)

        # Verify that files are moved to the correct directories
        for directory, files in proposed_structure.items():
            dir_path = os.path.join(self.test_repo_path, directory)
            self.assertTrue(os.path.isdir(dir_path))
            for file in files:
                file_path = os.path.join(dir_path, file)
                self.assertTrue(os.path.isfile(file_path))

        # Verify that no files remain in the root directory except the new directories
        remaining_files = os.listdir(self.test_repo_path)
        expected_directories = list(proposed_structure.keys())
        for file in remaining_files:
            self.assertIn(file, expected_directories)
