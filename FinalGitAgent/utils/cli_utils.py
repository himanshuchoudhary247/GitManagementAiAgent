# utils/cli_utils.py

def print_with_breaker(message, breaker="---"):
    """
    Prints a message with a breaker line before and after.

    :param message: The message to print.
    :param breaker: The breaker string.
    """
    print(breaker)
    print(message)
    print(breaker)