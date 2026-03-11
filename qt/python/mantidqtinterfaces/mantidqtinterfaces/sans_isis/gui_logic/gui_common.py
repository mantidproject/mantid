# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from qtpy.QtCore import QSettings
from qtpy.QtWidgets import QFileDialog

from sans.common.constant_containers import (
    SANSInstrument_enum_as_key,
    SANSInstrument_string_as_key_NoInstrument,
    SANSInstrument_string_list,
)
from sans.common.enums import ReductionMode, DetectorType, SANSInstrument


# ----------------------------------------------------------------------------------------------------------------------
# Option column globals
# ----------------------------------------------------------------------------------------------------------------------
def generate_table_index(multi_period):
    table_index = {}
    table_index.update({"SAMPLE_SCATTER_INDEX": 0})
    table_index.update({"SAMPLE_SCATTER_PERIOD_INDEX": 1 if multi_period else None})
    table_index.update({"SAMPLE_TRANSMISSION_INDEX": 2 if multi_period else 1})
    table_index.update({"SAMPLE_TRANSMISSION_PERIOD_INDEX": 3 if multi_period else None})
    table_index.update({"SAMPLE_DIRECT_INDEX": 4 if multi_period else 2})
    table_index.update({"SAMPLE_DIRECT_PERIOD_INDEX": 5 if multi_period else None})
    table_index.update({"CAN_SCATTER_INDEX": 6 if multi_period else 3})
    table_index.update({"CAN_SCATTER_PERIOD_INDEX": 7 if multi_period else None})
    table_index.update({"CAN_TRANSMISSION_INDEX": 8 if multi_period else 4})
    table_index.update({"CAN_TRANSMISSION_PERIOD_INDEX": 9 if multi_period else None})
    table_index.update({"CAN_DIRECT_INDEX": 10 if multi_period else 5})
    table_index.update({"CAN_DIRECT_PERIOD_INDEX": 11 if multi_period else None})
    table_index.update({"OUTPUT_NAME_INDEX": 12 if multi_period else 6})
    table_index.update({"USER_FILE_INDEX": 13 if multi_period else 7})
    table_index.update({"SAMPLE_THICKNESS_INDEX": 14 if multi_period else 8})
    table_index.update({"OPTIONS_INDEX": 15 if multi_period else 9})
    table_index.update({"HIDDEN_OPTIONS_INDEX": 16 if multi_period else 10})
    return table_index


OPTIONS_SEPARATOR = ","
OPTIONS_EQUAL = "="

# ----------------------------------------------------------------------------------------------------------------------
#  Other Globals
# ----------------------------------------------------------------------------------------------------------------------
LAB_STRINGS = {
    SANSInstrument.SANS2D: "rear",
    SANSInstrument.LOQ: "main-detector",
    SANSInstrument.LARMOR: "DetectorBench",
    SANSInstrument.ZOOM: "rear-detector",
    SANSInstrument.NO_INSTRUMENT: ReductionMode.LAB.value,
}

HAB_STRINGS = {SANSInstrument.SANS2D: "front", SANSInstrument.LOQ: "Hab", SANSInstrument.NO_INSTRUMENT: ReductionMode.HAB.value}

ALL = ReductionMode.ALL.value
DEFAULT_HAB = ReductionMode.HAB.value
MERGED = ReductionMode.MERGED.value

GENERIC_SETTINGS = "Mantid/ISISSANS"

JSON_SUFFIX = ".json"
# The following instruments do not scale for specific attributes
SCALING_EXCLUDED = [SANSInstrument.LARMOR]


def apply_selective_view_scaling(getter):
    """
    Scales
    """

    def wrapper(self):
        val = getter(self)
        try:
            return meter_2_millimeter(val) if self.instrument not in SCALING_EXCLUDED else val
        except TypeError:
            return val

    return wrapper


def undo_selective_view_scaling(setter):
    def wrapper(self, val):
        try:
            val = millimeter_2_meter(val) if self.instrument not in SCALING_EXCLUDED else val
        except TypeError:
            pass
        finally:
            setter(self, val)  # Always take user val including blank string

    return wrapper


def meter_2_millimeter(num):
    """
    Converts from m to mm
    @param float in m
    @returns float in mm
    """
    return num * 1000.0


def millimeter_2_meter(num):
    """
    Converts from mm to m
    @param float in mm
    @returns float in m
    """
    return num / 1000.0


def get_detector_strings_for_gui(instrument=None):
    if instrument is SANSInstrument.SANS2D or instrument is SANSInstrument.LOQ:
        return [LAB_STRINGS[instrument], HAB_STRINGS[instrument]]
    # Larmor and Zoom only need LAB
    elif instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.ZOOM:
        return [LAB_STRINGS[instrument]]

    else:
        return [LAB_STRINGS[SANSInstrument.NO_INSTRUMENT], HAB_STRINGS[SANSInstrument.NO_INSTRUMENT]]


