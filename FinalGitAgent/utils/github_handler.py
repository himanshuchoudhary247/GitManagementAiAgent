# utils/github_handler.py

from github import Github, GithubException
import os
import logging

class GitHubHandler:
    def __init__(self):
        self.token = os.getenv('GITHUB_TOKEN')
        self.repo_name = os.getenv('GITHUB_REPO_NAME')  # Format: 'username/repo'
        if not self.token or not self.repo_name:
            raise ValueError("GitHub token and repository name must be set in environment variables.")
        self.github = Github(self.token)
        try:
            self.repo = self.github.get_repo(self.repo_name)
        except GithubException as e:
            logging.error(f"GitHub Repository Error: {e}")
            raise e

    def commit_changes(self, branch_name, commit_message):
        try:
            source = self.repo.get_branch("main")
            self.repo.create_git_ref(ref=f"refs/heads/{branch_name}", sha=source.commit.sha)
        except GithubException as e:
            if e.status == 422:
                logging.info(f"Branch '{branch_name}' already exists.")
            else:
                logging.error(f"Error creating branch: {e}")
                raise e

        try:
            with open('README.md', 'r') as f:
                content = f.read()
            self.repo.update_file(path='README.md',
                                  message=commit_message,
                                  content=content,
                                  sha=self.repo.get_contents('README.md').sha,
                                  branch=branch_name)
            logging.info(f"Committed changes to branch '{branch_name}'.")
        except GithubException as e:
            logging.error(f"Error committing changes: {e}")
            raise e

    def create_pull_request(self, title, body, head, base):
        try:
            pr = self.repo.create_pull(title=title, body=body, head=head, base=base)
            logging.info(f"Pull request created: {pr.html_url}")
        except GithubException as e:
            logging.error(f"Error creating pull request: {e}")
            raise e
