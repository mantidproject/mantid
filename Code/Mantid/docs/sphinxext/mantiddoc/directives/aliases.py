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
        alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
        alias = alg.alias()
        if len(alias) == 0:
            return []

        self.add_rst(self.make_header("Aliases"))
        format_str = "This algorithm is also known as: **%s**"
        self.add_rst(format_str % alias)
        self.commit_rst()

        return []

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('aliases', AliasesDirective)