def get_detector_strings_for_diagnostic_page(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [LAB_STRINGS[instrument], HAB_STRINGS[instrument]]

    elif any(instrument is x for x in [SANSInstrument.LOQ, SANSInstrument.LARMOR, SANSInstrument.ZOOM]):
        return [LAB_STRINGS[instrument]]

    else:
        return [LAB_STRINGS[SANSInstrument.NO_INSTRUMENT], HAB_STRINGS[SANSInstrument.NO_INSTRUMENT]]


def get_reduction_mode_strings_for_gui(instrument=None):
    if instrument is SANSInstrument.SANS2D:
        return [LAB_STRINGS[instrument], HAB_STRINGS[instrument], MERGED, ALL]

    elif instrument is SANSInstrument.LOQ:
        return [LAB_STRINGS[instrument], HAB_STRINGS[instrument], MERGED, ALL]

    elif any(instrument is x for x in [SANSInstrument.LARMOR, SANSInstrument.ZOOM]):
        return [LAB_STRINGS[instrument]]

    else:
        return [LAB_STRINGS[SANSInstrument.NO_INSTRUMENT], HAB_STRINGS[SANSInstrument.NO_INSTRUMENT], MERGED, ALL]


def get_instrument_strings_for_gui():
    return SANSInstrument_string_list


def get_reduction_selection(instrument):
    selection = {ReductionMode.MERGED: MERGED, ReductionMode.ALL: ALL}

    if any(instrument is x for x in [SANSInstrument.SANS2D, SANSInstrument.LOQ]):
        selection.update({ReductionMode.LAB: LAB_STRINGS[instrument], ReductionMode.HAB: HAB_STRINGS[instrument]})

    elif any(instrument is x for x in [SANSInstrument.LARMOR, SANSInstrument.ZOOM]):
        selection = {ReductionMode.LAB: LAB_STRINGS[instrument]}

    else:
        selection.update(
            {ReductionMode.LAB: LAB_STRINGS[SANSInstrument.NO_INSTRUMENT], ReductionMode.HAB: HAB_STRINGS[SANSInstrument.NO_INSTRUMENT]}
        )
    return selection


def get_string_for_gui_from_reduction_mode(reduction_mode, instrument):
    reduction_selection = get_reduction_selection(instrument)
    if reduction_selection and reduction_mode in reduction_selection:
        return reduction_selection[reduction_mode]
    else:
        return None


def get_string_for_gui_from_instrument(instrument):
    try:
        return SANSInstrument_enum_as_key[instrument]
    except KeyError:
        return None


def get_reduction_mode_from_gui_selection(gui_selection: str):
    case_folded_selection = gui_selection.strip().casefold()
    if case_folded_selection == MERGED.casefold():
        return ReductionMode.MERGED
    elif case_folded_selection == ALL.casefold():
        return ReductionMode.ALL
    elif any(case_folded_selection == lab.casefold() for lab in LAB_STRINGS.values()):
        return ReductionMode.LAB
    elif any(case_folded_selection == hab.casefold() for hab in HAB_STRINGS.values()):
        return ReductionMode.HAB
    elif not case_folded_selection:
        return ReductionMode.NOT_SET
    else:
        raise RuntimeError("Reduction mode selection {0} is not valid.".format(gui_selection))


def get_detector_from_gui_selection(gui_selection):
    case_folded_selection = gui_selection.casefold()

    if any(case_folded_selection == hab.casefold() for hab in HAB_STRINGS.values()):
        return DetectorType.HAB
    else:
        return DetectorType.LAB


def get_instrument_from_gui_selection(gui_selection):
    try:
        return SANSInstrument_string_as_key_NoInstrument[gui_selection]
    except KeyError:
        raise RuntimeError("Instrument selection is not valid.")


def load_file(line_edit_field, filter_for_dialog, q_settings_group_key, q_settings_key, func_to_get_line_edit_val):
    # Get the last location of the user file
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    last_path = settings.value(q_settings_key, "", type=str)
    settings.endGroup()

    # Open the dialog
    open_file_dialog(line_edit_field, filter_for_dialog, last_path)

    # Save the new location
    new_path = func_to_get_line_edit_val()
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
    default = False if type is bool else ""
    default_property = settings.value(q_settings_key, default, type=type)
    settings.endGroup()

    return default_property


def set_setting(q_settings_group_key, q_settings_key, value):
    settings = QSettings()
    settings.beginGroup(q_settings_group_key)
    settings.setValue(q_settings_key, value)
    settings.endGroup()


def open_file_dialog(line_edit, filter_text, directory):
    file_name = QFileDialog.getOpenFileName(None, "Open", directory, filter_text)
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

    def set_setting(self, q_settings_key, value):
        settings = QSettings()
        settings.beginGroup(self.__generic_settings)
        settings.setValue(q_settings_key, value)
        settings.endGroup()
