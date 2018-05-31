from __future__ import (absolute_import, division, print_function)
import os
from mantid.api import FileFinder
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.presenter.gui_state_director import (GuiStateDirector)
from sans.user_file.user_file_reader import UserFileReader
from mantid.kernel import Logger

sans_logger = Logger("SANS")

def create_states(state_model, table_model, instrument, number_of_rows, facility, row_index=None, file_lookup=True):
    """
    Here we create the states based on the settings in the models
    :param state_model: the state model object
    :param table_model: the table model object
    :param row_index: the selected row, if None then all rows are generated
    """
    # number_of_rows = self._view.get_number_of_rows()
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
        if not is_empty_row(row, table_model):
            row_user_file = table_model.get_row_user_file(row)
            if row_user_file:
                user_file_path = FileFinder.getFullPath(row_user_file)
                if not os.path.exists(user_file_path):
                    raise RuntimeError("The user path {} does not exist. Make sure a valid user file path"
                                       " has been specified.".format(user_file_path))

                user_file_reader = UserFileReader(user_file_path)
                user_file_items = user_file_reader.read_user_file()

                row_state_model = StateGuiModel(user_file_items)
                row_gui_state_director = GuiStateDirector(table_model, row_state_model, facility)
                create_row_state(row_gui_state_director, states, row, instrument, file_lookup=file_lookup)
            else:
                create_row_state(gui_state_director, states, row, instrument, file_lookup=file_lookup)
    return states


def create_row_state(director, states, row, instrument, file_lookup=True):
    try:
        state = director.create_state(row, instrument=instrument, file_lookup=file_lookup)
        states.update({row: state})
    except (ValueError, RuntimeError) as e:
        raise RuntimeError("There was a bad entry for row {}. {}".format(row, str(e)))

def is_empty_row(row, table):
    for key, value in table._table_entries[row].__dict__.iteritems():
        if value:
            return False
    return True


def create_state_for_row(state_model_with_view_update, table_model, number_of_rows, instrument, facility, row_index, file_lookup=True):
    if table_model and state_model_with_view_update:
        states = create_states(state_model_with_view_update, table_model, instrument, number_of_rows, facility,
                                     row_index, file_lookup=file_lookup)
    else:
        sans_logger.warning("There does not seem to be data for a row {}.".format(row_index))
        return None

    if row_index in list(states.keys()):
        if states:
            return states[row_index]
    return None