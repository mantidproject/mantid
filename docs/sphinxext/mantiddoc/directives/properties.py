# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,deprecated-module
from mantiddoc.directives.base import AlgorithmBaseDirective  # pylint: disable=unused-import
import re
from string import punctuation

SUBSTITUTE_REF_RE = re.compile(r"\|(.+?)\|")


class PropertiesDirective(AlgorithmBaseDirective):
    """
    Outputs the given algorithm's properties into a ReST formatted table.
    """

    # Accept one required argument and no optional arguments.
    required_arguments, optional_arguments = 0, 0

    def execute(self):
        """
        Called by Sphinx when the ..properties:: directive is encountered.
        """
        self._create_properties_table()
        return []

    def _create_properties_table(self):
        """
        Populates the ReST table with algorithm properties.
        """
        if self.algorithm_version() is None:  # This is an IFunction
            ifunc = self.create_mantid_ifunction(self.algorithm_name())
            if ifunc.numParams() <= 0:
                return False

            # Stores each property of the algorithm in a tuple.
            properties = []

            # names for the table headers.
            header = ("Name", "Default", "Description")

            for i in range(ifunc.numParams()):
                properties.append((ifunc.parameterName(i), str(ifunc.getParameterValue(i)), ifunc.paramDescription(i)))
            self.add_rst(self.make_header("Properties (fitting parameters)"))
        else:  # this is an Algorithm
            alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
            alg_properties = alg.getProperties()
            if len(alg_properties) == 0:
                return False

            # Stores each property of the algorithm in a tuple.
            properties = []

            # names for the table headers.
            header = ("Name", "Direction", "Type", "Default", "Description")

            # Used to obtain the name for the direction property rather than an
            # int.
            direction_string = ["Input", "Output", "InOut", "None"]

            # dictionary to convert from property type to link to category page (where possible)
            property_type_dict = {
                "Workspace": ":ref:`Workspace <Workspace>`",
                "Workspace2D": ":ref:`Workspace2D <Workspace2D>`",
                "EventWorkspace": ":ref:`EventWorkspace <EventWorkspace>`",
                "MatrixWorkspace": ":ref:`MatrixWorkspace <MatrixWorkspace>`",
                "GroupWorkspace": ":ref:`GroupWorkspace <WorkspaceGroup>`",
                "MDEventWorkspace": ":ref:`MDEventWorkspace <MDWorkspace>`",
                "MDHistoWorkspace": ":ref:`MDHistoWorkspace <MDHistoWorkspace>`",
                "TableWorkspace": ":ref:`TableWorkspace <Table Workspaces>`",
            }

            for prop in alg_properties:
                # Append a tuple of properties to the list.
                properties.append(
                    (
                        str(prop.name),
                        str(direction_string[prop.direction]),
                        property_type_dict.get(str(prop.type), str(prop.type)),
                        str(self._get_default_prop(prop)),
                        self._create_property_description_string(prop),
                    )
                )

            self.add_rst(self.make_header("Properties"))
        self.add_rst(self._build_table(header, properties))
        return True

    def _build_table(self, header_content, table_content):
        """
        Build the ReST format

        Args:
          header_content (list): Header for the table. Must be the
          same length as the rows

          table_content (list of tuples): Each tuple (row) container
          property values for a unique property of that algorithm.

        Returns:
          str: ReST formatted table containing algorithm properties.
        """
        # The width of the columns. Multiply row length by 10 to ensure small
        # properties format correctly.
        # Added 10 to the length to ensure if table_content is 0 that
        # the table is still displayed.
        col_sizes = [max((len(row[i] * 10) + 10) for row in table_content) for i in range(len(header_content))]

        # Use the column widths as a means to formatting columns.
        formatter = " ".join("{%d:<%d}" % (index, col) for index, col in enumerate(col_sizes))
        # Add whitespace to each column. This depends on the values returned by
        # col_sizes.
        table_content_formatted = [formatter.format(*item) for item in table_content]
        # Create a separator for each column
        separator = formatter.format(*["=" * col for col in col_sizes])
        # Build the table.
        header = "\n" + separator + "\n" + formatter.format(*header_content) + "\n"
        content = separator + "\n" + "\n".join(table_content_formatted) + "\n" + separator
        # Join the header and footer.
        return header + content

    def _get_default_prop(self, prop):
        """
        Converts the default value of the property to a more use-friendly one.

        Args:
          prop (str): The algorithm property to use.

        Returns:
          str: The default value of the property.
        """
        from mantid.api import IWorkspaceProperty

        # Used to obtain the name for the direction property rather than
        # outputting an int.
        direction_string = ["Input", "Output", "InOut", "None"]

        # Nothing to show under the default section for an output properties
        # that are not workspace properties.
        if (direction_string[prop.direction] == "Output") and (not isinstance(prop, IWorkspaceProperty)):
            default_prop = ""
        elif prop.isValid == "":
            default_prop = self._create_property_default_string(prop)
        else:
            default_prop = "*Mandatory*"
        return default_prop

    def _create_property_default_string(self, prop):
        """
        Converts the default value of the property to a more use-friendly one.

        Args:
          prop. The property to find the default value of.

        Returns:
          str: The string to add to the property table default section.
        """

        default = prop.getDefault
        defaultstr = ""

        # Convert to int, then float, then any string
        try:
            val = int(default)
            if val >= 2147483647:
                defaultstr = "*Optional*"
            else:
                defaultstr = str(val)
        except ValueError:
            try:
                val = float(default)
                if val >= 1e307:
                    defaultstr = "*Optional*"
                else:
                    defaultstr = str(val)
            except ValueError:
                # Fall-back default for anything
                defaultstr = str(default)

        # Replace nonprintable characters with their printable
        # representations, such as \n, \t, ...
        defaultstr = repr(defaultstr)[1:-1]
        defaultstr = defaultstr.replace("\\", "\\\\")

        # A special case for single-character default values (e.g. + or *, see MuonLoad). We don't
        # want them to be interpreted as list items.
        if len(defaultstr) == 1 and defaultstr in punctuation:
            defaultstr = "\\" + defaultstr

        # Values ending with underscores should just be literals
        if defaultstr.endswith("_"):
            defaultstr = defaultstr[:-1] + "\\_"

        # Replace the ugly default values with "Optional"
        if (defaultstr == "8.9884656743115785e+307") or (defaultstr == "1.7976931348623157e+308") or (defaultstr == "2147483647"):
            defaultstr = "*Optional*"

        if str(prop.type) == "boolean":
            if defaultstr == "1":
                defaultstr = "True"
            else:
                defaultstr = "False"

        if str(prop.type) == "Dictionary":
            if defaultstr == r"null\\n":
                defaultstr = "dict()"

        return defaultstr

    def _create_property_description_string(self, prop):
        """
        Converts the description of the property to a more use-friendly one.

        Args:
          prop. The property to find the default value of.

        Returns:
          str: The string to add to the property table description section.
        """
        from mantid.api import IWorkspaceProperty

        desc = str(prop.documentation.replace("\n", " "))

        allowedValueString = str(prop.allowedValues)
        # 4 allows for ['']
        if len(allowedValueString) > 4 and not isinstance(prop, IWorkspaceProperty):
            ##make sure the last sentence ended with a full stop (or equivalent)
            if (
                (not desc.rstrip().endswith("."))
                and (not desc.rstrip().endswith("!"))
                and (not desc.rstrip().endswith("?"))
                and (len(desc.strip()) > 0)
            ):
                desc += "."
            isFileExts = True
            for item in prop.allowedValues:
                # check it does not look like a file extension
                if (not item.startswith(".")) and (not item[-4:].startswith(".")):
                    isFileExts = False
                    break

            prefixString = " Allowed values: "
            if isFileExts:
                prefixString = " Allowed extensions: "
            # put a space in between entries to allow the line to break
            allowedValueString = allowedValueString.replace("','", "', '")
            desc += prefixString + allowedValueString

        return self._escape_subsitution_refs(desc)

    def _escape_subsitution_refs(self, desc):
        """
        Find occurrences of text surrounded by vertical bars and assume they
        are not docutils substitution referencess by esacping them
        """

        def repl(match):
            return r"\|" + match.group(1) + r"\|"

        return SUBSTITUTE_REF_RE.sub(repl, desc)


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive("properties", PropertiesDirective)
