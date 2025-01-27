# utils/centralized_memory.py

import json
import os
import logging

class CentralizedMemory:
    def __init__(self, repo_path):
        self.repo_path = os.path.abspath(repo_path)
        self.memory_file = os.path.join(self.repo_path, 'centralized_memory.json')
        self.memory = {}
        self.load_memory()

    def load_memory(self):
        if os.path.exists(self.memory_file):
            try:
                with open(self.memory_file, 'r') as f:
                    self.memory = json.load(f)
                logging.info("Centralized memory loaded successfully.")
            except Exception as e:
                logging.error(f"Failed to load centralized memory: {e}")
                self.memory = {}
        else:
            self.memory = {}
            self.save_memory()

    def save_memory(self):
        try:
            with open(self.memory_file, 'w') as f:
                json.dump(self.memory, f, indent=4)
            logging.info("Centralized memory saved successfully.")
        except Exception as e:
            logging.error(f"Failed to save centralized memory: {e}")

    def set(self, agent_name, key, value):
        if agent_name not in self.memory:
            self.memory[agent_name] = {}
        self.memory[agent_name][key] = value
        self.save_memory()

    def get(self, agent_name, key, default=None):
        return self.memory.get(agent_name, {}).get(key, default)

    def update(self, agent_name, key, value):
        if agent_name not in self.memory:
            self.memory[agent_name] = {}
        if isinstance(self.memory[agent_name].get(key), dict) and isinstance(value, dict):
            self.memory[agent_name][key].update(value)
        else:
            self.memory[agent_name][key] = value
        self.save_memory()
