# utils/plan_tracker.py

import json
import os
from datetime import datetime
import logging

class PlanTracker:
    def __init__(self, repo_path):
        self.repo_path = os.path.abspath(repo_path)
        self.plan_file = os.path.join(self.repo_path, 'plan_tracker.json')
        self.plans = []
        self.load_plans()

    def load_plans(self):
        if os.path.exists(self.plan_file):
            try:
                with open(self.plan_file, 'r') as f:
                    self.plans = json.load(f)
                logging.info("Plan tracker loaded successfully.")
            except Exception as e:
                logging.error(f"Failed to load plan tracker: {e}")
                self.plans = []
        else:
            self.plans = []
            self.save_plans()

    def save_plans(self):
        try:
            with open(self.plan_file, 'w') as f:
                json.dump(self.plans, f, indent=4)
            logging.info("Plan tracker saved successfully.")
        except Exception as e:
            logging.error(f"Failed to save plan tracker: {e}")

    def add_plan(self, plan_name, sub_plans):
        plan_entry = {
            "plan_name": plan_name,
            "sub_plans": [{"name": sp, "status": "pending"} for sp in sub_plans],
            "timestamp": datetime.utcnow().isoformat() + "Z"
        }
        self.plans.append(plan_entry)
        self.save_plans()
        logging.info(f"Added new plan: {plan_name}")

    def update_sub_plan_status(self, plan_name, sub_plan_name, status):
        for plan in self.plans:
            if plan["plan_name"] == plan_name:
                for sub_plan in plan["sub_plans"]:
                    if sub_plan["name"] == sub_plan_name:
                        sub_plan["status"] = status
                        self.save_plans()
                        logging.info(f"Updated sub-plan '{sub_plan_name}' to status '{status}' in plan '{plan_name}'.")
                        return
        logging.warning(f"Plan '{plan_name}' or sub-plan '{sub_plan_name}' not found.")

    def get_pending_sub_plans(self):
        pending = []
        for plan in self.plans:
            for sub_plan in plan["sub_plans"]:
                if sub_plan["status"] == "pending":
                    pending.append({"plan_name": plan["plan_name"], "sub_plan_name": sub_plan["name"]})
        return pending

    def mark_sub_plan_completed(self, plan_name, sub_plan_name):
        self.update_sub_plan_status(plan_name, sub_plan_name, "completed")

    def has_pending_sub_plans(self):
        return len(self.get_pending_sub_plans()) > 0
