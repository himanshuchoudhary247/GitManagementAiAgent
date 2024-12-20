# agents/file_selector_agent.py

import logging
import json

class FileSelectorAgent:
    def __init__(self, memory):
        self.memory = memory

    def select_files(self):
        try:
            relevant_functions = self.memory.get('relevant_functions', [])
            selected_files = set()
            for func in relevant_functions:
                file_path = func.get('file')
                if file_path:
                    selected_files.add(file_path)
            self.memory.set('selected_files', list(selected_files))
            logging.info(f"Selected files for processing: {selected_files}")
            print(f"Selected files for processing: {selected_files}")
        except Exception as e:
            logging.error(f"Error in selecting files: {e}")
            print(f"Error: Failed to select files: {e}")
