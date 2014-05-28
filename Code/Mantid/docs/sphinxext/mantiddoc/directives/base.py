from docutils import statemachine
from docutils.parsers.rst import Directive
import re

ALG_DOCNAME_RE = re.compile(r'^([A-Z][a-zA-Z0-9]+)-v([0-9][0-9]*)$')

#----------------------------------------------------------------------------------------
def algorithm_name_and_version(docname):
    """
    Returns the name and version of an algorithm based on the name of the
    document supplied. The expected name of the document is "AlgorithmName-v?", which
    is the name of the file with the extension removed
    
    Arguments:
      docname (str): The name of the document as supplied by docutils. Can contain slashes to indicate a path

    Returns:
      tuple: A tuple containing two elements (name, version)
    """
    # docname includes path, using forward slashes, from root of documentation directory
    docname = docname.split("/")[-1]
    match = ALG_DOCNAME_RE.match(docname)
    if not match or len(match.groups()) != 2:
        raise RuntimeError("Document filename '%s.rst' does not match the expected format: AlgorithmName-vX.rst" % docname)

    grps = match.groups()
    return (str(grps[0]), int(grps[1]))

#----------------------------------------------------------------------------------------
class BaseDirective(Directive):

    """
    Contains shared functionality for Mantid custom directives.
    """

    has_content = True
    final_argument_whitespace = True

    def _algorithm_name_and_version(self):
        """
        Returns the name and version of an algorithm based on the name of the
        document. The expected name of the document is "AlgorithmName-v?", which
        is the name of the file with the extension removed
        """
        env = self.state.document.settings.env
        return algorithm_name_and_version(env.docname)

    def _make_header(self, name, pagetitle=False):
        """
        Makes a ReStructuredText title from the algorithm's name.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          pagetitle (bool): If True, line is inserted above & below algorithm name.

        Returns:
          str: ReST formatted header with algorithm_name as content.
        """
        if pagetitle:
            line = "\n" + "=" * len(name) + "\n"
            return line + name + line
        else:
            line = "\n" + "-" * len(name) + "\n"
            return name + line

    def _insert_rest(self, text):
        """
        Inserts ReStructuredText into the algorithm file.

        Args:
          text (str): Inserts ReStructuredText into the algorithm file.

        Returns:
          list: Empty list. This is required by the inherited run method.
        """
        self.state_machine.insert_input(statemachine.string2lines(text), "")
        return []

    def _create_mantid_algorithm(self, algorithm_name, version):
        """
        Create and initializes a Mantid algorithm.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          version (int): Version of the algorithm to create

        Returns:
          algorithm: An instance of a Mantid algorithm.
        """
        from mantid.api import AlgorithmManager
        alg = AlgorithmManager.createUnmanaged(algorithm_name, version)
        alg.initialize()
        return alg
