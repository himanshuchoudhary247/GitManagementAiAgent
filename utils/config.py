# utils/config.py

import os
from dotenv import load_dotenv

load_dotenv()  # Load environment variables from .env

# Llama 3 API Configuration
LLAMA3_API_KEY = os.getenv('LLAMA3_API_KEY')
LLAMA3_API_URL = os.getenv('LLAMA3_API_URL')

print(LLAMA3_API_KEY,LLAMA3_API_URL)
# LA-3d8a40b353dc46c183d3e346a2b691af39d9f025165a49e18226894a0d04a78f https://api.llama-api.com
# GitHub Configuration (If integrating GitHub interactions)
GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
GITHUB_REPO_NAME = os.getenv('GITHUB_REPO_NAME')
