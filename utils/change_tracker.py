# utils/change_tracker.py

import json
import os
from datetime import datetime
import logging

class ChangeTracker:
    def __init__(self, repo_path):
        self.repo_path = os.path.abspath(repo_path)
        self.log_file = os.path.join(self.repo_path, 'change_log.json')
        self.ensure_log_file()

    def ensure_log_file(self):
        if not os.path.exists(self.log_file):
            with open(self.log_file, 'w') as f:
                json.dump([], f, indent=4)

    def log_change(self, agent, action, file, content_before, content_after):
        try:
            change_entry = {
                "timestamp": datetime.utcnow().isoformat() + "Z",
                "agent": agent,
                "action": action,
                "file": file,
                "content_before": content_before,
                "content_after": content_after
            }
            with open(self.log_file, 'r+') as f:
                data = json.load(f)
                data.append(change_entry)
                f.seek(0)
                json.dump(data, f, indent=4)
            logging.info(f"Logged change: {change_entry}")
        except Exception as e:
            logging.error(f"Failed to log change: {e}")

    def get_changes(self):
        try:
            with open(self.log_file, 'r') as f:
                return json.load(f)
        except Exception as e:
            logging.error(f"Failed to read change log: {e}")
            return []

    def clear_change_log(self):
        try:
            with open(self.log_file, 'w') as f:
                json.dump([], f, indent=4)
            logging.info("Cleared change log.")
        except Exception as e:
            logging.error(f"Failed to clear change log: {e}")
