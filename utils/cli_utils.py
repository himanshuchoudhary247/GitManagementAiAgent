# utils/cli_utils.py

def print_with_breaker(message: str):
    """
    Prints the message with breaker lines for better readability.

    Parameters:
    - message (str): The message to print.
    """
    breaker = "-" * 50
    print(f"\n{breaker}\n{message}\n{breaker}\n")
