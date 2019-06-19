# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The table  model contains all the reduction information which is provided via the data table

The main information in the table model are the run numbers and the selected periods. However it also contains
information regarding the custom output name and the information in the options tab.
"""

from __future__ import (absolute_import, division, print_function)

import functools
import os
import re

from sans.common.constants import ALL_PERIODS
from sans.common.enums import RowState, SampleShape
from sans.common.file_information import SANSFileInformationFactory
from sans.gui_logic.models.basic_hint_strategy import BasicHintStrategy
from sans.gui_logic.presenter.create_file_information import create_file_information
from ui.sans_isis.work_handler import WorkHandler


class TableModel(object):
    column_name_converter = ["sample_scatter", "sample_scatter_period", "sample_transmission",
                             "sample_transmission_period", "sample_direct", "sample_direct_period",
                             "can_scatter", "can_scatter_period",
                             "can_transmission", "can_transmission_period", "can_direct", "can_direct_period",
                             "output_name", "user_file", "sample_thickness", "sample_height", "sample_width",
                             "sample_shape", "options_column_model"]
    THICKNESS_ROW = 14

    def __init__(self):
        super(TableModel, self).__init__()
        self._user_file = ""
        self._batch_file = ""
        self._table_entries = []
        self.work_handler = WorkHandler()
        self._subscriber_list = []
        self._id_count = 0

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
        self._batch_file = value

    def get_table_entry(self, index):
        return self._table_entries[index]

    def add_table_entry(self, row, table_index_model):
        table_index_model.id = self._id_count
        self._id_count += 1
        self._table_entries.insert(row, table_index_model)
        if row >= self.get_number_of_rows():
            row = self.get_number_of_rows() - 1
        self.get_thickness_for_rows([row])
        self.notify_subscribers()

    def append_table_entry(self, table_index_model):
        table_index_model.id = self._id_count
        self._id_count += 1
        self._table_entries.append(table_index_model)
        self.get_thickness_for_rows([self.get_number_of_rows() - 1])
        self.notify_subscribers()

    def remove_table_entries(self, rows):
        # For speed rows should be a Set here but don't think it matters for the list sizes involved.
        self._table_entries[:] = [item for i, item in enumerate(self._table_entries) if i not in rows]
        if not self._table_entries:
            row_index_model = self.create_empty_row()
            self.append_table_entry(row_index_model)
        else:
            self.notify_subscribers()

    def replace_table_entries(self, row_to_replace_index, rows_to_insert):
        self.remove_table_entries(row_to_replace_index)
        for row_entry in reversed(rows_to_insert):
            self.add_table_entry(row_to_replace_index[0], row_entry)

    def clear_table_entries(self):
        self._table_entries = []
        row_index_model = self.create_empty_row()
        self.append_table_entry(row_index_model)

    def get_number_of_rows(self):
        return len(self._table_entries)

    def update_table_entry(self, row, column, value):
        self._table_entries[row].update_attribute(self.column_name_converter[column], value)
        self._table_entries[row].update_attribute('row_state', RowState.Unprocessed)
        self._table_entries[row].update_attribute('tool_tip', '')
        if column == 0:
            self.get_thickness_for_rows([row])
        self.notify_subscribers()

    def is_empty_row(self, row):
        return self._table_entries[row].is_empty()

    @staticmethod
    def create_empty_row():
        row = [''] * 16
        return TableIndexModel(*row)

    def get_non_empty_rows(self, rows):
        return list(filter(lambda x: not self.get_table_entry(x).is_empty(), rows))

    def get_options_hint_strategy(self):
        return OptionsColumnModel.get_hint_strategy()

    def get_sample_shape_hint_strategy(self):
        return SampleShapeColumnModel.get_hint_strategy()

    def set_row_to_processed(self, row, tool_tip):
        self._table_entries[row].update_attribute('row_state', RowState.Processed)
        self._table_entries[row].update_attribute('tool_tip', tool_tip)
        self.notify_subscribers()

    def reset_row_state(self, row):
        self._table_entries[row].update_attribute('row_state', RowState.Unprocessed)
        self._table_entries[row].update_attribute('tool_tip', '')
        self.notify_subscribers()

    def set_row_to_error(self, row, tool_tip):
        self._table_entries[row].update_attribute('row_state', RowState.Error)
        self._table_entries[row].update_attribute('tool_tip', tool_tip)
        self.notify_subscribers()

    def get_thickness_for_rows(self, rows=None):
        """
        Read in the sample thickness for the given rows from the file and set it in the table.
        :param rows: list of table rows
        """
        if not rows:
            rows = range(len(self._table_entries))
        for row in rows:
            entry = self._table_entries[row]
            # If the row is empty, or a sample thickness has been provided in the batch file
            # we do not need to get thickness for row
            if entry.is_empty() or entry.sample_thickness != '':
                continue
            entry.file_finding = True
            success_callback = functools.partial(self.update_thickness_from_file_information, entry.id)

            error_callback = functools.partial(self.failure_handler, entry.id)
            create_file_information(entry.sample_scatter, error_callback, success_callback,
                                    self.work_handler, entry.id)

    def failure_handler(self, id, error):
        row = self.get_row_from_id(id)
        self._table_entries[row].update_attribute('file_information', '')
        self._table_entries[row].update_attribute('sample_thickness', '')
        self._table_entries[row].update_attribute('sample_height', '')
        self._table_entries[row].update_attribute('sample_width', '')
        self._table_entries[row].update_attribute('sample_shape', '')
        self._table_entries[row].file_finding = False
        self.set_row_to_error(row, str(error[1]))

    def update_thickness_from_file_information(self, id, file_information):
        row = self.get_row_from_id(id)
        if file_information:
            rounded_file_thickness = round(file_information.get_thickness(), 2)
            rounded_file_height = round(file_information.get_height(), 2)
            rounded_file_width = round(file_information.get_width(), 2)

            self._table_entries[row].update_attribute('file_information', file_information)
            self._table_entries[row].update_attribute('sample_thickness', rounded_file_thickness)
            self._table_entries[row].update_attribute('sample_height', rounded_file_height)
            self._table_entries[row].update_attribute('sample_width', rounded_file_width)
            if self._table_entries[row].sample_shape_string == "":
                self._table_entries[row].update_attribute('sample_shape', file_information.get_shape())
            self._table_entries[row].file_finding = False
            self.reset_row_state(row)

    def subscribe_to_model_changes(self, subscriber):
        self._subscriber_list.append(subscriber)

    def notify_subscribers(self):
        for subscriber in self._subscriber_list:
            subscriber.on_update_rows()

    def get_file_information_for_row(self, row):
        return self._table_entries[row].file_information

    def get_row_from_id(self, id):
        for row, entry in enumerate(self._table_entries):
            if entry.id == id:
                return row
        return None

    def wait_for_file_finding_done(self):
        self.work_handler.wait_for_done()

    def wait_for_file_information(self, row):
        if self._table_entries[row].file_finding:
            self.wait_for_file_finding_done()

    def add_table_entry_no_thread_or_signal(self, row, table_index_model):
        table_index_model.id = self._id_count
        self._id_count += 1
        self._table_entries.insert(row, table_index_model)
        if row >= self.get_number_of_rows():
            row = self.get_number_of_rows() - 1

        entry = self._table_entries[row]
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(entry.sample_scatter)
        self.update_thickness_from_file_information(entry.id, file_information)

    def set_option(self, row, key, value):
        self._table_entries[row].options_column_model.set_option(key, value)

    def __eq__(self, other):
        return self.equal_dicts(self.__dict__, other.__dict__, ['work_handler'])

    def __ne__(self, other):
        return not self.equal_dicts(self.__dict__, other.__dict__, ['work_handler'])

    @staticmethod
    def equal_dicts(d1, d2, ignore_keys):
        d1_filtered = dict((k, v) for k, v in d1.items() if k not in ignore_keys)
        d2_filtered = dict((k, v) for k, v in d2.items() if k not in ignore_keys)
        return d1_filtered == d2_filtered


class TableIndexModel(object):
    def __init__(self, sample_scatter, sample_scatter_period,
                 sample_transmission, sample_transmission_period,
                 sample_direct, sample_direct_period,
                 can_scatter, can_scatter_period,
                 can_transmission, can_transmission_period,
                 can_direct, can_direct_period,
                 output_name="", user_file="", sample_thickness='', sample_height='', sample_width='',
                 sample_shape='', options_column_string=""):
        super(TableIndexModel, self).__init__()
        self.id = None
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
        self.sample_height = sample_height
        self.sample_width = sample_width
        self.output_name = output_name

        self.options_column_model = options_column_string
        self.sample_shape_model = SampleShapeColumnModel()
        self.sample_shape = sample_shape

        self.row_state = RowState.Unprocessed

        self.tool_tip = ''
        self.file_information = None
        self.file_finding = False

    # Options column entries
    @property
    def options_column_model(self):
        return self._options_column_model

    @options_column_model.setter
    def options_column_model(self, value):
        self._options_column_model = OptionsColumnModel(value)

    # Sample shape
    @property
    def sample_shape_string(self):
        return self.sample_shape_model.sample_shape_string

    @property
    def sample_shape(self):
        return self.sample_shape_model.sample_shape

    @sample_shape.setter
    def sample_shape(self, value):
        self.sample_shape_model(value)

    def update_attribute(self, attribute_name, value):
        setattr(self, attribute_name, value)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__

    def to_list(self):
        return [self.sample_scatter, self._string_period(self.sample_scatter_period), self.sample_transmission,
                self._string_period(self.sample_transmission_period), self.sample_direct,
                self._string_period(self.sample_direct_period), self.can_scatter,
                self._string_period(self.can_scatter_period), self.can_transmission,
                self._string_period(self.can_transmission_period), self.can_direct,
                self._string_period(self.can_direct_period), self.output_name, self.user_file, self.sample_thickness,
                self.sample_height, self.sample_width,
                self.sample_shape_string,
                self.options_column_model.get_options_string()]

    def to_batch_list(self):
        """
        Return a list of data in the order as would typically appear in the batch file
        :return: a list containing entries in the row present in the batch file
        """
        return_list = [self.sample_scatter, self.sample_transmission,
                       self.sample_direct, self.can_scatter, self.can_transmission,
                       self.can_direct, self.output_name, self.user_file, self.sample_thickness, self.sample_height,
                       self.sample_width]
        return list(map(lambda item: str(item).strip(), return_list))

    def isMultiPeriod(self):
        return any((self.sample_scatter_period, self.sample_transmission_period, self.sample_direct_period,
                    self.can_scatter_period, self.can_transmission_period, self.can_direct_period))

    def is_empty(self):
        return not any((self.sample_scatter, self.sample_transmission, self.sample_direct, self.can_scatter,
                        self.can_transmission, self.can_direct))

    def _string_period(self, _tag):
        return "" if _tag == ALL_PERIODS else str(_tag)


class OptionsColumnModel(object):
    def __init__(self, options_column_string):
        super(OptionsColumnModel, self).__init__()
        self._options_column_string = options_column_string
        self._options = self._get_options(self._options_column_string)

    def get_options(self):
        return self._options

    def set_option(self, key, value):
        self._options.update({key: value})

    def get_options_string(self):
        return self._serialise_options_dict()

    @staticmethod
    def _get_permissible_properties():
        return {"WavelengthMin": float, "WavelengthMax": float, "EventSlices": str, "MergeScale": float,
                "MergeShift": float, "PhiMin": float, "PhiMax": float, "UseMirror": options_column_bool}

    @staticmethod
    def get_hint_strategy():
        return BasicHintStrategy({"WavelengthMin": 'The min value of the wavelength when converting from TOF.',
                                  "WavelengthMax": 'The max value of the wavelength when converting from TOF.',
                                  "PhiMin": 'The min angle of the detector to accept.'
                                            ' Anti-clockwise from horizontal.',
                                  "PhiMax": 'The max angle of the detector to accept.'
                                            ' Anti-clockwise from horizontal.',
                                  "UseMirror": 'True or False. Whether or not to accept phi angle in opposing quadrant',
                                  "MergeScale": 'The scale applied to the HAB when merging',
                                  "MergeShift": 'The shift applied to the HAB when merging',
                                  "EventSlices": 'The event slices to reduce.'
                                  ' The format is the same as for the event slices'
                                  ' box in settings, however if a comma separated list is given '
                                  'it must be enclosed in quotes'})

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

    def _serialise_options_dict(self):
        return ', '.join(['{}={}'.format(k,self._options[k]) for k in sorted(self._options)])

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


class SampleShapeColumnModel(object):
    SAMPLE_SHAPES = ["cylinder", "disc", "flatplate"]
    SAMPLE_SHAPES_DICT = {"cylinder": "Cylinder",
                          "disc": "Disc",
                          "flatplate": "FlatPlate"}

    def __init__(self):
        self.sample_shape = ""
        self.sample_shape_string = ""

    def __call__(self, original_value):
        self._get_sample_shape(original_value)

    def _get_sample_shape(self, original_value):
        try:
            original_value = SampleShape.to_string(original_value)
        except RuntimeError as e:
            if not isinstance(original_value, str):
                raise ValueError(str(e))

        value = original_value.strip().lower()
        if value == "":
            self.sample_shape = ""
            self.sample_shape_string = ""
        else:
            for shape in SampleShapeColumnModel.SAMPLE_SHAPES:
                if shape.startswith(value):
                    shape_enum_string = SampleShapeColumnModel.SAMPLE_SHAPES_DICT[shape]
                    self.sample_shape = SampleShape.from_string(shape_enum_string)
                    self.sample_shape_string = shape_enum_string
                    break

    @staticmethod
    def get_hint_strategy():
        return BasicHintStrategy({"Cylinder": "",
                                  "Disc": "",
                                  "FlatPlate": ""})

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        return self.__dict__ != other.__dict__


def options_column_bool(string):
    """
    Evaluate input string as a bool. Used for UseMirror in Options column,
    as evaluating bool("False") returns True (any string is Truthy).
    :param string: User input string to be evaluated as False or True
    :return: True or False
    """
    truthy_strings = ("true", "1", "yes", "t", "y")  # t short for true, y short for yes
    falsy_strings = ("false", "0", "no", "f", "n")
    if string.lower() in truthy_strings:
        return True
    elif string.lower() in falsy_strings:
        return False
    else:
        raise ValueError("Could not evaluate {} as a boolean value. It should be True or False.".format(string))
