from properties import PropertiesDirective
import string


class AttributesDirective(PropertiesDirective):

    """
    Outputs the given fit function's properties into a ReST formatted table.
    """
    # Accept one required argument and no optional arguments.
    required_arguments, optional_arguments = 0, 0

    def execute(self):
        """
        Called by Sphinx when the ..attributes:: directive is encountered.
        """
        self._create_attributes_table()
        return []

    def _create_attributes_table(self):
        """
        Populates the ReST table with algorithm properties.
        """
        if self.algorithm_version() is None: # This is an IFunction
            ifunc = self.create_mantid_ifunction(self.algorithm_name())
            if ifunc.nAttributes() <= 0:
                return False

            # Stores each property of the algorithm in a tuple.
            attributes = []

            # names for the table headers.
            header = ('Name', 'Type', 'Default', 'Description')

            for name in ifunc.attributeNames():
                attributes.append((name, "", "", ""))

            self.add_rst(self.make_header("Attributes (non-fitting parameters)"))
        else:
            raise RuntimeError("Document does not appear to describe a fit function")

        self.add_rst(self._build_table(header, attributes))
        return True

def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('attributes', AttributesDirective)
