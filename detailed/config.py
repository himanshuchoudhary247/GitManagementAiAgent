# config.py

import os

# Path to the local GitHub repository directory
REPO_PATH = '/Users/sudhanshu/demo-auth-repo/demo-auth-repo'  # Update this path to your cloned repository

# Llama 3 API Configuration
LLAMA3_API_KEY = os.getenv('LLAMA3_API_KEY')  # Set as environment variable
LLAMA3_API_URL = os.getenv('LLAMA3_API_URL')  # Set as environment variable

# GitHub Configuration (If integrating GitHub interactions)
GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')  # Set as environment variable
GITHUB_REPO_NAME = 'username/repository'  # Replace with your repository name


# export LLAMA3_API_KEY="LA-3d8a40b353dc46c183d3e346a2b691af39d9f025165a49e18226894a0d04a7f8"
# export LLAMA3_API_URL='https://api.llama-api.com'
# export GITHUB_TOKEN='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'  
# export GITHUB_REPO_NAME='/himanshuchoudhary247/demo-auth-repo'  
