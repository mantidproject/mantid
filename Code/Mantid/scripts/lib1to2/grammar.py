"""
Defines the grammar translation from version 1 to version 2 of Mantid's Python API.
"""
import messages
import rules
import astbuilder

class Grammar(object):
    """
    Translation from v1->v2 of the Python API
    """

    def __init__(self):
        pass

    def translate(self, orig_code):
        """
        Translates the input string, assuming it contains code written in
        version 1 of Mantid's Python API, to version 2 of the PythonAPI

        @param orig_code The original code string
        @returns The translated string
        """
        errors = []
        # Simple string replacements
        string_replace = rules.SimpleStringReplace()
        translated = string_replace.apply(orig_code)

        api_call_replace = rules.SimpleAPIFunctionCallReplace()
        # Convert to an abstract syntax tree
        # (uses the lib2to3 libraries that can convert AST back to source code)
        tree = astbuilder.parse(translated)
        tree = api_call_replace.apply_to_ast(tree)

        errors.extend(api_call_replace.errors)
        # No python algorithm support yet
        if "PythonAlgorithm" in orig_code:
            errors.append("Cannot fully migrate PythonAlgorithm.")

        return astbuilder.regenerate(tree), errors
