# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantiddoc.directives.properties import PropertiesDirective


class AttributesDirective(PropertiesDirective):
    """
    Outputs the given fit function's properties into a ReST formatted table.
    """

    # Accept one required argument and no optional arguments.
    required_arguments, optional_arguments = 0, 0
    has_content = True

    def execute(self):
        """
        Called by Sphinx when the ..attributes:: directive is encountered.
        """
        self._create_attributes_table()
        return []

    def _create_attributes_table(self):
        """
        Populates the ReST table with algorithm properties.

        If it is done as a part of a multiline description, each line
        will describe a single attribute as a semicolon separated list
        Name;Type;Default;Description
        """
        if self.algorithm_version() is None:  # This is an IFunction
            ifunc = self.create_mantid_ifunction(self.algorithm_name())
            if ifunc.nAttributes() <= 0:
                return False

            # Stores each property of the algorithm in a tuple.
            attributes = []

            # names for the table headers.
            header = ("Name", "Type", "Default", "Description")

            if len(self.content) > 0:
                for line in self.content:
                    args = tuple(line.split(";"))
                    args = [item.strip() for item in args]
                    if len(args) != len(header):
                        raise RuntimeError("Expected %d items in line '%s'" % (len(header), str(args)))
                    else:
                        attributes.append(args)
            else:
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
    app.add_directive("attributes", AttributesDirective)
