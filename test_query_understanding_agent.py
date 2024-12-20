# test_query_understanding_agent.py

from agents.query_understanding_agent import QueryUnderstandingAgent
from memory_node import MemoryNode
from utils.llama3_client import Llama3Client

def test_query_understanding():
    memory = MemoryNode("QueryUnderstandingAgent")
    llama3_client = Llama3Client()
    agent = QueryUnderstandingAgent(llama3_client, memory)
    
    user_query = "Add a new function `authenticate_user` to handle OAuth2 authentication in the `auth/login.py` module."
    agent.understand_query(user_query)
    memory.display()

if __name__ == "__main__":
    test_query_understanding()
