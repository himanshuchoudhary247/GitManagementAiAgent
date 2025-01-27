# utils/github_handler.py

from github import Github, GithubException
import os
import logging
from utils.cli_utils import print_with_breaker

class GitHubHandler:
    def __init__(self):
        self.token = os.getenv('GITHUB_TOKEN')
        self.repo_name = os.getenv('GITHUB_REPO_NAME')
        if not self.token or not self.repo_name:
            raise ValueError("GitHub token and repository name must be set in environment variables.")
        self.github = Github(self.token)
        self.repo = self.github.get_repo(self.repo_name)

    def commit_changes(self, branch_name: str, commit_message: str):
        """
        Commits changes to a specified branch.

        Parameters:
        - branch_name (str): The name of the branch to commit to.
        - commit_message (str): The commit message.
        """
        try:
            source = self.repo.get_branch("main")
            self.repo.create_git_ref(ref=f"refs/heads/{branch_name}", sha=source.commit.sha)
            # Add commit logic here (e.g., using the GitHub API to commit files)
            print_with_breaker(f"Committed changes to branch {branch_name}.")
            logging.info(f"Committed changes to branch {branch_name}.")
        except GithubException as e:
            logging.error(f"GitHub Commit Error: {e}")
            print_with_breaker(f"Error: Failed to commit changes: {e}")
            raise e

    def create_pull_request(self, title: str, body: str, head: str, base: str):
        """
        Creates a pull request from head to base.

        Parameters:
        - title (str): The title of the pull request.
        - body (str): The body/content of the pull request.
        - head (str): The name of the branch where changes are implemented.
        - base (str): The name of the branch you want the changes pulled into.
        """
        try:
            pr = self.repo.create_pull(title=title, body=body, head=head, base=base)
            print_with_breaker(f"Pull request created: {pr.html_url}")
            logging.info(f"Pull request created: {pr.html_url}")
        except GithubException as e:
            logging.error(f"GitHub Pull Request Error: {e}")
            print_with_breaker(f"Error: Failed to create pull request: {e}")
            raise e
