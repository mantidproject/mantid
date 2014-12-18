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

    def apply(self, text):
        """
        """
        raise RuntimeError("Simple API replacement can only be applied to an abstract syntax tree." + \
                           "See apply_to_ast()")

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
        # Transform to all keywords
        nchildren = len(parent.children)
        no_lhs = True
        if nchildren == 2:
            # Child 0 = whole function call
            fn_call_node = parent.children[0]
            no_lhs = True
        elif nchildren == 3:
            self.errors.append("Unable to handle assignment for '%s' algorithm call" % (name))
            return

        # Get the argument list node: child 1 = arglist parent node then child 1 = arg list
        arglist_node = fn_call_node.children[1].children[1]

        # Transform to keywords
        alg_object = mantid.api.AlgorithmManager.Instance().createUnmanaged(name)
        alg_object.initialize()
        arglist_node = self.transform_arglist_to_keywords(arglist_node, alg_object)
        # Replace node as a new one may have been created
        fn_call_node.children[1].children[1] = arglist_node

    def transform_arglist_to_keywords(self, arglist_node, alg_object):
        """Takes a node that points to argument list and transforms
        it to all keyword=values
            @param arglist_node The node that points to the argument list
            @param alg_object The algorithm object that corresponds to this list
        """
        ordered_props = alg_object.orderedProperties()
        # Special case where the arglist has no children
        if len(arglist_node.children) == 0:
            arglist_node = Node(syms.argument, [Leaf(token.NAME,ordered_props[0]),Leaf(token.EQUAL,"="),
                                                Leaf(arglist_node.type,arglist_node.value)])
            return arglist_node
        # Quick check: A 3 arg leaf list with an equals at the 2nd element needs nothing doing
        if len(arglist_node.children) == 3 and arglist_node.children[1].type == token.EQUAL:
            return arglist_node

        # Construct our argument list from the children to make sure we separate out whole comma-separated
        # sections i.e get embedded lists correct
        args = [[]] # Each list will be delimited by a comma
        nargs = 0
        index = 0
        for node in arglist_node.children:
            if node.type == token.COMMA:
                args.append(node)
                args.append([]) # new arg list
                index += 2 # include comma
                nargs += 1
            else:
                args[index].append(node)

        # Ordered props
        prop_index = 0 # List has commas so standard enumerate won't do the trick
        arg_nodes = [] # Holds the final node list
        for arg_list in args:
            if isinstance(arg_list, Leaf): # Must be comma from construction above
                arg_nodes.append(arg_list)
            else:
                first = arg_list[0]
                if not (isinstance(first, Node) and first.type == syms.argument):
                    prop_name = ordered_props[prop_index]
                    children=[Leaf(token.NAME,prop_name),Leaf(token.EQUAL,"=")]
                    children.extend(arg_list)
                    for c in children:
                        c.parent = None # A new node requires all old parent nodes to be None
                    arg_nodes.append(Node(syms.argument, children))
                else:
                    for node in arg_list:
                        arg_nodes.append(node)
                # Move to the next property
                prop_index += 1

        arglist_node.children = arg_nodes
        return arglist_node

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
            child_zero = node.children[0]
            if hasattr(child_zero, "value") and \
                    _is_mantid_algorithm(child_zero.value):
                # Make sure we get the whole expression including any assignment
                self._nodes.append(tuple([child_zero.value,node.parent]))

        # Ensure we visit the whole tree
        self.visit(node.children)
