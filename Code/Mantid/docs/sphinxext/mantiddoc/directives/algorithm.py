from base import BaseDirective


class AlgorithmDirective(BaseDirective):

    """
    Adds a referenceable link for a given algorithm, a title,
    and a screenshot of the algorithm to an rst file.
    """

    required_arguments, optional_arguments = 1, 0

    def run(self):
        """
        Called by Sphinx when the ..algorithm:: directive is encountered
        """
        algorithm_name = str(self.arguments[0])

        # Seperate methods for each unique piece of functionality.
        reference = self._make_reference_link(algorithm_name)
        title = self._make_header(algorithm_name, True)
        toc = self._make_local_toc()
        screenshot = self._get_algorithm_screenshot(algorithm_name)

        return self._insert_rest(reference + title + screenshot + toc)

    def _make_reference_link(self, algorithm_name):
        """
        Outputs a reference to the top of the algorithm's rst file.

        Args:
          algorithm_name (str): The name of the algorithm to reference.

        Returns:
          str: A ReST formatted reference.
        """
        return ".. _" + algorithm_name.title() + ":" + "\n"

    def _make_local_toc(self):
        return ".. contents:: Table of Contents\n    :local:\n"

    def _get_algorithm_screenshot(self, algorithm_name):
        """
        Obtains the location of the screenshot for a given algorithm.

        Args:
          algorithm_name (str): The name of the algorithm.

        Returns:
          str: The location of the screenshot for the given algorithm.
        """
        images_dir = self.state.document.settings.env.config["mantid_images"]
        screenshot = images_dir + algorithm_name + ".png"
        return ".. image:: " + screenshot + "\n" + "    :class: screenshot"


def setup(app):
    app.add_config_value('mantid_images', 'mantid_images', 'env')
    app.add_directive('algorithm', AlgorithmDirective)
