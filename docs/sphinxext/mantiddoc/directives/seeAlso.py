from mantiddoc.directives.base import AlgorithmBaseDirective #pylint: disable=unused-import

class SeeAlsoDirective(AlgorithmBaseDirective):

    """
    Obtains the see also section for a given algorithm based on it's name.
    """

    required_arguments, optional_arguments = 0, 0

    def execute(self):
        """
        Called by Sphinx when the ..seealso:: directive is encountered.
        """
        alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
        seeAlsoList = alg.seeAlso()
        if len(seeAlsoList) > 0:
            self.add_rst(self.make_header("See Also"))
            for seeAlsoEntry in seeAlsoList:
                #test the algorithm exists
                try:
                  alg = self.create_mantid_algorithm_by_name(seeAlsoEntry)
                  link_rst = "`%s <_algm-%s-v%d>`_ \n" % (alg.name(), alg.name(), alg.version())
                  self.add_rst(link_rst)
                except RuntimeError:
                    Sphinx.warn('SeeAlso: Could not find algorithm "{0}" listed in the seeAlso for {1}.v{2}'.format(
                      seeAlsoEntry,self.algorithm_name(), self.algorithm_version()))

        return []

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('seealso', SeeAlsoDirective)
