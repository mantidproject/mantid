"""
Defines functions for displaying messages to the user
"""

__QUIET__ = False

def quiet():
    """
    """
    return __QUIET__

def notify(msg):
    """
    Prints a message to the console
    """
    if not quiet():
        print msg