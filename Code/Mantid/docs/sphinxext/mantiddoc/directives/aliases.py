from base import BaseDirective


class AliasesDirective(BaseDirective):

    """
    Obtains the aliases for a given algorithm based on it's name.
    """

    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..aliases:: directive is encountered.
        """
        title = self._make_header("Aliases")
        alias = self._get_alias()
        return self._insert_rest(title + alias)

    def _get_alias(self):
        """
        Return the alias for the named algorithm.

        Args:
          algorithm_name (str): The name of the algorithm to get the alias for.
        """
        name, version = self._algorithm_name_and_version()
        alg = self._create_mantid_algorithm(name, version)
        return "This algorithm is also known as: " + "**" + alg.alias() + "**"


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('aliases', AliasesDirective)
