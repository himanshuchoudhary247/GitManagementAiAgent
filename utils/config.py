# utils/config.py

import os
from dotenv import load_dotenv

load_dotenv()  # Load environment variables from .env

# Llama 3 API Configuration
LLAMA3_API_KEY = os.getenv('LLAMA3_API_KEY')
LLAMA3_API_URL = os.getenv('LLAMA3_API_URL')

# GitHub Configuration (If integrating GitHub interactions)
GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
GITHUB_REPO_NAME = os.getenv('GITHUB_REPO_NAME')
