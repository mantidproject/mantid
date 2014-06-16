from base import BaseDirective
import string


class PropertiesDirective(BaseDirective):

    """
    Outputs the given algorithm's properties into a ReST formatted table.
    """
    # Accept one required argument and no optional arguments.
    required_arguments, optional_arguments = 0, 0

    def run(self):
        """
        Called by Sphinx when the ..properties:: directive is encountered.
        """
        if self._create_properties_table():
            self.commit_rst()

        return []

    def _create_properties_table(self):
        """
        Populates the ReST table with algorithm properties.
        """
        alg = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version())
        alg_properties = alg.getProperties()
        if len(alg_properties) == 0:
            return False

        # Stores each property of the algorithm in a tuple.
        properties = []

        # Used to obtain the name for the direction property rather than an
        # int.
        direction_string = ["Input", "Output", "InOut", "None"]

        for prop in alg_properties:
            # Append a tuple of properties to the list.
            properties.append((
                str(prop.name),
                str(direction_string[prop.direction]),
                str(prop.type),
                str(self._get_default_prop(prop)),
                str(prop.documentation.replace("\n", " "))
            ))

        self.add_rst(self.make_header("Properties"))
        self.add_rst(self._build_table(properties))
        return True

    def _build_table(self, table_content):
        """
        Build the ReST format

        Args:
          table_content (list of tuples): Each tuple (row) container
          property values for a unique property of that algorithm.

        Returns:
          str: ReST formatted table containing algorithm properties.
        """
        # Default values for the table headers.
        header_content = (
            'Name', 'Direction', 'Type', 'Default', 'Description')
        # The width of the columns. Multiply row length by 10 to ensure small
        # properties format correctly.
        # Added 10 to the length to ensure if table_content is 0 that
        # the table is still displayed.
        col_sizes = [max( (len(row[i] * 10) + 10) for row in table_content)
                for i in range(len(header_content))]

        # Use the column widths as a means to formatting columns.
        formatter = ' '.join('{%d:<%d}' % (index,col) for index, col in enumerate(col_sizes))
        # Add whitespace to each column. This depends on the values returned by
        # col_sizes.
        table_content_formatted = [
            formatter.format(*item) for item in table_content]
        # Create a seperator for each column
        seperator = formatter.format(*['=' * col for col in col_sizes])
        # Build the table.
        header = '\n' + seperator + '\n' + formatter.format(*header_content) + '\n'
        content = seperator + '\n' + \
            '\n'.join(table_content_formatted) + '\n' + seperator
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
        if (direction_string[prop.direction] == "Output") and \
           (not isinstance(prop, IWorkspaceProperty)):
            default_prop = ""
        elif (prop.isValid == ""):
            default_prop = self._create_property_default_string(prop)
        else:
            default_prop = "Mandatory"
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
            if (val >= 2147483647):
                defaultstr = "Optional"
            else:
                defaultstr = str(val)
        except:
            try:
                val = float(default)
                if (val >= 1e+307):
                    defaultstr = "Optional"
                else:
                    defaultstr = str(val)
            except:
                # Fall-back default for anything
                defaultstr = str(default)

        # A special case for single-character default values (e.g. + or *, see MuonLoad). We don't
        # want them to be interpreted as list items.
        if len(defaultstr) == 1 and defaultstr in string.punctuation:
            defaultstr = "\\" + defaultstr

        # Replace the ugly default values with "Optional"
        if (defaultstr == "8.9884656743115785e+307") or \
           (defaultstr == "1.7976931348623157e+308") or \
           (defaultstr == "2147483647"):
            defaultstr = "Optional"

        if str(prop.type) == "boolean":
            if defaultstr == "1":
                defaultstr = "True"
            else:
                defaultstr = "False"

        return defaultstr


def setup(app):
    """
    Setup the directives when the extension is activated

    Args:
      app: The main Sphinx application object
    """
    app.add_directive('properties', PropertiesDirective)
