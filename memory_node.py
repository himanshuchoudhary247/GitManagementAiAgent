# memory_node.py

import json
import os
import logging

class MemoryNode:
    def __init__(self, node_name: str):
        self.node_name = node_name
        self.memory_file = f"{self.node_name.lower()}_memory.json"
        self.memory = self.load_memory()

    def load_memory(self) -> dict:
        """
        Loads the memory from a JSON file.

        Returns:
        - dict: The loaded memory.
        """
        try:
            if os.path.exists(self.memory_file):
                with open(self.memory_file, 'r') as f:
                    return json.load(f)
            else:
                return {}
        except Exception as e:
            logging.error(f"Error loading memory for {self.node_name}: {e}")
            return {}

    def save_memory(self):
        """
        Saves the current memory to a JSON file.
        """
        try:
            with open(self.memory_file, 'w') as f:
                json.dump(self.memory, f, indent=4)
        except Exception as e:
            logging.error(f"Error saving memory for {self.node_name}: {e}")

    def set(self, key: str, value):
        """
        Sets a key-value pair in memory.

        Parameters:
        - key (str): The key to set.
        - value: The value to associate with the key.
        """
        self.memory[key] = value
        self.save_memory()

    def get(self, key: str, default=None):
        """
        Retrieves a value from memory.

        Parameters:
        - key (str): The key to retrieve.
        - default: The default value if the key does not exist.

        Returns:
        - The value associated with the key or the default.
        """
        return self.memory.get(key, default)
