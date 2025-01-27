# tests/test_directory_structuring_agent.py

import unittest
from unittest.mock import patch
import os
import shutil
from agents.directory_structuring_agent import DirectoryStructuringAgent
from utils.llama3_client import Llama3Client

class TestDirectoryStructuringAgent(unittest.TestCase):
    def setUp(self):
        # Setup a temporary repository directory
        self.test_repo_path = 'test_repo'
        os.makedirs(self.test_repo_path, exist_ok=True)
        # Create sample files and directories
        os.makedirs(os.path.join(self.test_repo_path, 'module1'), exist_ok=True)
        with open(os.path.join(self.test_repo_path, 'module1', 'file1.py'), 'w') as f:
            f.write("# file1.py")
        with open(os.path.join(self.test_repo_path, 'module1', 'file2.py'), 'w') as f:
            f.write("# file2.py")
        os.makedirs(os.path.join(self.test_repo_path, 'module2'), exist_ok=True)
        with open(os.path.join(self.test_repo_path, 'module2', 'file3.py'), 'w') as f:
            f.write("# file3.py")
        self.llama3_client = Llama3Client()
        self.agent = DirectoryStructuringAgent(self.llama3_client, self.test_repo_path)

    def tearDown(self):
        # Remove the temporary repository directory after tests
        shutil.rmtree(self.test_repo_path)

    @patch.object(Llama3Client, 'generate')
    def test_propose_restructuring(self, mock_generate):
        # Mock the response from Llama3Client
        mock_response = '''
        {
            "src": ["file1.py", "file2.py"],
            "tests": ["test_file1.py", "test_file2.py"],
            "docs": ["README.md"],
            "configs": ["config.json"],
            "utils": ["helper.py"],
            "agents": ["agent1.py", "agent2.py"],
            "misc": ["file3.py"]
        }
        '''
        mock_generate.return_value = mock_response

        user_query = "Optimize the directory structure for better scalability and maintainability."
        proposed_structure = self.agent.propose_restructuring(user_query)

        expected_structure = {
            "src": ["file1.py", "file2.py"],
            "tests": ["test_file1.py", "test_file2.py"],
            "docs": ["README.md"],
            "configs": ["config.json"],
            "utils": ["helper.py"],
            "agents": ["agent1.py", "agent2.py"],
            "misc": ["file3.py"]
        }

        self.assertEqual(proposed_structure, expected_structure)
