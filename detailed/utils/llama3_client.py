# utils/llama3_client.py

from openai import OpenAI
import config
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
            logging.error("OpenAI API key and base URL must be provided.")
            sys.exit("Error: OpenAI API key and base URL must be provided.")
        try:
            self.client = OpenAI(
                api_key=config.LLAMA3_API_KEY,
                base_url=config.LLAMA3_API_URL
            )
            logging.info("Initialized Llama3Client successfully.")
        except Exception as e:
            logging.error(f"Failed to initialize OpenAI client: {e}")
            sys.exit(f"Error: Failed to initialize OpenAI client: {e}")

    def generate(self, prompt, max_tokens=150, temperature=0.7):
        try:
            response = self.client.chat.completions.create(
                model="llama3.1-70b",
                messages=[
                    {"role": "system", "content": "Assistant is a large language model trained by OpenAI."},
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
