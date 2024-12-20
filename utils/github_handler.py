# utils/github_handler.py

from github import Github
from utils import config
import logging
import os
import sys
import subprocess

class GitHubHandler:
    def __init__(self):
        try:
            self.g = Github(config.GITHUB_TOKEN)
            self.repo = self.g.get_repo(config.GITHUB_REPO_NAME)
            logging.info(f"Connected to GitHub repository: {config.GITHUB_REPO_NAME}")
        except Exception as e:
            logging.error(f"Failed to connect to GitHub: {e}")
            sys.exit(f"Error: Failed to connect to GitHub: {e}")

    def commit_changes(self, branch_name, commit_message):
        try:
            clone_path = './cloned_repo'
            if not os.path.exists(clone_path):
                self.clone_repo()

            # Checkout to the new branch
            subprocess.check_call(['git', '-C', clone_path, 'checkout', '-b', branch_name])

            # Stage all changes
            subprocess.check_call(['git', '-C', clone_path, 'add', '.'])

            # Commit changes
            subprocess.check_call(['git', '-C', clone_path, 'commit', '-m', commit_message])

            # Push changes to the new branch
            subprocess.check_call(['git', '-C', clone_path, 'push', '--set-upstream', 'origin', branch_name])

            logging.info(f"Committed changes to branch '{branch_name}'.")
            print(f"Committed changes to branch '{branch_name}'.")
        except subprocess.CalledProcessError as e:
            logging.error(f"Git command failed: {e}")
            print(f"Error: Git command failed: {e}")
        except Exception as e:
            logging.error(f"Failed to commit changes: {e}")
            print(f"Error: Failed to commit changes: {e}")

    def clone_repo(self):
        try:
            subprocess.check_call(['git', 'clone', self.repo.clone_url, './cloned_repo'])
            logging.info(f"Cloned repository to './cloned_repo'.")
            print(f"Cloned repository to './cloned_repo'.")
        except subprocess.CalledProcessError as e:
            logging.error(f"Failed to clone repository: {e}")
            print(f"Error: Failed to clone repository: {e}")
            sys.exit(1)

    def create_pull_request(self, title, body, head, base):
        try:
            pr = self.repo.create_pull(title=title, body=body, head=head, base=base)
            logging.info(f"Created pull request: {pr.html_url}")
            print(f"Created pull request: {pr.html_url}")
        except Exception as e:
            logging.error(f"Failed to create pull request: {e}")
            print(f"Error: Failed to create pull request: {e}")
