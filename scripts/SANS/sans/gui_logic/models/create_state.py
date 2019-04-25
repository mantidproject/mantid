# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
from mantid.api import FileFinder
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.presenter.gui_state_director import (GuiStateDirector)
from sans.user_file.user_file_reader import UserFileReader
from mantid.kernel import Logger
from sans.state.state import State

sans_logger = Logger("SANS")


def create_states(state_model, table_model, instrument, facility, row_index=None,
                  file_lookup=True, user_file=""):
    """
    Here we create the states based on the settings in the models
    :param state_model: the state model object
    :param table_model: the table model object
    :param row_index: the selected row, if None then all rows are generated
    :param user_file: the user file under which the data is reduced
    """
    number_of_rows = table_model.get_number_of_rows()
    rows = [x for x in row_index if x < number_of_rows]

    states = {}
    errors = {}

    gui_state_director = GuiStateDirector(table_model, state_model, facility)
    for row in rows:
        if file_lookup:
            table_model.wait_for_file_information(row)
        state = _create_row_state(row, table_model, state_model, facility, instrument, file_lookup,
                                  gui_state_director, user_file)
        if isinstance(state, State):
            states.update({row: state})
        elif isinstance(state, str):
            errors.update({row: state})
    return states, errors


def _create_row_state(row, table_model, state_model, facility, instrument, file_lookup,
                      gui_state_director, user_file):
    try:
        sans_logger.information("Generating state for row {}".format(row))
        state = None

        table_entry = table_model.get_table_entry(row)
        if not table_entry.file_information and file_lookup:
            error_message = "Trying to find the SANS file {0}, but cannot find it. Make sure that " \
                            "the relevant paths are added and the correct instrument is selected."
            raise RuntimeError(error_message.format(table_entry.sample_scatter))

        if not __is_empty_row(row, table_model):
            row_user_file = table_model.get_row_user_file(row)
            if row_user_file:
                row_state_model = create_gui_state_from_userfile(row_user_file, state_model)
                row_gui_state_director = GuiStateDirector(table_model, row_state_model, facility)
                state = row_gui_state_director.create_state(row, instrument=instrument, file_lookup=file_lookup,
                                                            user_file=row_user_file)
            else:
                state = gui_state_director.create_state(row, instrument=instrument, file_lookup=file_lookup,
                                                        user_file=user_file)
        return state
    except (ValueError, RuntimeError) as e:
        return "{}".format(str(e))


def __is_empty_row(row, table):
    for key, value in table._table_entries[row].__dict__.items():
        if value and key in ['sample_scatter']:
            return False
    return True


def create_gui_state_from_userfile(row_user_file, state_model):
    user_file_path = FileFinder.getFullPath(row_user_file)
    if not os.path.exists(user_file_path):
        raise RuntimeError("The user path {} does not exist. Make sure a valid user file path"
                           " has been specified.".format(user_file_path))

    user_file_reader = UserFileReader(user_file_path)
    user_file_items = user_file_reader.read_user_file()
    state_gui_model = StateGuiModel(user_file_items)
    state_gui_model.save_types = state_model.save_types
    return state_gui_model
