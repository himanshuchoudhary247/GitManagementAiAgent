# main.py

import os
import subprocess
from coordinator import Coordinator
from utils import config
import sys

def clone_git_repo(repo_url, clone_path):
    try:
        subprocess.check_call(['git', 'clone', repo_url, clone_path])
        print(f"Cloned repository {repo_url} to {clone_path}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to clone repository: {e}")
        sys.exit(1)

def main():
    use_gitrepo = False  # Set to True to use GitHub repository
    git_repo_url = 'https://github.com/yourusername/yourrepository.git'  # Replace with your repository URL
    clone_path = './cloned_repo'
    
    # Prompt user for the repository path
    # repo_path_input = input("Enter the path to the target repository: ").strip()
    repo_path_input='/Users/sudhanshu/chat_model'
    requirement = "In agent.py file write a function CodeWriter that takes user's natural query and return the final answer as llm output."
    #input("Enter your requirement (or 'undo changes' to revert): ")

    if use_gitrepo:
        if not os.path.exists(clone_path):
            clone_git_repo(git_repo_url, clone_path)
        repo_path = clone_path
    else:
        if not repo_path_input:
            print("Error: No repository path provided.")
            sys.exit(1)
        repo_path = os.path.abspath(repo_path_input)
        if not os.path.exists(repo_path):
            print(f"Error: The directory '{repo_path}' does not exist.")
            sys.exit(1)
    
    # Prevent the agentic system from pointing to its own directory
    agentic_system_path = os.path.abspath(os.path.dirname(__file__))
    if os.path.commonpath([agentic_system_path, repo_path]) == agentic_system_path:
        print("Error: The target repository path cannot be the agentic system's own directory.")
        sys.exit(1)
    
    # Initialize Coordinator with the validated repo_path
    coordinator = Coordinator(repo_path=repo_path, use_gitrepo=use_gitrepo)
    coordinator.process_requirement(requirement=requirement)  # You can modify this as needed

if __name__ == "__main__":
    main()

# import os
# import subprocess
# from coordinator import Coordinator
# from utils import config

# def clone_git_repo(repo_url, clone_path):
#     try:
#         subprocess.check_call(['git', 'clone', repo_url, clone_path])
#         print(f"Cloned repository {repo_url} to {clone_path}")
#     except subprocess.CalledProcessError as e:
#         print(f"Failed to clone repository: {e}")
#         exit(1)

# def main():
#     use_gitrepo = False  # Set to False to use local directory
#     git_repo_url = 'https://github.com/yourusername/yourrepository.git'
#     clone_path = './cloned_repo'
#     local_dir_path = '/Users/sudhanshu/chat_model'  # Update to your target directory

#     requirement = "In code writeing agent.py file write a function that takes user query to write the code and return the final answer."#input("Enter your requirement (or 'undo changes' to revert): ")

#     if use_gitrepo:
#         if not os.path.exists(clone_path):
#             clone_git_repo(git_repo_url, clone_path)
#         repo_path = clone_path
#     else:
#         repo_path = local_dir_path
#         if not os.path.exists(repo_path):
#             print(f"Local directory {repo_path} does not exist.")
#             exit(1)

#     coordinator = Coordinator(repo_path=repo_path, use_gitrepo=use_gitrepo)
#     coordinator.process_requirement(requirement)

# if __name__ == "__main__":
#     main()
