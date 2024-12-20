# main.py

import os
import subprocess
from coordinator import Coordinator
from utils import config  # Updated import

def clone_git_repo(repo_url, clone_path):
    try:
        subprocess.check_call(['git', 'clone', repo_url, clone_path])
        print(f"Cloned repository {repo_url} to {clone_path}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to clone repository: {e}")
        exit(1)

def main():
    # Configuration variables
    use_gitrepo = False  # Set to False to use local directory
    git_repo_url = 'https://github.com/yourusername/yourrepository.git'
    clone_path = './cloned_repo'
    local_dir_path = '/Users/sudhanshu/demo-auth-repo/demo-auth-repo'
    
    # User-provided requirement (can be modified to accept user input)
    requirement = "Create a file /fs.py thath has a fact_sort function.Fact sort should take a list of integers and return a sorted list."
    
    # Determine the repository path
    if use_gitrepo:
        # Clone the repository if it hasn't been cloned already
        if not os.path.exists(clone_path):
            clone_git_repo(git_repo_url, clone_path)
        repo_path = clone_path
    else:
        repo_path = local_dir_path
        if not os.path.exists(repo_path):
            print(f"Local directory {repo_path} does not exist.")
            exit(1)
    
    # Initialize Coordinator
    coordinator = Coordinator(repo_path=repo_path, use_gitrepo=use_gitrepo)
    
    # Process the requirement
    coordinator.process_requirement(requirement)

if __name__ == "__main__":
    main()
