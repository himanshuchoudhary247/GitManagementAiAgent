# utils/base_agent.py

import logging

class BaseAgent:
    def __init__(self, llama3_client, centralized_memory, name="BaseAgent"):
        self.llama3_client = llama3_client
        self.centralized_memory = centralized_memory
        self.name = name
        self.setup_logger()
    
    def setup_logger(self):
        self.logger = logging.getLogger(self.name)
        handler = logging.StreamHandler()
        formatter = logging.Formatter('%(asctime)s:%(levelname)s:%(message)s')
        handler.setFormatter(formatter)
        if not self.logger.handlers:
            self.logger.addHandler(handler)
        self.logger.setLevel(logging.DEBUG)
    
    def log_info(self, message):
        self.logger.info(message)
    
    def log_warning(self, message):
        self.logger.warning(message)
    
    def log_error(self, message):
        self.logger.error(message)
    
    def handle_exception(self, exception, attempt):
        self.log_error(f"Exception occurred on attempt {attempt}: {exception}")
    
    def execute(self, *args, **kwargs):
        raise NotImplementedError("Each agent must implement the execute method.")
