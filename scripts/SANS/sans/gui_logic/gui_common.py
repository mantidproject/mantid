# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os

from qtpy.QtCore import (QSettings)  # noqa
from qtpy.QtWidgets import (QFileDialog)  # noqa

from mantid.kernel import Logger
from sans.common.enums import SANSInstrument, ISISReductionMode, DetectorType


# ----------------------------------------------------------------------------------------------------------------------
# Option column globals
# ----------------------------------------------------------------------------------------------------------------------
def generate_table_index(multi_period):
    table_index = {}
    table_index.update({'SAMPLE_SCATTER_INDEX': 0})
    table_index.update({'SAMPLE_SCATTER_PERIOD_INDEX': 1 if multi_period else None})
    table_index.update({'SAMPLE_TRANSMISSION_INDEX': 2 if multi_period else 1})
    table_index.update({'SAMPLE_TRANSMISSION_PERIOD_INDEX': 3 if multi_period else None})
    table_index.update({'SAMPLE_DIRECT_INDEX': 4 if multi_period else 2})
    table_index.update({'SAMPLE_DIRECT_PERIOD_INDEX': 5 if multi_period else None})
    table_index.update({'CAN_SCATTER_INDEX': 6 if multi_period else 3})
    table_index.update({'CAN_SCATTER_PERIOD_INDEX': 7 if multi_period else None})
    table_index.update({'CAN_TRANSMISSION_INDEX': 8 if multi_period else 4})
    table_index.update({'CAN_TRANSMISSION_PERIOD_INDEX': 9 if multi_period else None})
    table_index.update({'CAN_DIRECT_INDEX': 10 if multi_period else 5})
    table_index.update({'CAN_DIRECT_PERIOD_INDEX': 11 if multi_period else None})
    table_index.update({'OUTPUT_NAME_INDEX': 12 if multi_period else 6})
    table_index.update({'USER_FILE_INDEX': 13 if multi_period else 7})
    table_index.update({'SAMPLE_THICKNESS_INDEX': 14 if multi_period else 8})
    table_index.update({'OPTIONS_INDEX': 15 if multi_period else 9})
    table_index.update({'HIDDEN_OPTIONS_INDEX': 16 if multi_period else 10})
    return table_index


OPTIONS_SEPARATOR = ","
OPTIONS_EQUAL = "="

# ----------------------------------------------------------------------------------------------------------------------
#  Other Globals
# ----------------------------------------------------------------------------------------------------------------------
SANS2D_LAB = "rear"
SANS2D_HAB = "front"

LOQ_LAB = "main-detector"
LOQ_HAB = "Hab"

LARMOR_LAB = "DetectorBench"

ZOOM_LAB = "rear-detector"

DEFAULT_LAB = ISISReductionMode.to_string(ISISReductionMode.LAB)
DEFAULT_HAB = ISISReductionMode.to_string(ISISReductionMode.HAB)
MERGED = "Merged"
ALL = "All"

GENERIC_SETTINGS = "Mantid/ISISSANS"

JSON_SUFFIX = ".json"


