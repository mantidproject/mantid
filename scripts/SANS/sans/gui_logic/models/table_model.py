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

from mantid.kernel import Logger
from sans.common.enums import RowState
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.basic_hint_strategy import BasicHintStrategy
from ui.sans_isis.work_handler import WorkHandler


class TableModel(object):
    column_name_converter = ["sample_scatter", "sample_scatter_period", "sample_transmission",
                             "sample_transmission_period", "sample_direct", "sample_direct_period",
                             "can_scatter", "can_scatter_period",
                             "can_transmission", "can_transmission_period", "can_direct", "can_direct_period",
                             "output_name", "user_file", "sample_thickness", "sample_height", "sample_width",
                             "sample_shape", "options_column_model"]
    THICKNESS_ROW = 14

    logger = Logger("ISIS SANS GUI")

    def __init__(self):
        super(TableModel, self).__init__()
        self.batch_file = ""
        self.user_file = ""
        self.work_handler = WorkHandler()
        self._subscriber_list = []

        self._table_entries = []
        self._default_entry_added = None
        self.clear_table_entries()

    def get_row_user_file(self, row_index):
        if row_index < len(self._table_entries):
            return self._table_entries[row_index].user_file
        else:
            raise IndexError("The row {} does not exist.".format(row_index))

    def add_multiple_table_entries(self, table_index_model_list):
        for row in table_index_model_list:
            self._add_single_table_entry(table_index_model=row)
        self.notify_subscribers()

    def append_table_entry(self, table_index_model):
        self._add_single_table_entry(table_index_model=table_index_model)
        self.notify_subscribers()

    def get_all_rows(self):
        return self._table_entries

    def get_row(self, index):
        return self._table_entries[index]

    def get_row_index(self, row):
        return self._table_entries.index(row)

    def replace_table_entry(self, row, table_index_model):
        self._add_single_table_entry(row=row, table_index_model=table_index_model)
        self.notify_subscribers()

    def remove_table_entries(self, row_indices):
        new_table_entries = [item for i, item in enumerate(self._table_entries) if i not in row_indices]

        if not new_table_entries:
            self.clear_table_entries()
        else:
            self._table_entries = new_table_entries

        self.notify_subscribers()

    def _add_single_table_entry(self, table_index_model, row=None):
        assert isinstance(table_index_model, RowEntries), \
            "%r is not a RowEntries object" % table_index_model

        assert len(self._table_entries) >= 1, \
            "There must always be 1 element in the model, currently there is 0"

        if self._default_entry_added:
            del self._table_entries[0]
            self._default_entry_added = False

        if row is None or row == len(self._table_entries):
            return self._table_entries.append(table_index_model)

        self._table_entries[row] = table_index_model

    def replace_table_entries(self, row_to_replace_index, rows_to_insert):
        starting_index = row_to_replace_index[0]
        for i, row_entry in enumerate(reversed(rows_to_insert)):
            self.replace_table_entry(starting_index + i, row_entry)

    def clear_table_entries(self):
        self._table_entries = [RowEntries()]
        self._default_entry_added = True

    def get_number_of_rows(self):
        return len(self._table_entries)

    def is_empty_row(self, row):
        return self._table_entries[row].is_empty()

    def get_non_empty_rows(self):
        return len([x for x in self._table_entries if not x.is_empty()])

    @staticmethod
    def get_options_hint_strategy():
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
    def get_sample_shape_hint_strategy():
        return BasicHintStrategy({"Cylinder": "",
                                  "Disc": "",
                                  "FlatPlate": ""})

    def set_row_to_processed(self, row, tool_tip):
        self._table_entries[row].state = RowState.PROCESSED
        self._table_entries[row].tool_tip = tool_tip
        self.notify_subscribers()

    def reset_row_state(self, row):
        self._table_entries[row].state = RowState.UNPROCESSED
        self._table_entries[row].tool_tip = None

    def set_row_to_error(self, row, msg):
        self._table_entries[row].state = RowState.ERROR
        self._table_entries[row].tool_tip = msg
        self.notify_subscribers()

    def failure_handler(self, entry, error):
        entry.file_information = None
        entry.sample_thickness = None
        entry.sample_height = None
        entry.sample_width = None
        entry.sample_shape = None
        self.set_row_to_error(entry, str(error[1]))


    def subscribe_to_model_changes(self, subscriber):
        self._subscriber_list.append(subscriber)

    def notify_subscribers(self):
        for subscriber in self._subscriber_list:
            subscriber.on_update_rows()

    def __eq__(self, other):
        return self.equal_dicts(self.__dict__, other.__dict__, ['work_handler'])

    def __ne__(self, other):
        return not self.equal_dicts(self.__dict__, other.__dict__, ['work_handler'])

    @staticmethod
    def equal_dicts(d1, d2, ignore_keys):
        d1_filtered = dict((k, v) for k, v in d1.items() if k not in ignore_keys)
        d2_filtered = dict((k, v) for k, v in d2.items() if k not in ignore_keys)
        return d1_filtered == d2_filtered
