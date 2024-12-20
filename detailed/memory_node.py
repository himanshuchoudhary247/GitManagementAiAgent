# memory_node.py

class MemoryNode:
    def __init__(self, agent_name):
        self.storage = {}
        self.agent_name = agent_name

    def set(self, key, value):
        self.storage[key] = value

    def get(self, key, default=None):
        return self.storage.get(key, default)

    def update(self, key, value):
        if key in self.storage and isinstance(self.storage[key], dict):
            self.storage[key].update(value)
        else:
            self.storage[key] = value

    def display(self):
        print(f"Memory for {self.agent_name}:")
        for key, value in self.storage.items():
            print(f"{key}: {value}")
