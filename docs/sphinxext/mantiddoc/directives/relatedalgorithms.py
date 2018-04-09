from mantiddoc.directives.base import AlgorithmBaseDirective #pylint: disable=unused-import

class relatedalgorithmsDirective(AlgorithmBaseDirective):

    """
    Obtains the see also section for a given algorithm based on it's name.
    This lists similar algorithms and aliases
    """

    required_arguments, optional_arguments = 0, 0

    def execute(self):
        """
        Called by Sphinx when the ..seealso:: directive is encountered.
        """
        alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
        seeAlsoList = alg.seeAlso()
        alias = alg.alias()
        link_rst = ""
        if seeAlsoList:
            for seeAlsoEntry in seeAlsoList:
                #test the algorithm exists
                try:
                    alg = self.create_mantid_algorithm_by_name(seeAlsoEntry)
                    link_rst += ":ref:`%s <algm-%s>`, " % (alg.name(), alg.name())
                except RuntimeError:
                    env = self.state.document.settings.env
                    env.app.warn('relatedalgorithms - Could not find algorithm "{0}" listed in the seeAlso for {1}.v{2}'.format(
                                 seeAlsoEntry,self.algorithm_name(), self.algorithm_version()))
                      
        if link_rst or alias:
            self.add_rst(self.make_header("See Also",level=3))
            if link_rst:
                link_rst = link_rst.rstrip(", ") # remove final separator
                self.add_rst(link_rst + "\n\n")
            if alias:
                format_str = "This algorithm is also known as: **%s**"
                self.add_rst(format_str % alias)
        return []
        
def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('relatedalgorithms', relatedalgorithmsDirective)
