"""  The GuiStateDirector generates the state object from the models.

The GuiStateDirector gets the information from the table and state model and generates state objects. It delegates
the main part of the work to an StateDirectorISIS object.
"""

from __future__ import (absolute_import, division, print_function)

import copy

from sans.state.data import get_data_builder
from sans.user_file.state_director import StateDirectorISIS


class GuiStateDirector(object):
    def __init__(self, table_model, state_gui_model, facility):
        super(GuiStateDirector, self).__init__()
        self._table_model = table_model
        self._state_gui_model = state_gui_model
        self._facility = facility

    def create_state(self, row):
        # 1. Get the data settings, such as sample_scatter, etc... and create the data state.
        table_index_model = self._table_model.get_table_entry(row)
        data_builder = get_data_builder(self._facility)

        self._set_data_entry(data_builder.set_sample_scatter, table_index_model.sample_scatter)
        self._set_data_entry(data_builder.set_sample_transmission, table_index_model.sample_transmission)
        self._set_data_entry(data_builder.set_sample_direct, table_index_model.sample_direct)
        self._set_data_entry(data_builder.set_can_scatter, table_index_model.can_scatter)
        self._set_data_entry(data_builder.set_can_transmission, table_index_model.can_transmission)
        self._set_data_entry(data_builder.set_can_direct, table_index_model.can_direct)

        data = data_builder.build()

        # 2. Add elements from the options column
        state_gui_model = copy.deepcopy(self._state_gui_model)
        options_column_model = table_index_model.options_column_model
        self._apply_column_options_to_state(options_column_model, state_gui_model)

        # 3. Add other columns
        output_name = table_index_model.output_name
        if output_name:
            state_gui_model.output_name = output_name

        # 4. Create the rest of the state based on the builder.
        user_file_state_director = StateDirectorISIS(data)
        settings = copy.deepcopy(state_gui_model.settings)
        user_file_state_director.add_state_settings(settings)

        return user_file_state_director.construct()

    @staticmethod
    def _set_data_entry(func, entry):
        if entry:
            func(entry)

    @staticmethod
    def _apply_column_options_to_state(options_column_model, state_gui_model):
        """
        Apply the column setting of the user to the state for that particular row.

        Note if you are extending the options functionality, then you will have to add the property here.
        :param options_column_model: the option column model with the row specific settings
        :param state_gui_model: the state gui model
        """
        options = options_column_model.get_options()

        # Here we apply the correction to the state depending on the settings in the options. This is not very nice,
        # but currently it is not clear how to solve this differently.
        if "WavelengthMin" in options.keys():
            state_gui_model.wavelength_min = options["WavelengthMin"]

        if "WavelengthMax" in options.keys():
            state_gui_model.wavelength_max = options["WavelengthMax"]

        if "Thickness" in options.keys():
            state_gui_model.sample_thickness = options["Thickness"]
