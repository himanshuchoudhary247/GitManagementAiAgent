# main.py

import os
import subprocess
from coordinator import Coordinator
from utils import config
from utils.cli_utils import print_with_breaker

def clone_git_repo(repo_url: str, clone_path: str):
    try:
        subprocess.check_call(['git', 'clone', repo_url, clone_path])
        print_with_breaker(f"Cloned repository {repo_url} to {clone_path}")
    except subprocess.CalledProcessError as e:
        print_with_breaker(f"Failed to clone repository: {e}")
        exit(1)

def display_help():
    help_text = """
Agentic RAG System - User Guide

**Available Commands:**
1. Enter any project requirement to initiate the system.
2. Type 'undo changes' to revert the last set of changes made to the codebase.
3. Type 'restructure directories' followed by your instructions to reorganize the directory structure.
4. Type 'help' to view this user guide.
5. Type 'exit' to quit the system.

**New Features:**
- **Project Initialization:** Automatically sets up a standard directory structure for new projects.
- **Enhanced Plan Generation:** The system can rephrase and split complex queries into actionable sub-plans.
- **Safe Code Execution:** Critical operations require user confirmation before execution.
- **Undo Functionality:** Easily revert changes to maintain code integrity.
- **Improved Logging:** Clear and readable logs with breaker lines for better tracking.
- **Dynamic Directory Restructuring:** Reorganize the repository's directory structure based on user requirements.

**Security Enhancements:**
- The execution environment is restricted to prevent unauthorized operations.

**Error Handling:**
- The system provides informative feedback for better troubleshooting.

For further assistance, refer to the README.md in the project directory.
"""
    print_with_breaker(help_text)

def main():
    use_gitrepo = False  # Set to False to use local directory
    git_repo_url = 'https://github.com/yourusername/yourrepository.git'
    clone_path = './cloned_repo'
    
    # Take input directory from user
    repo_path = '/Users/sudhanshu/chat_model' #input("Enter the path to your project directory: ").strip()
    
    if not os.path.exists(repo_path):
        create_repo = input(f"The directory '{repo_path}' does not exist. Do you want to clone a Git repository here? (yes/no): ").strip().lower()
        if create_repo == 'yes':
            repo_url = input("Enter the Git repository URL to clone: ").strip()
            clone_git_repo(repo_url, repo_path)
        else:
            print_with_breaker(f"Please provide an existing directory or clone a repository first.")
            exit(1)
    
    coordinator = Coordinator(repo_path=repo_path, use_gitrepo=use_gitrepo)
    
    while True:
        command = command = input("Enter your requirement (or 'undo changes', 'restructure directories', 'help', 'exit' to quit): ").strip()
        
        if command.lower() == 'help':
            display_help()
            continue
        
        if command.lower() == 'exit':
            print_with_breaker("Exiting Agentic RAG System. Goodbye!")
            break
        
        coordinator.process_requirement(command)
        break
if __name__ == "__main__":
    main()

# # main.py

# import os
# import subprocess
# from coordinator import Coordinator
# from utils import config
# from utils.cli_utils import print_with_breaker

# def clone_git_repo(repo_url, clone_path):
#     try:
#         subprocess.check_call(['git', 'clone', repo_url, clone_path])
#         print_with_breaker(f"Cloned repository {repo_url} to {clone_path}")
#     except subprocess.CalledProcessError as e:
#         print_with_breaker(f"Failed to clone repository: {e}")
#         exit(1)

# def display_help():
#     help_text = """
# Agentic RAG System - User Guide

# **Available Commands:**
# 1. Enter any project requirement to initiate the system.
# 2. Type 'undo changes' to revert the last set of changes made to the codebase.
# 3. Type 'restructure directories' followed by your instructions to reorganize the directory structure.
# 4. Type 'help' to view this user guide.

# **New Features:**
# - **Project Initialization:** Automatically sets up a standard directory structure for new projects.
# - **Enhanced Plan Generation:** The system can rephrase and split complex queries into actionable sub-plans.
# - **Safe Code Execution:** Critical operations require user confirmation before execution.
# - **Undo Functionality:** Easily revert changes to maintain code integrity.
# - **Improved Logging:** Clear and readable logs with breaker lines for better tracking.
# - **Dynamic Directory Restructuring:** Reorganize the repository's directory structure based on user requirements.

# **Security Enhancements:**
# - The execution environment is restricted to prevent unauthorized operations.

# **Error Handling:**
# - The system provides informative feedback for better troubleshooting.

# For further assistance, refer to the README.md in the project directory.
# """
#     print_with_breaker(help_text)

# def main():
#     use_gitrepo = False  # Set to False to use local directory
#     git_repo_url = 'https://github.com/yourusername/yourrepository.git'
#     clone_path = './cloned_repo'
    
#     # Take input directory from user
#     repo_path = '/Users/sudhanshu/chat_model' #input("Enter the path to your project directory: ").strip()
    
#     if not os.path.exists(repo_path):
#         create_repo = input(f"The directory '{repo_path}' does not exist. Do you want to clone a Git repository here? (yes/no): ").strip().lower()
#         if create_repo == 'yes':
#             repo_url = input("Enter the Git repository URL to clone: ").strip()
#             clone_git_repo(repo_url, repo_path)
#         else:
#             print_with_breaker(f"Please provide an existing directory or clone a repository first.")
#             exit(1)
    
#     # command = input("Enter your requirement (or 'undo changes' or 'help' for guidance): ").strip()
#     command = "For input directory modify that directory structure and move files that can be clubed into one module in one folder." #input("Enter your requirement (or 'undo changes' to revert): ")
    
#     if command.lower() == 'help':
#         display_help()
#         return
    
#     coordinator = Coordinator(repo_path=repo_path, use_gitrepo=use_gitrepo)
#     coordinator.process_requirement(command)

# if __name__ == "__main__":
#     main()

