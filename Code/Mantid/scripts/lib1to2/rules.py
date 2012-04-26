"""Defines the rules for translation from API v1 -> v2
"""
import re
import mantid

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
                     (|=\s*) # The equals sign including any whitespace on the right
                     (\w*) # The function name
                     \((.*?)\) # The function call arguments (without the parentheses)
                     (\.workspace\(\)|) # An optional trailing .workspace() call""" 

__FUNCTION_CALL_REGEX__ = re.compile(__FUNCTION_CALL_REGEXSTR, re.VERBOSE)

__MANTID_ALGS__ = mantid.api.AlgorithmFactory.getRegisteredAlgorithms(True)

class SimpleAPIFunctionCallReplace(Rules):
    
    func_regex = __FUNCTION_CALL_REGEX__
    current_line = None
    
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
        the given regex MatchObject to the new API.
            @param match :: A re.Match object containing 5 groups:
                              (1) - A possible variable name or empty string;
                              (2) - A possible equals sign, with surrounding white space or an empty string;
                              (3) - The name of the function call;
                              (4) - The exact string of arguments as passed to the function call;
                              (5) - A possible .workspace() call or empty string
        """
        self.current_line = inputline
        print match.groups()
        func_name = self.get_function_name(match)
        if not self.is_mantid_algorithm(func_name):
            return inputline
        lhs_var = self.get_lhs_var_name(match)
        if lhs_var == "":
            # Simple case of no return
            replacement = self.apply_replace_for_no_return(func_name, self.get_function_args(match))
            
        
        
        return replacement
        
    def apply_replace_for_no_return(self, name, arguments):
        """Takes a function name & arguments from the input
        and reconstructs the call in the new api"""
        replacement = name + "()"
        alg = mantid.api.AlgorithmManager.Instance().createUnmanaged(name)
        alg.initialize()
        ordered_names = alg.orderedProperties()
        
        keyword_found = False
        kwargs = {}
        for index, arg in enumerate(arguments):
            if type(arg) == tuple:
                keyword_found = True
                name = arg[0]
                value = arg[1]
            else:
                if keyword_found == True:
                    raise ValueError("Found a positional argument after a keyword in line '%s'" % self.current_line)
                name = ordered_names[index]
                value = arg
            print type(value)
            name = name.strip()
            value = value.strip()
            kwargs[name] = value
        print kwargs
        return replacement
        
    def get_lhs_var_name(self, match):
        """Returns the lhs variable name from the match or an empty string"""
        return match.group(1)
    
    def get_function_name(self, match):
        """Returns the name of the function called"""
        name = match.group(3)
        if name == '':
            raise ValueError("The regex has returned an empty string for the function name in line '%s'. Check regex" % self.current_line)
        return name

    def get_function_args(self, match):
        """Returns the function arguments required for the new API"""
        argstring = match.group(4)
        arg_values = self.split_up_argstring(argstring)
        return arg_values
    
    def split_up_argstring(self, argstring):
        """
        Returns a list of arguments inside the call. 
        Note that if a tuple is returned as one of the elements of the list this gives the keyword
        """
        arg_values = argstring.split(",")
        arguments = []
        for item in arg_values:
            if '=' in item:
                name, value = item.split("=")
                arguments.append(tuple([name,value]))
            else:
                arguments.append(item)
        return arguments
    
    def is_mantid_algorithm(self, name):
        """Does the name string match a Mantid algorithm name"""
        return name in __MANTID_ALGS__
        