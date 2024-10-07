# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The GuiStateDirector generates the state object from the models.

The GuiStateDirector gets the information from the table and state model and generates state objects. It delegates
the main part of the work to an StateDirectorISIS object.
"""

import copy
import os

from mantid import FileFinder
from sans.common.file_information import SANSFileInformationBlank
from sans.common.RowEntries import RowEntries
from mantidqtinterfaces.sans_isis.gui_logic.models.file_loading import FileLoading
from mantidqtinterfaces.sans_isis.gui_logic.models.state_gui_model import StateGuiModel
from sans.state.StateObjects.StateData import get_data_builder


class GuiStateDirector(object):
    def __init__(self, state_gui_model: StateGuiModel, facility):
        self._state_gui_model = state_gui_model
        self._facility = facility

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    def create_state(self, row_entry: RowEntries, file_lookup=True, row_user_file: str = None) -> StateGuiModel:
        """
        Packs the current GUI options (e.g. settings selection) into a state object for the current row
        :param row_entry: The associated row entry
        :param file_lookup: Whether to lookup file information whilst creating state
        :param row_user_file: (Optional) The name of an overriding user file if one is provided. In this case the
        GUI options selected will be ignored.
        :return: A populated StateGuiModel
        """

        # 1. Get the data settings, such as sample_scatter, etc... and create the data state.
        if file_lookup:
            file_information = row_entry.file_information
        else:
            file_information = SANSFileInformationBlank()

        gui_state = self._load_current_state(file_information, row_user_file)

        data_builder = get_data_builder(self._facility, file_information)
        self._populate_row_entries(data_builder, row_entry)

        # 2. Add elements from the options column
        self._apply_column_options_to_state(row_entry, gui_state)

        # 3. Add other columns
        if row_user_file:
            # We need to copy these particular settings from the GUI when custom user file settings are being applied
            gui_state.all_states.save = copy.deepcopy(self._state_gui_model.all_states.save)
            gui_state.reduction_dimensionality = self._state_gui_model.reduction_dimensionality

        gui_state.output_name = row_entry.output_name if row_entry.output_name else None

        if row_entry.sample_thickness:
            gui_state.sample_thickness = float(row_entry.sample_thickness)
        if row_entry.sample_height:
            gui_state.sample_height = float(row_entry.sample_height)
        if row_entry.sample_width:
            gui_state.sample_width = float(row_entry.sample_width)
        if row_entry.sample_shape:
            gui_state.sample_shape = row_entry.sample_shape

        if row_entry.background_ws:
            gui_state.background_workspace = row_entry.background_ws
        if row_entry.scale_factor:
            gui_state.scale_factor = row_entry.scale_factor

        # 4. Create the rest of the state based on the builder.
        gui_state.all_states.data = data_builder.build()
        return gui_state

    def _populate_row_entries(self, data_builder, row_entry):
        self._set_data_entry(data_builder.set_sample_scatter, row_entry.sample_scatter)
        self._set_data_period_entry(data_builder.set_sample_scatter_period, row_entry.sample_scatter_period)
        self._set_data_entry(data_builder.set_sample_transmission, row_entry.sample_transmission)
        self._set_data_period_entry(data_builder.set_sample_transmission_period, row_entry.sample_transmission_period)
        self._set_data_entry(data_builder.set_sample_direct, row_entry.sample_direct)
        self._set_data_period_entry(data_builder.set_sample_direct_period, row_entry.sample_direct_period)
        self._set_data_entry(data_builder.set_can_scatter, row_entry.can_scatter)
        self._set_data_period_entry(data_builder.set_can_scatter_period, row_entry.can_scatter_period)
        self._set_data_entry(data_builder.set_can_transmission, row_entry.can_transmission)
        self._set_data_period_entry(data_builder.set_can_transmission_period, row_entry.can_transmission_period)
        self._set_data_entry(data_builder.set_can_direct, row_entry.can_direct)
        self._set_data_period_entry(data_builder.set_can_direct_period, row_entry.can_direct_period)

    def _load_current_state(self, file_information, row_user_file):
        if row_user_file:
            # Has a custom user file so ignore any settings from GUI
            user_file_path = FileFinder.getFullPath(row_user_file)
            if not os.path.exists(user_file_path):
                raise ValueError(f"The user file '{row_user_file}'" " cannot be found. Make sure a valid user file has been specified.")
            state = FileLoading.load_user_file(user_file_path, file_information=file_information)
            gui_state = StateGuiModel(state)
        else:
            # We want to copy our master GUI state to set any row options (see prototype pattern)
            gui_state = copy.deepcopy(self._state_gui_model)
        return gui_state

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
            except ValueError:
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
