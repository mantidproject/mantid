from __future__ import (absolute_import, division, print_function)
import os
from mantid.api import FileFinder
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.presenter.gui_state_director import (GuiStateDirector)
from sans.user_file.user_file_reader import UserFileReader
from mantid.kernel import Logger

sans_logger = Logger("SANS")


def create_states(state_model, table_model, instrument, facility, row_index=None, file_lookup=True):
    """
    Here we create the states based on the settings in the models
    :param state_model: the state model object
    :param table_model: the table model object
    :param row_index: the selected row, if None then all rows are generated
    """
    number_of_rows = table_model.get_number_of_rows()
    if row_index is not None:
        # Check if the selected index is valid
        if row_index >= number_of_rows:
            return None
        rows = [row_index]
    else:
        rows = range(number_of_rows)
    states = {}

    gui_state_director = GuiStateDirector(table_model, state_model, facility)
    for row in rows:
        sans_logger.information("Generating state for row {}".format(row))
        if not __is_empty_row(row, table_model):
            row_user_file = table_model.get_row_user_file(row)
            if row_user_file:
                row_state_model = create_gui_state_from_userfile(row_user_file, state_model)
                row_gui_state_director = GuiStateDirector(table_model, row_state_model, facility)
                state = __create_row_state(row_gui_state_director, row, instrument, file_lookup=file_lookup)
                states.update({row: state})
            else:
                state = __create_row_state(gui_state_director, row, instrument, file_lookup=file_lookup)
                states.update({row: state})
    return states


def __create_row_state(director, row, instrument, file_lookup=True):
    try:
        return director.create_state(row, instrument=instrument, file_lookup=file_lookup)
    except (ValueError, RuntimeError) as e:
        raise RuntimeError("There was a bad entry for row {}. {}".format(row, str(e)))


def __is_empty_row(row, table):
    for key, value in table._table_entries[row].__dict__.items():
        if value and key not in ['index', 'options_column_model', 'sample_thickness']:
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
