""" The table  model contains all the reduction information which is provided via the data table

The main information in the table model are the run numbers and the selected periods. However it also contains
information regarding the custom output name and the information in the options tab.
"""

from __future__ import (absolute_import, division, print_function)

import os
import re

from sans.gui_logic.sans_data_processor_gui_algorithm import create_option_column_properties
from sans.common.constants import ALL_PERIODS


class TableModel(object):
    column_name_converter = ["sample_scatter", "sample_scatter_period", "sample_transmission",
                             "sample_transmission_period", "sample_direct", "sample_direct_period",
                             "can_scatter", "can_scatter_period",
                             "can_transmission", "can_transmission_period", "can_direct", "can_direct_period",
                             "output_name", "user_file", "sample_thickness", "options_column_model"]

    def __init__(self):
        super(TableModel, self).__init__()
        self._user_file = ""
        self._batch_file = ""
        self._table_entries = []

    @staticmethod
    def _validate_file_name(file_name):
        if not file_name:
            return
        if not os.path.exists(file_name):
            raise ValueError("The file {} does not seem to exist.".format(file_name))

    @property
    def user_file(self):
        return self._user_file

    @user_file.setter
    def user_file(self, value):
        self._validate_file_name(value)
        self._user_file = value

    def get_row_user_file(self, row_index):
        if row_index < len(self._table_entries):
            return self._table_entries[row_index].user_file
        else:
            raise IndexError("The row {} does not exist.".format(row_index))

    @property
    def batch_file(self):
        return self._batch_file

    @batch_file.setter
    def batch_file(self, value):
        # self._validate_file_name(value)
        self._batch_file = value

    def get_table_entry(self, index):
        return self._table_entries[index]

    def add_table_entry(self, row, table_index_model):
        self._table_entries.insert(row, table_index_model)

    def append_table_entry(self, table_index_model):
        self._table_entries.append(table_index_model)

    def remove_table_entries(self, rows):
        # For speed rows should be a Set here but don't think it matters for the list sizes involved.
        self._table_entries[:] = [item for i,item in enumerate(self._table_entries) if i not in rows]

    def clear_table_entries(self):
        self._table_entries = []

    def get_number_of_rows(self):
        return len(self._table_entries)

    def update_table_entry(self, row, column, value):
        self._table_entries[row].update_attribute(self.column_name_converter[column], value)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__


class TableIndexModel(object):
    def __init__(self, sample_scatter, sample_scatter_period,
                 sample_transmission, sample_transmission_period,
                 sample_direct, sample_direct_period,
                 can_scatter, can_scatter_period,
                 can_transmission, can_transmission_period,
                 can_direct, can_direct_period,
                 output_name="", user_file="", sample_thickness='0.0', options_column_string=""):
        super(TableIndexModel, self).__init__()
        self.sample_scatter = sample_scatter
        self.sample_scatter_period = sample_scatter_period
        self.sample_transmission = sample_transmission
        self.sample_transmission_period = sample_transmission_period
        self.sample_direct = sample_direct
        self.sample_direct_period = sample_direct_period

        self.can_scatter = can_scatter
        self.can_scatter_period = can_scatter_period
        self.can_transmission = can_transmission
        self.can_transmission_period = can_transmission_period
        self.can_direct = can_direct
        self.can_direct_period = can_direct_period

        self.user_file = user_file
        self.sample_thickness = sample_thickness
        self.output_name = output_name

        self.options_column_model = options_column_string

    # Options column entries
    @property
    def options_column_model(self):
        return self._options_column_model

    @options_column_model.setter
    def options_column_model(self, value):
        self._options_column_model = OptionsColumnModel(value)

    def update_attribute(self, attribute_name, value):
        setattr(self, attribute_name, value)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__

    def toList(self):
        return [self.sample_scatter, self._string_period(self.sample_scatter_period), self.sample_transmission,
                self._string_period(self.sample_transmission_period),self.sample_direct,
                self._string_period(self.sample_direct_period), self.can_scatter,
                self._string_period(self.can_scatter_period), self.can_transmission,
                self._string_period(self.can_transmission_period), self.can_direct,
                self._string_period(self.can_direct_period), self.output_name, self.user_file, self.sample_thickness,
                self.options_column_model.get_options_string()]

    def isMultiPeriod(self):
        return any ((self.sample_scatter_period, self.sample_transmission_period ,self.sample_direct_period,
                     self.can_scatter_period, self.can_transmission_period, self.can_direct_period))

    def _string_period(self, _tag):
        return "" if _tag == ALL_PERIODS else str(_tag)


class OptionsColumnModel(object):
    def __init__(self, options_column_string):
        super(OptionsColumnModel, self).__init__()
        self._options_column_string = options_column_string
        self._options = self._get_options(self._options_column_string)

    def get_options(self):
        return self._options

    def get_options_string(self):
        return self._options_column_string

    @staticmethod
    def _get_permissible_properties():
        props = {}
        option_column_properties = create_option_column_properties()
        for element in option_column_properties:
            props.update({element.algorithm_property: element.property_type})
        return props

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
        options_column_string_no_whitespace = options_column_string_no_whitespace.replace('"', '')
        options_column_string_no_whitespace = options_column_string_no_whitespace.replace("'", '')

        if not options_column_string_no_whitespace:
            return parsed

        # This is a regular expression to detect key value pairs in the options string.
        # The different parts are:
        # ([^,=]+) Anything except equals detects keys in the options string
        # =* then an equals sign
        # ((?:[^,=]+(?:,|$))*) Any number of repetitions of a string without = followed by a comma or end of input.
        option_pattern = re.compile(r'''([^,=]+)=*((?:[^,=]+(?:,|$))*)''')

        # The findall option finds all instances of the pattern specified above in the options string.
        for key, value in option_pattern.findall(options_column_string_no_whitespace):
            if value.endswith(','):
                value=value[:-1]
            parsed.update({key:value})

        return parsed

    @staticmethod
    def _get_options(options_column_string):
        """
        Gets all allowed values from the options column.

        :param options_column_string: the string in the options column
        :return: a dict with all options
        """
        parsed_options = OptionsColumnModel._parse_string(options_column_string)
        permissible_properties = OptionsColumnModel._get_permissible_properties()

        options = {}
        for key, value in parsed_options.items():
            if key in permissible_properties.keys():
                conversion_functions = permissible_properties[key]
                options.update({key: conversion_functions(value)})
        return options

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__
