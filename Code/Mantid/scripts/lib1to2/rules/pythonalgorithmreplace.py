"""Defines the rules for translation of Python algorithms
from v1 to v2
"""
import rules

class PythonAlgorithmReplace(rules.Rules):

    def __init__(self):
        rules.Rules.__init__(self)

    def apply(self, text):
        """
        Returns a replacement string for the input text
        where the python algorithm calls have been replaced
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """

        return text
