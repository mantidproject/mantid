"""
   Defines custom directives for Mantid documentation

   Each directive should be defined in a different module and have its own
   setup(app) function. The setup function defined here is used to tie them
   all together in the case where all directives are required, allowing
   'mantiddoc.directives' to be added to the Sphinx extensions configuration.
"""

import mantiddoc.directives.algorithm
import mantiddoc.directives.alias
import mantiddoc.directives.attributes
import mantiddoc.directives.categories
import mantiddoc.directives.diagram
import mantiddoc.directives.interface
import mantiddoc.directives.properties
import mantiddoc.directives.sourcelink
import mantiddoc.directives.summary

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
    sourcelink.setup(app)
    summary.setup(app)
