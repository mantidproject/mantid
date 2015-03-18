"""
   Defines custom directives for Mantid documentation

   Each directive should be defined in a different module and have its own
   setup(app) function. The setup function defined here is used to tie them
   all together in the case where all directives are required, allowing
   'mantiddoc.directives' to be added to the Sphinx extensions configuration.
"""

import algorithm, alias, attributes, categories, diagram, interface, properties, summary

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    algorithm.setup(app)
    alias.setup(app)
    attributes.setup(app)
    categories.setup(app)
    diagram.setup(app)
    interface.setup(app)
    properties.setup(app)
    summary.setup(app)
