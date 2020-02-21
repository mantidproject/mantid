# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""  The GuiStateDirector generates the state object from the models.

The GuiStateDirector gets the information from the table and state model and generates state objects. It delegates
the main part of the work to an StateDirectorISIS object.
"""

from __future__ import (absolute_import, division, print_function)

import copy

from sans.common.file_information import SANSFileInformationBlank
from sans.state.StateRunDataBuilder import StateRunDataBuilder
from sans.state.StateBuilder import StateBuilder
from sans.state.StateObjects.StateData import get_data_builder
from sans.user_file.txt_parsers.CommandInterfaceAdapter import CommandInterfaceAdapter


class GuiStateDirector(object):
    def __init__(self, state_gui_model, facility):
        self._state_gui_model = state_gui_model
        self._facility = facility

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def create_state(self, row_entry, file_lookup=True, user_file=""):
        # 1. Get the data settings, such as sample_scatter, etc... and create the data state.
        if file_lookup:
            file_information = row_entry.file_information
        else:
            file_information = SANSFileInformationBlank()

        data_builder = get_data_builder(self._facility, file_information)

        self._set_data_entry(data_builder.set_sample_scatter, row_entry.sample_scatter)
        self._set_data_period_entry(data_builder.set_sample_scatter_period, row_entry.sample_scatter_period)
        self._set_data_entry(data_builder.set_sample_transmission, row_entry.sample_transmission)
        self._set_data_period_entry(data_builder.set_sample_transmission_period, row_entry.sample_transmission_period)  # noqa
        self._set_data_entry(data_builder.set_sample_direct, row_entry.sample_direct)
        self._set_data_period_entry(data_builder.set_sample_direct_period, row_entry.sample_direct_period)
        self._set_data_entry(data_builder.set_can_scatter, row_entry.can_scatter)
        self._set_data_period_entry(data_builder.set_can_scatter_period, row_entry.can_scatter_period)
        self._set_data_entry(data_builder.set_can_transmission, row_entry.can_transmission)
        self._set_data_period_entry(data_builder.set_can_transmission_period, row_entry.can_transmission_period)
        self._set_data_entry(data_builder.set_can_direct, row_entry.can_direct)
        self._set_data_period_entry(data_builder.set_can_direct_period, row_entry.can_direct_period)

        data = data_builder.build()

        # 2. Add elements from the options column
        state_gui_model = copy.deepcopy(self._state_gui_model)
        self._apply_column_options_to_state(row_entry, state_gui_model)

        # 3. Add other columns
        output_name = row_entry.output_name
        if output_name:
            state_gui_model.output_name = output_name

        if row_entry.sample_thickness:
            state_gui_model.sample_thickness = float(row_entry.sample_thickness)
        if row_entry.sample_height:
            state_gui_model.sample_height = float(row_entry.sample_height)
        if row_entry.sample_width:
            state_gui_model.sample_width = float(row_entry.sample_width)
        if row_entry.sample_shape:
            state_gui_model.sample_shape = row_entry.sample_shape

        # 4. Create the rest of the state based on the builder.
        settings = copy.deepcopy(state_gui_model.settings)
        command_interface = CommandInterfaceAdapter(data_info=data, processed_state=settings)
        run_data_builder = StateRunDataBuilder(file_information=file_information)

        state = StateBuilder(run_data_builder=run_data_builder, i_state_parser=command_interface).get_all_states()
        return state

    @staticmethod
    def _set_data_entry(func, entry):
        if entry:
            func(entry)

    @staticmethod
    def _set_data_period_entry(func, entry):
        if entry:
            # Ensure that it is convertible to an integer and that it is larger than 0
            try:
                entry_as_integer = int(entry)
                if entry_as_integer > 0:
                    func(entry_as_integer)
            except ValueError:  # noqa
                pass

    @staticmethod
    def _apply_column_options_to_state(table_index_model, state_gui_model):
        """
        Apply the column setting of the user to the state for that particular row.

        Note if you are extending the options functionality, then you will have to add the property here.
        :param options_column_model: the option column model with the row specific settings
        :param state_gui_model: the state gui model
        """
        options = table_index_model.options.get_options_dict()

        # Here we apply the correction to the state depending on the settings in the options. This is not very nice,
        # but currently it is not clear how to solve this differently.
        if "WavelengthMin" in options.keys():
            state_gui_model.wavelength_min = options["WavelengthMin"]

        if "WavelengthMax" in options.keys():
            state_gui_model.wavelength_max = options["WavelengthMax"]

        if "EventSlices" in options.keys():
            state_gui_model.event_slices = options["EventSlices"]

        if "MergeScale" in options.keys():
            state_gui_model.merge_scale = options["MergeScale"]

        if "MergeShift" in options.keys():
            state_gui_model.merge_shift = options["MergeShift"]

        if "PhiMin" in options.keys():
            state_gui_model.phi_limit_min = options["PhiMin"]

        if "PhiMax" in options.keys():
            state_gui_model.phi_limit_max = options["PhiMax"]

        if "UseMirror" in options.keys():
            state_gui_model.phi_limit_use_mirror = options["UseMirror"]
