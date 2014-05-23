from base import BaseDirective


class SummaryDirective(BaseDirective):

    """
    Obtains the summary for a given algorithm based on it's name.
    """

    required_arguments, optional_arguments = 1, 0

    def run(self):
        """
        Called by Sphinx when the ..summary:: directive is encountered.
        """
        title = self._make_header(__name__.title())
        summary = self._get_summary(str(self.arguments[0]))
        return self._insert_rest(title + summary)

    def _get_summary(self, algorithm_name):
        """
        Return the summary for the named algorithm.

        Args:
          algorithm_name (str): The name of the algorithm.
        """
        alg = self._create_mantid_algorithm(algorithm_name)
        return alg.getWikiSummary()


def setup(app):
    app.add_directive('summary', SummaryDirective)
