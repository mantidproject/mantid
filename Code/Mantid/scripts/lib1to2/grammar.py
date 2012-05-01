"""
Defines the grammar translation from version 1 to version 2 of Mantid's Python API.
"""
import messages 
import rules
        
class Grammar(object):
    """
    Translation from v1->v2 of the Python API
    """
    
    def __init__(self):
        pass
    
    def translate(self, input):
        """
        Translates the input string, assuming it contains code written in
        version 1 of Mantid's Python API, to version 2 of the PythonAPI
        """
        string_replace = rules.SimpleStringReplace()
        translated = string_replace.apply(input)
       
        api_call_replace = rules.SimpleAPIFunctionCallReplace()
        translated = api_call_replace.apply(translated)

        return translated