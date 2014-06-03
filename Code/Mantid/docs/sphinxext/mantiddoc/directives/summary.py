from base import BaseDirective


class SummaryDirective(BaseDirective):

    """
    Obtains the summary for a given algorithm based on it's name.
    """

    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..summary:: directive is encountered.
        """
        self.add_rst(self.make_header("Summary"))
        alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
        self.add_rst(alg.summary())
        self.commit_rst()

        return []

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('summary', SummaryDirective)
