"""Defines the rules for translation of simple API function calls
from v1 to v2
"""
from lib2to3.pytree import Node, Leaf
from lib2to3.pygram import python_symbols as syms
from ..astvisitor import ASTVisitor, descend

import mantid

import rules
import re
import token

__FUNCTION_CALL_REGEXSTR = r"""(|\w*\s*) # Any variable on the lhs of the function call
                     (|=\s*) # The equals sign including any whitespace on the right
                     (\w*) # The function name
                     \((.*?)\) # The function call arguments (without the parentheses)
                     (\.workspace\(\)|) # An optional trailing .workspace() call""" 

__FUNCTION_CALL_REGEX__ = re.compile(__FUNCTION_CALL_REGEXSTR, re.VERBOSE)

__WHITESPACE_REGEX__ = re.compile("(\s*).*")

__MANTID_ALGS__ = mantid.api.AlgorithmFactory.getRegisteredAlgorithms(True)

def _is_mantid_algorithm(name):
    """Returns true if the given name is a mantid algorithm"""
    if type(name) is not str: 
        raise ValueError("Expected string name found: %s" % str(name))
    return name in __MANTID_ALGS__

class SimpleAPIFunctionCallReplace(rules.Rules):
    
    func_regex = __FUNCTION_CALL_REGEX__
    current_line = None
    
    def __init__(self):
        rules.Rules.__init__(self)
        
    def apply_to_ast(self, tree):
        """
        Returns a replacement ast tree for the input tree where the simple 
        API function calls are replaced by improved ones for the new API. 
            @param tree An AST tree from lib2to3
            @returns A string containing the replacement of the original text
                     if no replacement was needed
        """
        nodes = self.find_simpleapi_nodes(tree)
        for name, parent in nodes:
            self.apply_to_node(name, parent)
        return tree

    def find_simpleapi_nodes(self, tree):
        """
            Find the parent nodes that are a call to the simple api
            @param tree The abstract syntax tree
        """
        visitor = SimpleAPIVisitor()
        return visitor.find_parent_nodes(tree)

    def apply_to_node(self, name, parent):
        """Given a node and it's function name, assume it is a v1 API call and update it
        to version 2. This replaces everything with keywords
        """
        # Two steps: 
        #   First - transform to all keywords
        #   Second - remove output properties from list and put on lhs
        nchildren = len(parent.children)
        no_lhs = True
        if nchildren == 2:
            # Child 0 = whole function call
            fn_call_node = parent.children[0]
            no_lhs = True
        elif nchildren == 3:
            raise RuntimeError("Unable to handle assignment from algorithm call")
        
        # Get the argument list node: child 1 = arglist parent node then child 1 = arg list
        arglist_node = fn_call_node.children[1].children[1]

        # Transform to keywords
        alg_object = mantid.api.AlgorithmManager.Instance().createUnmanaged(name)
        alg_object.initialize()
        self.transform_arglist_to_keywords(arglist_node, alg_object)

        # Pull out the output args from the argument list
        self.put_output_args_on_lhs(fn_call_node, arglist_node, alg_object)

    def transform_arglist_to_keywords(self, arglist_node, alg_object):
        """Takes a node that points to argument list and transforms
        it to all keyword=values
            @param arglist_node The node that points to the argument list
            @param alg_object The algorithm object that corresponds to this list
        """
        # Nodes include the commas so nchildren = 2*nargs - 1
        args = arglist_node.children
        nargs = len(args)
        ordered_props = alg_object.orderedProperties()
        # Loop until we hit a Node that is not a comma and has
        # children in which case it is already keyworded
        prop_index = 0
        for i in range(nargs):
            arg_node = args[i]
            if isinstance(arg_node, Node):
                # Keywords must be after positional so we are done
                break
            elif isinstance(arg_node, Leaf):
                if arg_node.type == token.COMMA:
                    continue
                else:
                    prop_name = ordered_props[prop_index]
                    # Make new keyword Node
                    kw_node = Node(syms.argument, 
                                   [Leaf(token.NAME,prop_name),Leaf(token.EQUAL,"="),
                                    Leaf(arg_node.type,arg_node.value)]
                                   )
                    args[i] = kw_node
                    # increment property index counter
                    prop_index += 1

    def put_output_args_on_lhs(self, fn_call_node, arglist_node, alg_object):
        """Takes a node that points to argument list and puts the output arguments
           on the LHS
            @param fn_call_mode The node pointing to the function call
            @param arglist_node The node that points to the argument list
            @param alg_object The algorithm object that corresponds to this list
        """
        output_props = alg_object.outputProperties()
        nargs = len(arglist_node.children)
        prop_index = 0
        for i in range(nargs):
            if arg_node.type == token.COMMA:
                prop_index += 1
                continue

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

#=====================================================================================

class SimpleAPIVisitor(ASTVisitor):
    """A visitor class that is applies the simple API
    fixes to the AST
    """
    # Stores the found simple API parent nodes in a tuple with their name
    # as the first argument
    _nodes = []

    def __init__(self):
        ASTVisitor.__init__(self)

    def find_parent_nodes(self, tree):
        self.visit(tree)
        return self._nodes

    def visit_node(self, node):
        if node.type == syms.power:
            fn_name = node.children[0].value
            if _is_mantid_algorithm(fn_name):
                # Make sure we get the whole expression including any assignment
                self._nodes.append(tuple([fn_name,node.parent]))

        # Ensure we visit the whole tree
        self.visit(node.children)
