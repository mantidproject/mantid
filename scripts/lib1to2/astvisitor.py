"""
Module containing ASTVistor class and helpers useful for traversing
and extracting information from an AST.

Taken from http://pythoscope.org/ and modified
"""

from lib2to3 import pytree
from lib2to3.patcomp import compile_pattern
from lib2to3.pgen2 import token
from lib2to3.pytree import Node, Leaf

__all__ = ["ASTError", "ASTVisitor", "descend", "find_last_leaf",
    "get_starting_whitespace", "is_leaf_of_type", "is_node_of_type",
    "remove_trailing_whitespace"]

def descend(tree, visitor_type):
    """Walk over the AST using a visitor of a given type and return the visitor
    object once done.
    """
    visitor = visitor_type()
    visitor.visit(tree)
    return visitor

def find_last_leaf(node):
    if isinstance(node, Leaf):
        return node
    else:
        return find_last_leaf(node.children[-1])

def get_starting_whitespace(code):
    whitespace = ""
    for child in code.children:
        if is_leaf_of_type(child, token.NEWLINE, token.INDENT):
            whitespace += child.value
        else:
            break
    return whitespace

def remove_trailing_whitespace(code):
    leaf = find_last_leaf(code)
    leaf.prefix = leaf.prefix.replace(' ', '').replace('\t', '')

class ASTError(Exception):
    pass

def is_leaf_of_type(leaf, *types):
    return isinstance(leaf, Leaf) and leaf.type in types

def is_node_of_type(node, *types):
    return isinstance(node, Node) and pytree.type_repr(node.type) in types

def leaf_value(leaf):
    return leaf.value

def remove_commas(nodes):
    def isnt_comma(node):
        return not is_leaf_of_type(node, token.COMMA)
    return filter(isnt_comma, nodes)

def remove_defaults(nodes):
    ignore_next = False
    for node in nodes:
        if ignore_next is True:
            ignore_next = False
            continue
        if is_leaf_of_type(node, token.EQUAL):
            ignore_next = True
            continue
        yield node

def derive_class_name(node):
    return str(node).strip()

def derive_class_names(node):
    if node is None:
        return []
    elif is_node_of_type(node, 'arglist'):
        return map(derive_class_name, remove_commas(node.children))
    else:
        return [derive_class_name(node)]

def derive_argument(node):
    if is_leaf_of_type(node, token.NAME):
        return node.value
    elif is_node_of_type(node, 'tfpdef'):
        return tuple(map(derive_argument,
                         remove_commas(node.children[1].children)))

def derive_arguments_from_typedargslist(typedargslist):
    prefix = ''
    for node in remove_defaults(remove_commas(typedargslist.children)):
        if is_leaf_of_type(node, token.STAR):
            prefix = '*'
        elif is_leaf_of_type(node, token.DOUBLESTAR):
            prefix = '**'
        elif prefix:
            yield prefix + derive_argument(node)
            prefix = ''
        else:
            yield derive_argument(node)

def derive_arguments(node):
    if node == []:
        return []
    elif is_node_of_type(node, 'typedargslist'):
        return list(derive_arguments_from_typedargslist(node))
    else:
        return [derive_argument(node)]

def derive_import_name(node):
    if is_leaf_of_type(node, token.NAME):
        return node.value
    elif is_node_of_type(node, 'dotted_as_name'):
        return (derive_import_name(node.children[0]),
                derive_import_name(node.children[2]))
    elif is_node_of_type(node, 'dotted_name'):
        return "".join(map(leaf_value, node.children))

def derive_import_names(node):
    if node is None:
        return None
    elif is_node_of_type(node, 'dotted_as_names', 'import_as_names'):
        return map(derive_import_name,
                   remove_commas(node.children))
    else:
        return [derive_import_name(node)]


class ASTVisitor(object):
    DEFAULT_PATTERNS = [
        ('_visit_all', "file_input< nodes=any* >"),
        ('_visit_all', "suite< nodes=any* >"),
        ('_visit_class', "body=classdef< 'class' name=NAME ['(' bases=any ')'] ':' any >"),
        ('_visit_function', "body=funcdef< 'def' name=NAME parameters< '(' [args=any] ')' > ':' any >"),
        ('_visit_import', "body=import_name< 'import' names=any > | body=import_from< 'from' import_from=any 'import' names=any >"),
        ('_visit_lambda_assign', "expr_stmt< name=NAME '=' lambdef< 'lambda' [args=any] ':' any > >"),
        ('_visit_main_snippet', "body=if_stmt< 'if' comparison< '__name__' '==' (\"'__main__'\" | '\"__main__\"' ) > ':' any >"),
    ]

    def __init__(self):
        self.patterns = []
        for method, pattern in self.DEFAULT_PATTERNS:
            self.register_pattern(method, pattern)

    def register_pattern(self, method, pattern):
        """Register method to handle given pattern.
        """
        self.patterns.append((method, compile_pattern(pattern)))

    def visit(self, tree):
        """Main entry point of the ASTVisitor class.
        """
        if isinstance(tree, Leaf):
            self.visit_leaf(tree)
        elif isinstance(tree, Node):
            self.visit_node(tree)
        elif isinstance(tree, list):
            for subtree in tree:
                self.visit(subtree)
        else:
            raise ASTError("Unknown tree type: %r." % tree)

    def visit_leaf(self, leaf):
        pass

    def visit_node(self, node):
        for method, pattern in self.patterns:
            results = {}
            if pattern.match(node, results):
                getattr(self, method)(results)
                break
        else:
            # For unknown nodes simply descend to their list of children.
            self.visit(node.children)

    def visit_class(self, name, bases, body):
        self.visit(body.children)

    def visit_function(self, name, args, body):
        self.visit(body.children)

    def visit_import(self, names, import_from, body):
        pass

    def visit_lambda_assign(self, name, args):
        pass

    def visit_main_snippet(self, body):
        pass

    def _visit_all(self, results):
        self.visit(results['nodes'])

    def _visit_class(self, results):
        self.visit_class(name=results['name'].value,
                         bases=derive_class_names(results.get('bases')),
                         body=results['body'])

    def _visit_function(self, results):
        self.visit_function(name=results['name'].value,
                            args=derive_arguments(results.get('args', [])),
                            body=results['body'])

    def _visit_import(self, results):
        self.visit_import(names=derive_import_names(results['names']),
                          import_from=derive_import_name(results.get('import_from')),
                          body=results['body'])

    def _visit_lambda_assign(self, results):
        self.visit_lambda_assign(name=results['name'].value,
                                 args=derive_arguments(results.get('args', [])))

    def _visit_main_snippet(self, results):
        self.visit_main_snippet(body=results['body'])
