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
    has_content = False
    final_argument_whitespace = True

    rst_lines = None

    def add_rst(self, text):
        """
        Appends given reST into a managed list. It is NOT inserted into the
        document until commit_rst() is called

        Args:
          text (str): reST to track
        """
        if self.rst_lines is None:
            self.rst_lines = []

        self.rst_lines.extend(statemachine.string2lines(text))

    def commit_rst(self):
        """
        Inserts the currently tracked rst lines into the state_machine
        """
        self.state_machine.insert_input(self.rst_lines, "")
        self.rst_lines = []

    def make_header(self, name, pagetitle=False):
        """
        Makes a ReStructuredText title from the algorithm's name.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          pagetitle (bool): If True, line is inserted above & below algorithm name.

        Returns:
          str: ReST formatted header with algorithm_name as content.
        """
        if pagetitle:
            line = "\n" + "=" * (len(name) + 1) + "\n"
            return line + name + line
        else:
            line = "\n" + "-" * len(name) + "\n"
            return name + line

#----------------------------------------------------------------------------------------

class AlgorithmBaseDirective(BaseDirective):
    """
    Specialized base directive for an algorithm
    """

    algm_name = None
    algm_version = None

    def run(self):
        """
        The main entry point that docutils calls.
        It calls self.execute to do the main work. If an
        algorithm doesn't exist then the directive is
        skipped and a warning is emitted.

        Derived classes should override execute() and insert
        whatever rst they require with self.add_rst()
        """
        nodes = []
        skip_msg = self.skip()
        if skip_msg != "":
            self.add_rst("**ERROR: %s**" % skip_msg)
        else:
            nodes = self.execute()

        if self.rst_lines is not None:
            self.commit_rst()
        return nodes

    def skip(self):
        """
        Override and return a string depending on whether the directive
        should be skipped. If empty then the directive should be processed
        otherwise the string should contain the error message
        The default is to skip (and warn) if the algorithm is not known.

        Returns:
          str: Return error mesage string if the directive should be skipped
        """
        from mantid.api import AlgorithmFactory

        name, version = self.algorithm_name(), self.algorithm_version()
        if AlgorithmFactory.exists(name, version):
            return ""
        else:
            msg = "No algorithm '%s' version '%d', skipping directive" % (name, version)
            env = self.state.document.settings.env
            env.warn(env.docname, msg)
            return msg

    def algorithm_name(self):
        """
        Returns the algorithm name as parsed from the document name
        """
        if self.algm_name is None:
            self._set_algorithm_name_and_version()
        return self.algm_name

    def algorithm_version(self):
        """
        Returns the algorithm version as parsed from the document name
        """
        if self.algm_version is None:
            self._set_algorithm_name_and_version()
        return self.algm_version

    def create_mantid_algorithm(self, algorithm_name, version):
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

    def _set_algorithm_name_and_version(self):
        """
        Returns the name and version of an algorithm based on the name of the
        document. The expected name of the document is "AlgorithmName-v?", which
        is the name of the file with the extension removed
        """
        env = self.state.document.settings.env
        self.algm_name, self.algm_version = algorithm_name_and_version(env.docname)
