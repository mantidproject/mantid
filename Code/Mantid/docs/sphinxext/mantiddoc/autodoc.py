"""
Mantid customizations for the behaviour of the Sphinx autodoc
extension
"""

def skip(app, what, name, obj, skip, options):
    """
    Arguments:
      app: Sphinx application object
      what: the type of the object which the docstring belongs to (one of "module", "class", "exception", "function", "method", "attribute")
      name: the fully qualified name of the object
      obj: the object itself
      skip: a boolean indicating if autodoc will skip this member if the user handler does not override the decision
      options: the options given to the directive: an object with attributes inherited_members, undoc_members,
               show_inheritance and noindex that are true if the flag option of same name was given to the auto directive
    """
    if name == "__init__":
        return False
    return skip

def setup(app):
    # Define which methods are skipped when running autodoc
    # on a member
    app.connect("autodoc-skip-member", skip)