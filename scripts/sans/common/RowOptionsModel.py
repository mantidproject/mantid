# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re


class RowOptionsModel(object):
    def __init__(self):
        self._user_options = None
        self._developer_options = {}

    def set_user_options(self, val):
        self._user_options = val

    def set_developer_option(self, key, value):
        self._developer_options[key] = value

    def get_displayed_text(self):
        output = self._user_options + "," if self._user_options else ""
        for k, v in self._developer_options.items():
            output += k + "=" + str(v) + ", "

        if output.endswith(", "):
            output = output[:-2]
        return output

    def get_options_dict(self):
        """
        Gets all allowed values from the options column.
        :param options_column_string: the string in the options column
        :return: a dict with all options
        """
        user_text = self.get_displayed_text()
        if not user_text:
            return {}

        parsed_options = self._parse_string(user_text)
        permissible_properties = {
            "WavelengthMin": float,
            "WavelengthMax": float,
            "EventSlices": str,
            "MergeScale": float,
            "MergeShift": float,
            "PhiMin": float,
            "PhiMax": float,
            "UseMirror": bool,
        }

        options = {}
        for key, value in parsed_options.items():
            if key == "UseMirror":
                val = any(v == value for v in ["true", "1", "yes", "True", "t", "y"])
                if not val and not any(v == value for v in ["false", "0", "no", "f", "n", "False"]):
                    raise ValueError("Could not evaluate {} as a boolean value. It should be True or False.".format(value))
                options.update({key: value})
                continue

            if key in permissible_properties.keys():
                conversion_type = permissible_properties[key]
                options.update({key: conversion_type(value)})
        return options

    @staticmethod
    def _parse_string(options_column_string):
        """
        Parses a string of the form "PropName1=value1,PropName2=value2"
        :param options_column_string: the string in the options column
        :return: a dict with parsed values
        """
        # Remove all white space
        parsed = {}
        options_column_string_no_whitespace = "".join(options_column_string.split())
        options_column_string_no_whitespace = options_column_string_no_whitespace.replace('"', "")
        options_column_string_no_whitespace = options_column_string_no_whitespace.replace("'", "")

        if not options_column_string_no_whitespace:
            return parsed

        # This is a regular expression to detect key value pairs in the options string.
        # The different parts are:
        # ([^,=]+) Anything except equals detects keys in the options string
        # =* then an equals sign
        # ((?:[^,=]+(?:,|$))*) Any number of repetitions of a string without = followed by a comma or end of input.
        option_pattern = re.compile(r"""([^,=]+)=*((?:[^,=]+(?:,|$))*)""")

        # The findall option finds all instances of the pattern specified above in the options string.
        for key, value in option_pattern.findall(options_column_string_no_whitespace):
            if value.endswith(","):
                value = value[:-1]
            parsed.update({key: value})

        return parsed
