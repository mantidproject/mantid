from docutils import statemachine
from docutils.parsers.rst import Directive


class BaseDirective(Directive):

    """
    Contains shared functionality for Mantid custom directives.
    """

    has_content = True
    final_argument_whitespace = True

    def _make_header(self, name, title=False):
        """
        Makes a ReStructuredText title from the algorithm's name.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          title (bool): If True, line is inserted above & below algorithm name.

        Returns:
          str: ReST formatted header with algorithm_name as content.
        """
        line = "\n" + "-" * len(name) + "\n"
        if title:
            return line + name + line
        else:
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

    def _create_mantid_algorithm(self, algorithm_name):
        """
        Create and initializes a Mantid algorithm.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.

        Returns:
          algorithm: An instance of a Mantid algorithm.
        """
        from mantid.api import AlgorithmManager
        alg = AlgorithmManager.createUnmanaged(algorithm_name)
        alg.initialize()
        return alg
