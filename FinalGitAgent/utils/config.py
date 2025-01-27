# utils/config.py

import os
from dotenv import load_dotenv

load_dotenv()

LLAMA3_API_KEY = os.getenv('LLAMA3_API_KEY')
LLAMA3_API_URL = os.getenv('LLAMA3_API_URL')
# LA-3d8a40b353dc46c183d3e346a2b691af39d9f025165a49e18226894a0d04a7f8 https://api.llama-api.com

GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
GITHUB_REPO_NAME = os.getenv('GITHUB_REPO_NAME')

# LLAMA3_API_KEY=your_llama3_api_key
# LLAMA3_API_URL=your_llama3_api_url
# GITHUB_TOKEN=your_github_token
# GITHUB_REPO_NAME=your_github_repo_name
