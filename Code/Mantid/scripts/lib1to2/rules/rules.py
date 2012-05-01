"""Defines the rules base class and helper functions
"""

class Rules(object):
        
    def apply(self, text):
        """
            Interface for applying the rules
        """
        raise NotImplementedError("Derived classes should implement apply()")

