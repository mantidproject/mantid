from base import BaseDirective


class CategoriesDirective(BaseDirective):

    """
    Obtains the categories for a given algorithm based on it's name.
    """

    required_arguments, optional_arguments = 1, 0

    def run(self):
        """
        Called by Sphinx when the ..categories:: directive is encountered.
        """
        title = self._make_header(__name__.title())
        categories = self._get_categories(str(self.arguments[0]))
        return self._insert_rest(title + categories)

    def _get_categories(self, algorithm_name):
        """
        Return the categories for the named algorithm.

        Args:
          algorithm_name (str): The name of the algorithm.
        """
        alg = self._create_mantid_algorithm(algorithm_name)

        # Create a list containing each category.
        categories = alg.category().split("\\")

        if len(categories) >= 2:
            # Add a cross reference for each catagory.
            links = (":ref:`%s` | " * len(categories)) % tuple(categories)
            # Remove last three characters to remove last |
            return ("`Categories: <categories.html>`_ " + links)[:-3]
        else:
            return "`Category: <categoies.html>`_ :ref:`%s`" % (categories)


def setup(app):
    app.add_directive('categories', CategoriesDirective)