def get_detector_strings_for_gui(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [SANS2D_LAB, SANS2D_HAB]
    elif instrument is SANSInstrument.LOQ:
        return [LOQ_LAB, LOQ_HAB]
    elif instrument is SANSInstrument.LARMOR:
        return [LARMOR_LAB]
    elif instrument is SANSInstrument.ZOOM:
        return [ZOOM_LAB]
    else:
        return [DEFAULT_LAB, DEFAULT_HAB]


def get_detector_strings_for_diagnostic_page(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [SANS2D_LAB, SANS2D_HAB]
    elif instrument is SANSInstrument.LOQ:
        return [LOQ_LAB]
    elif instrument is SANSInstrument.LARMOR:
        return [LARMOR_LAB]
    elif instrument is SANSInstrument.ZOOM:
        return [ZOOM_LAB]
    else:
        return [DEFAULT_LAB, DEFAULT_HAB]


def get_reduction_mode_strings_for_gui(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [SANS2D_LAB, SANS2D_HAB, MERGED, ALL]
    elif instrument is SANSInstrument.LOQ:
        return [LOQ_LAB, LOQ_HAB, MERGED, ALL]
    elif instrument is SANSInstrument.LARMOR:
        return [LARMOR_LAB]
    elif instrument is SANSInstrument.ZOOM:
        return [ZOOM_LAB]
    else:
        return [DEFAULT_LAB, DEFAULT_HAB, MERGED, ALL]


def get_instrument_strings_for_gui():
        return ['SANS2D', 'LOQ', 'LARMOR', 'ZOOM']


def get_reduction_selection(instrument):
    selection = {ISISReductionMode.Merged: MERGED,
                 ISISReductionMode.All: ALL}
    if instrument is SANSInstrument.SANS2D:
        selection.update({ISISReductionMode.LAB: SANS2D_LAB,
                          ISISReductionMode.HAB: SANS2D_HAB})
    elif instrument is SANSInstrument.LOQ:
        selection.update({ISISReductionMode.LAB: LOQ_LAB,
                          ISISReductionMode.HAB: LOQ_HAB})
    elif instrument is SANSInstrument.LARMOR:
        selection = {ISISReductionMode.LAB: LARMOR_LAB}
    elif instrument is SANSInstrument.ZOOM:
        selection = {ISISReductionMode.LAB: ZOOM_LAB}
    else:
        selection.update({ISISReductionMode.LAB: DEFAULT_LAB,
                          ISISReductionMode.HAB: DEFAULT_HAB})
    return selection


def get_string_for_gui_from_reduction_mode(reduction_mode, instrument):
    reduction_selection = get_reduction_selection(instrument)
    if reduction_selection and reduction_mode in reduction_selection:
        return reduction_selection[reduction_mode]
    else:
        return None


def get_string_for_gui_from_instrument(instrument):
    instrument_selection = {SANSInstrument.SANS2D: 'SANS2D', SANSInstrument.LOQ: 'LOQ', SANSInstrument.LARMOR: 'LARMOR'
                            , SANSInstrument.ZOOM: 'ZOOM'}
    if instrument in instrument_selection:
        return instrument_selection[instrument]
    else:
        return None


def get_reduction_mode_from_gui_selection(gui_selection):
    if gui_selection == MERGED:
        return ISISReductionMode.Merged
    elif gui_selection == ALL:
        return ISISReductionMode.All
    elif gui_selection == SANS2D_LAB or gui_selection == LOQ_LAB or gui_selection == LARMOR_LAB or gui_selection == ZOOM_LAB or gui_selection == DEFAULT_LAB:  # noqa
        return ISISReductionMode.LAB
    elif gui_selection == SANS2D_HAB or gui_selection == LOQ_HAB:
        return ISISReductionMode.HAB
    else:
        raise RuntimeError("Reduction mode selection is not valid.")


def get_detector_from_gui_selection(gui_selection):
    if gui_selection == LOQ_HAB or gui_selection == SANS2D_HAB:
        return DetectorType.HAB
    else:
        return DetectorType.LAB


def get_instrument_from_gui_selection(gui_selection):
    if gui_selection == 'LOQ':
        return SANSInstrument.LOQ
    elif gui_selection == 'LARMOR':
        return SANSInstrument.LARMOR
    elif gui_selection == 'SANS2D':
        return SANSInstrument.SANS2D
    elif gui_selection == 'ZOOM':
        return SANSInstrument.ZOOM
    elif gui_selection == 'NoInstrument':
        return SANSInstrument.NoInstrument
    else:
        raise RuntimeError("Instrument selection is not valid.")


def load_file(line_edit_field, filter_for_dialog, q_settings_group_key, q_settings_key, func):
    # Get the last location of the user file
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    last_path = settings.value(q_settings_key, "", type=str)
    settings.endGroup()

    # Open the dialog
    open_file_dialog(line_edit_field, filter_for_dialog, last_path)

    # Save the new location
    new_path, _ = os.path.split(func())
    if new_path:
        set_setting(q_settings_group_key, q_settings_key, new_path)


def load_default_file(line_edit_field, q_settings_group_key, q_settings_key):
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    default_file = settings.value(q_settings_key, "", type=str)
    settings.endGroup()

    line_edit_field.setText(default_file)


def load_property(q_settings_group_key, q_settings_key, type=str):
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    default = False if type == bool else ""
    default_property = settings.value(q_settings_key, default, type=type)
    settings.endGroup()

    return default_property


def set_setting(q_settings_group_key, q_settings_key, value):
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    settings.setValue(q_settings_key, value)
    settings.endGroup()


def open_file_dialog(line_edit, filter_text, directory):
    file_name = QFileDialog.getOpenFileName(None, 'Open', directory, filter_text)
    if not file_name:
        return
    if isinstance(file_name, tuple):
        file_name = file_name[0]
    line_edit.setText(file_name)


def get_batch_file_dir_from_path(batch_file_path):
    path, file = os.path.split(batch_file_path)
    if path != "" and path[-1] != "/":
        # Make string inline with other ConfigService paths
        path += "/"
    return path


def add_dir_to_datasearch(batch_file_path, current_directories):
    batch_file_directory = get_batch_file_dir_from_path(batch_file_path)
    if batch_file_directory != "" and batch_file_directory not in current_directories:
        current_directories = ";".join([current_directories, batch_file_directory])
    return batch_file_directory, current_directories


def remove_dir_from_datasearch(batch_file_path, directories):
    new_dirs = ";".join([path for path in directories.split(";") if path != batch_file_path])
    return new_dirs


# ----------------------------------------------------------------------------------------------------------------------
# GUI Property Defaults
# ----------------------------------------------------------------------------------------------------------------------
class SANSGuiPropertiesHandler(object):
    """
    This class handles the setting and getting
    of SANSDataProcessorGUI default properties.
    """
    def __init__(self, keys, line_edits={}):
        """
        Initialise a properties handler for a particular pyqt view.
        :param keys: A dict where keys are q_settings_key and values are a tuple of (update_function, type)
                    where update function is method in the view to be called when loading a property and type
                    is the data type of the property
        :param line_edits: A dict where keys are the q_settings_key and values are a view's line edits to be updated
        """
        self.__generic_settings = GENERIC_SETTINGS

        self.keys = keys
        self.line_edits = line_edits

        self._set_out_defaults()

    def _set_out_defaults(self):
        for property_key, (load_func, property_type) in self.keys.items():
            args = []
            if property_key in self.line_edits:
                self._load_default_file(self.line_edits[property_key], self.__generic_settings, property_key)
            else:
                try:
                    property_value = self._load_property(self.__generic_settings, property_key)
                    args.append(property_value)
                except RuntimeError:
                    pass
            load_func(*args)

    def update_default(self, gui_property, value):
        if gui_property in self.keys:
            self._set_setting(self.__generic_settings, gui_property, value)
        else:
            Logger("SANSPropertyHandler").information("Trying to set property {} for which a default property "
                                                      "does not exist".format(gui_property))

    @staticmethod
    def _load_default_file(line_edit_field, q_settings_group_key, q_settings_key):
        settings = QSettings()
        settings.beginGroup(q_settings_group_key)
        default_file = settings.value(q_settings_key, "", type=str)
        settings.endGroup()

        line_edit_field.setText(default_file)

    @staticmethod
    def _load_property(q_settings_group_key, q_settings_key):
        settings = QSettings()
        settings.beginGroup(q_settings_group_key)
        default_property = settings.value(q_settings_key, "", type=str)
        settings.endGroup()

        return default_property

    @staticmethod
    def _set_setting(q_settings_group_key, q_settings_key, value):
        settings = QSettings()
        settings.beginGroup(q_settings_group_key)
        settings.setValue(q_settings_key, value)
        settings.endGroup()
