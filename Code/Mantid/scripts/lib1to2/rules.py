"""Defines the rules for translation from API v1 -> v2
"""
import re

class Rules(object):
        
    def apply(self, text):
        """
            Interface for applying the rules
        """
        raise NotImplementedError("Derived classes should implement apply()")
        
#########################################################################################

__STRINGREPLACEMENTS__ = [
    (re.compile("from MantidFramework import \*"), "from mantid import *"),
    (re.compile("import MantidFramework"), "import mantid"),
    (re.compile("from mantidsimple import \*"), "from mantid.simpleapi import *"),
    (re.compile("from mantidsimple import"), "from mantid.simpleapi import"),
    (re.compile("import mantidsimple as"), "import mantid.simpleapi as"),
    (re.compile("import mantidsimple"), "import mantid.simpleapi as simpleapi"),
    (re.compile("mantidsimple\."), "simpleapi."),   
    (re.compile("(MantidFramework\.|)(mtd|mantid)\.initiali[z,s]e\(\)"), ""),
    (re.compile("MantidFramework\."), "mantid."),
    (re.compile("\.getSampleDetails"), ".getRun"),
    (re.compile("mtd\.settings"), "config"),
    (re.compile("mtd\.getConfigProperty"), "config.getString"),
    (re.compile("(mtd|mantid).sendLogMessage"), "logger.notice")
]

class SimpleStringReplace(Rules):
    """
        Implements any rules that are simply a matter of matching a pattern
        and replacing it with a fixed string
    """
    _rules = None
    
    def __init__(self):
        Rules.__init__(self)
        self._rules = __STRINGREPLACEMENTS__
        
    def apply(self, text):
        """
        Returns a replacement string for the input text
        with the simple string relpacements definined by the regexes 
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """
        for pattern, replacement in self._rules:
            text = pattern.sub(replacement, text)            
        return text


#########################################################################################

__FUNCTION_CALL_REGEXSTR = r"""(|\w*\s*) # Any variable on the lhs of the function call
                     (|=)(\s*) # The equals sign
                     (\w*) # Whitespace directly on the other side of the equals
                     \((.*?)\) # The function call arguments (without the parentheses)
                     (\.workspace\(\)|) # An optional trailing .workspace() call""" 

__FUNCTION_CALL_REGEX__ = re.compile(__FUNCTION_CALL_REGEXSTR, re.VERBOSE) 

class SimpleAPIFunctionCallReplace(Rules):
    
    func_regex = __FUNCTION_CALL_REGEX__
    
    def __init__(self):
        Rules.__init__(self)
        
    def apply(self, text):
        """
        Returns a replacement string for the input text
        where the simple API function calls are replaced by improved
        ones for the new API. 
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """
        # Now more complex replacements for simple API calls going line by line
        if "\\r\\n" in text:
            eol = "\r\n"
        elif "\\r" in text:
            eol = "\r"
        else:
            eol = "\n"
        
        inputlines = text.splitlines()
        updated_lines = []
        for line in inputlines:
            match = self.func_regex.match(line) 
            if match is not None:
                updated_line = self.apply_func_call_rules(line, match)
            else:
                updated_line = line
            updated_lines.append(updated_line)
                
        replaced_text = eol.join(updated_lines)
        if text.endswith(eol):
            replaced_text += eol
        return replaced_text
        
    def apply_func_call_rules(self, inputline, match):
        """
            Apply rules to transform the inputline, with
            the given regex MatchObject to the new API
        """
        return inputline
    
    