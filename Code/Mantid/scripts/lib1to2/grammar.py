"""
Defines the grammar translation from version 1 to version 2 of Mantid's Python API.
"""
import messages 

import re

__RULES__ = [
    ("from MantidFramework import \*", "from mantid import *"),
    ("import MantidFramework", "import mantid"),
    ("from mantidsimple import \*", "from mantid.simpleapi import *"),
    ("(MantidFramework\.|)(mtd|mantid)\.initiali[z,s]e\(\)", ""),
    ("\.getSampleDetails", ".getRun"),
    ("mtd\.settings", "config"),
    ("mtd\.getConfigProperty", "config.getString"),
    ("(mtd|mantid)\.sendLogMessage", "logger.notice")
]

class Rules(object):
    
    _rules = None
    
    def __init__(self):
        """Compiles the string rules to regexes"""
        self._rules = []
        for key, value in __RULES__:
            self._rules.append(([re.compile(key), value]))
        
    def apply(self, text):
        """
        Returns a replacement string for the input text
        according to the defined Rules. 
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """
        for pattern, replacement in self._rules:
            text = pattern.sub(replacement, text)
        return text
        
        
class Grammar(object):
    """
    Translation from v1->v2 of the Python API
    """
    _rules = None
    
    def __init__(self):
        self._rules = Rules()
    
    def translate(self, input):
        """
        Translates the input string, assuming it contains code written in
        version 1 of Mantid's Python API, to version 2 of the PythonAPI
        """
        translated = self._rules.apply(input)
        return translated