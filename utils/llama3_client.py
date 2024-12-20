# utils/llama3_client.py

import openai  # Ensure you have the openai package installed
from utils import config
import logging
import sys
import json

# Configure logging
logging.basicConfig(
    filename='agentic_rag_system.log',
    level=logging.DEBUG,  # Set to DEBUG for more detailed logs
    format='%(asctime)s:%(levelname)s:%(message)s'
)

class Llama3Client:
    def __init__(self):
        if not config.LLAMA3_API_KEY or not config.LLAMA3_API_URL:
            logging.error("Llama3 API key and base URL must be provided.")
            sys.exit("Error: Llama3 API key and base URL must be provided.")
        try:
            openai.api_key = config.LLAMA3_API_KEY
            openai.api_base = config.LLAMA3_API_URL
            logging.info("Initialized Llama3Client successfully.")
        except Exception as e:
            logging.error(f"Failed to initialize Llama3 client: {e}")
            sys.exit(f"Error: Failed to initialize Llama3 client: {e}")

    def generate(self, prompt, max_tokens=150, temperature=0.7):
        try:
            response = openai.ChatCompletion.create(
                model="gpt-4",  # Replace with the appropriate model name
                messages=[
                    {"role": "system", "content": "You are a helpful assistant."},
                    {"role": "user", "content": prompt}
                ],
                max_tokens=max_tokens,
                temperature=temperature
            )
            # Log the raw response for debugging
            logging.debug(f"Received response: {response}")
            # Extract the generated content
            generated_content = response.choices[0].message.content.strip()
            logging.debug(f"Generated content: {generated_content}")
            return generated_content
        except Exception as e:
            logging.error(f"Llama 3 API Error: {e}")
            return ""
