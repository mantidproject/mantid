"""Defines the rules base class and helper functions
"""

class Rules(object):

    errors = []

    def apply_to_ast(self, tree):
        """
            Apply the transformation to the AST
        """
        raise NotImplementedError("Derived classes should implement apply_to_ast()")

    def apply(self, text):
        """
            Interface for applying the rules
        """
        raise NotImplementedError("Derived classes should implement apply()")

