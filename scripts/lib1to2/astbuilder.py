"""
Module containing functions for parsing and modification of an AST.

Taken from http://pythoscope.org/ and modified
"""
from lib2to3 import pygram, pytree
from lib2to3.pgen2 import driver, token
from lib2to3.pgen2.parse import ParseError
from lib2to3.pygram import python_symbols as syms
from lib2to3.pytree import Node, Leaf

__all__ = ["EmptyCode", "Newline", "ParseError", "clone", "create_import",
    "insert_after", "insert_before", "parse", "parse_fragment", "regenerate"]

EmptyCode = lambda: Node(syms.file_input, [])
Newline = lambda: Leaf(token.NEWLINE, "\n")

def quoted_block(text):
    return ''.join(["> %s" % line for line in text.splitlines(True)])

def clone(tree):
    """Clone the tree, preserving its add_newline attribute.
    """
    if tree is None:
        return None

    new_tree = tree.clone()
    if hasattr(tree, 'added_newline') and tree.added_newline:
        new_tree.added_newline = True
    return new_tree

def create_import(import_desc):
    """Create an AST representing import statement from given description.

    >>> regenerate(create_import("unittest"))
    'import unittest\\n'
    >>> regenerate(create_import(("nose", "SkipTest")))
    'from nose import SkipTest\\n'
    """
    if isinstance(import_desc, tuple):
        package, name = import_desc
        return Node(syms.import_from,
                    [Leaf(token.NAME, 'from'),
                     Leaf(token.NAME, package, prefix=" "),
                     Leaf(token.NAME, 'import', prefix=" "),
                     Leaf(token.NAME, name, prefix=" "),
                     Newline()])
    else:
        return Node(syms.import_name,
                    [Leaf(token.NAME, 'import'),
                     Leaf(token.NAME, import_desc, prefix=" "),
                     Newline()])

def index(node):
    """Return index this node is at in parent's children list.
    """
    return node.parent.children.index(node)

def insert_after(node, code):
    if not node.parent:
        raise TypeError("Can't insert after node that doesn't have a parent.")
    node.parent.insert_child(index(node)+1, code)

def insert_before(node, code):
    if not node.parent:
        raise TypeError("Can't insert before node that doesn't have a parent.")
    node.parent.insert_child(index(node), code)

def parse(code):
    """String -> AST

    Parse the string and return its AST representation. May raise
    a ParseError exception.
    """
    added_newline = False
    if not code.endswith("\n"):
        code += "\n"
        added_newline = True

    try:
        drv = driver.Driver(pygram.python_grammar, pytree.convert)
        result = drv.parse_string(code, True)
    except ParseError:
        print "Had problems parsing:\n%s\n" % quoted_block(code)
        raise

    # Always return a Node, not a Leaf.
    if isinstance(result, Leaf):
        result = Node(syms.file_input, [result])

    result.added_newline = added_newline

    return result

def parse_fragment(code):
    """Works like parse() but returns an object stripped of the file_input
    wrapper. This eases merging this piece of code into other ones.
    """
    parsed_code = parse(code)

    if is_node_of_type(parsed_code, 'file_input') and \
           len(parsed_code.children) == 2 and \
           is_leaf_of_type(parsed_code.children[-1], token.ENDMARKER):
        return parsed_code.children[0]
    return parsed_code

def regenerate(tree):
    """AST -> String

    Regenerate the source code from the AST tree.
    """
    if hasattr(tree, 'added_newline') and tree.added_newline:
        return str(tree)[:-1]
    else:
        return str(tree)
