""" The table  model contains all the reduction information which is provided via the data table

The main information in the table model are the run numbers and the selected periods. However it also contains
information regarding the custom output name and the information in the options tab.
"""

from __future__ import (absolute_import, division, print_function)

import os

from sans.gui_logic.sans_data_processor_gui_algorithm import create_option_column_properties
from sans.gui_logic.gui_common import OPTIONS_SEPARATOR, OPTIONS_EQUAL


class TableModel(object):
    def __init__(self):
        super(TableModel, self).__init__()
        self._user_file = ""
        self._batch_file = ""
        self._table_entries = {}

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

    @property
    def batch_file(self):
        return self._batch_file

    @batch_file.setter
    def batch_file(self, value):
        self._validate_file_name(value)
        self._batch_file = value

    def get_table_entry(self, index):
        if index not in self._table_entries:
            raise ValueError("The {}th row entry does not exist".format(index))
        return self._table_entries[index]

    def add_table_entry(self, row, table_index_model):
        self._table_entries.update({row: table_index_model})

    def clear_table_entries(self):
        self._table_entries = {}


class TableIndexModel(object):
    def __init__(self, index, sample_scatter, sample_transmission, sample_direct,
                 can_scatter, can_transmission, can_direct, output_name="",
                 options_column_string=""):
        super(TableIndexModel, self).__init__()
        self.index = index
        self.sample_scatter = sample_scatter
        self.sample_transmission = sample_transmission
        self.sample_direct = sample_direct

        self.can_scatter = can_scatter
        self.can_transmission = can_transmission
        self.can_direct = can_direct

        self.user_file = ""
        self.output_name = output_name

        # Options column entries
        self.options_column_model = OptionsColumnModel(options_column_string)


class OptionsColumnModel(object):
    def __init__(self, options_column_string):
        super(OptionsColumnModel, self).__init__()
        self._options_column_string = options_column_string
        self._options = self._get_options(self._options_column_string)

    def get_options(self):
        return self._options

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
        parsed = {}
        # Remove all white space
        options_column_string_no_whitespace = "".join(options_column_string.split())
        if not options_column_string_no_whitespace:
            return parsed

        # Split by the comma
        elements = options_column_string_no_whitespace.split(OPTIONS_SEPARATOR)
        for element in elements:
            key, value = element.split(OPTIONS_EQUAL)
            parsed.update({key: value})
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
