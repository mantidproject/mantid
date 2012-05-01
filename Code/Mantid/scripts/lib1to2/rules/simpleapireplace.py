"""Defines the rules for translation of simple API function calls
from v1 to v2
"""
import rules
import re

import mantid

__FUNCTION_CALL_REGEXSTR = r"""(|\w*\s*) # Any variable on the lhs of the function call
                     (|=\s*) # The equals sign including any whitespace on the right
                     (\w*) # The function name
                     \((.*?)\) # The function call arguments (without the parentheses)
                     (\.workspace\(\)|) # An optional trailing .workspace() call""" 

__FUNCTION_CALL_REGEX__ = re.compile(__FUNCTION_CALL_REGEXSTR, re.VERBOSE)

__WHITESPACE_REGEX__ = re.compile("(\s*).*")

__MANTID_ALGS__ = mantid.api.AlgorithmFactory.getRegisteredAlgorithms(True)

class SimpleAPIFunctionCallReplace(rules.Rules):
    
    func_regex = __FUNCTION_CALL_REGEX__
    current_line = None
    
    def __init__(self):
        rules.Rules.__init__(self)
        
    def apply(self, text):
        """
        Returns a replacement string for the input text
        where the simple API function calls are replaced by improved
        ones for the new API. 
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """
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
        func_name = self.get_function_name(match)
        if not self.is_mantid_algorithm(func_name):
            return inputline
        lhs_var = self.get_lhs_var_name(match)
        if lhs_var.strip() == "":
            # Simple case of no return
            replacement = self.apply_replace_for_no_return(func_name, self.get_function_args(match))
        else:
            raise RuntimeError("No rules implemented for return value from simple API function on line '%s'" % self.current_line)
        
        replacement = self.fix_indentation(replacement)
        return replacement
        
    def apply_replace_for_no_return(self, name, arguments):
        """Takes a function name & arguments from the input
        and reconstructs the call in the new api"""
        alg = mantid.api.AlgorithmManager.Instance().createUnmanaged(name)
        alg.initialize()
        kwargs = self.transform_all_to_kwargs(alg, arguments)
        # These will not appear in the final list
        output_args = alg.outputProperties()
        # The one that appears in the current argument list as an output needs to go on the LHS of the '='
        lhs_vars = self.build_lhs_vars(kwargs, output_args)
        # If the value is not a legal variable name just leave it in the keyword list
        if lhs_vars != "":
            lhs_vars += " = "
        final_argstring = self.build_argstring(kwargs)
        replacement = '%s%s(%s)'
        return replacement % (lhs_vars, name, final_argstring)
        
    def get_lhs_var_name(self, match):
        """Returns the lhs variable name from the match or an empty string"""
        return match.group(1)

    def is_lhs_name_legal(self, name):
        """Returns true if the name given is a legal variable name"""
        if name is None: 
            return False
        return mantid.api._adsimports.is_valid_identifier(name)

    def build_lhs_vars(self, kwargs, output_args):
        """Returns a list of lhs variables for the input keywords & given output args"""
        lhs_vars = ""
        indices_for_removal = []
        for index, keyvalue in enumerate(kwargs):
            if keyvalue[0] in output_args:
                value = keyvalue[1].strip("'\"")
                if self.is_lhs_name_legal(value):
                    lhs_vars += value + ","
                    indices_for_removal.append(index)

        for item in indices_for_removal:
            kwargs.pop(item)        
        return lhs_vars.rstrip(",")
    
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

    def transform_all_to_kwargs(self, alg_obj, arguments):
        """
        Transforms an ordered list of arguments to a full keyword list
        The arguments list is expected to be a list of names or tuples
        already containing a keyword argument
        """
        ordered_names = alg_obj.orderedProperties()        
        keyword_found = False
        kwargs = []
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
            name = name.strip()
            value = value.strip()
            kwargs.append(tuple([name, value]))
        
        return kwargs

    def build_argstring(self, kwargs):
        """Builds a single string containing the key=value arguments in the list"""
        argstring = ''
        for key, value in kwargs:
            argstring += key + '=' + value + ","
        return argstring.rstrip(",")
       
    def fix_indentation(self, replaced_string):
        """
        Compare the indentation of the input line
        with the original line & fix the replaced_string if
        necessary
        """
        matches = __WHITESPACE_REGEX__.match(self.current_line)
        indent = matches.groups()[0]
        if indent == '':
            return replaced_string
        indent = indent.replace("\t", "    ")
        return indent + replaced_string

    def is_mantid_algorithm(self, name):
        """Returns true if the given name is a mantid algorithm"""
        if name is None: 
            return False
        return name in __MANTID_ALGS__