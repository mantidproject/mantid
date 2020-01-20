# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import logger

from sans.common.enums import (DetectorType)
from sans.common.file_information import find_full_file_path
from sans.state.automatic_setters import set_up_setter_forwarding_from_director_to_builder
from sans.user_file.user_file_reader import UserFileReader
from sans.user_file.settings_tags import (TubeCalibrationFileId)

from sans.state.AllStates import get_all_states_builder
from sans.state.StateObjects.StateMaskDetectors import get_mask_builder
from sans.state.StateObjects.StateMoveDetectors import get_move_builder
from sans.state.StateObjects.StateReductionMode import get_reduction_mode_builder
from sans.state.StateObjects.StateSliceEvent import get_slice_event_builder
from sans.state.StateObjects.StateWavelength import get_wavelength_builder
from sans.state.StateObjects.StateSave import get_save_builder
from sans.state.StateObjects.StateScale import get_scale_builder
from sans.state.StateObjects.StateAdjustment import get_adjustment_builder
from sans.state.StateObjects.StateNormalizeToMonitor import get_normalize_to_monitor_builder
from sans.state.StateObjects.StateCalculateTransmission import get_calculate_transmission_builder
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import get_wavelength_and_pixel_adjustment_builder
from sans.state.StateObjects.StateConvertToQ import get_convert_to_q_builder
from sans.state.StateObjects.StateCompatibility import get_compatibility_builder


def check_if_contains_only_one_element(to_check, element_name):
    if len(to_check) > 1:
        msg = "The element {0} contains more than one element. Expected only one element. " \
              "The last element {1} is used. The elements {2} are discarded.".format(element_name,
                                                                                     to_check[-1], to_check[:-1])
        logger.information(msg)


def log_non_existing_field(field):
    msg = "The field {0} does not seem to exist on the state.".format(field)
    logger.information(msg)


def convert_detector(detector_type):
    if detector_type is DetectorType.HAB:
        detector_type_as_string = DetectorType.HAB.value
    elif detector_type is DetectorType.LAB:
        detector_type_as_string = DetectorType.LAB.value
    else:
        raise RuntimeError("UserFileStateDirector: Cannot convert detector {0}".format(detector_type))
    return detector_type_as_string












class StateDirectorISIS(object):
    def __init__(self, data_info, file_information):
        super(StateDirectorISIS, self).__init__()
        data_info.validate()
        self._data = data_info

        self._user_file = None

        self._state_builder = get_all_states_builder(self._data)
        self._mask_builder = get_mask_builder(self._data)
        self._move_builder = get_move_builder(self._data)
        self._reduction_builder = get_reduction_mode_builder(self._data)
        self._slice_event_builder = get_slice_event_builder(self._data)
        self._wavelength_builder = get_wavelength_builder(self._data)
        self._save_builder = get_save_builder(self._data)
        self._scale_builder = get_scale_builder(self._data, file_information)

        self._adjustment_builder = get_adjustment_builder(self._data)
        self._normalize_to_monitor_builder = get_normalize_to_monitor_builder(self._data)
        self._calculate_transmission_builder = get_calculate_transmission_builder(self._data)
        self._wavelength_and_pixel_adjustment_builder = get_wavelength_and_pixel_adjustment_builder(self._data)

        self._convert_to_q_builder = get_convert_to_q_builder(self._data)

        self._compatibility_builder = get_compatibility_builder(self._data)

        # Now that we have setup all builders in the director we want to also allow for manual setting
        # of some components. In order to get the desired results we need to perform setter forwarding, e.g
        # self._scale_builder has the setter set_width, then the director should have a method called
        # set_scale_width whose input is forwarded to the actual builder. We can only set this retroactively
        # via monkey-patching.
        self._set_up_setter_forwarding()

    def _set_up_setter_forwarding(self):
        set_up_setter_forwarding_from_director_to_builder(self, "_state_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_mask_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_move_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_reduction_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_slice_event_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_wavelength_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_save_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_scale_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_adjustment_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_normalize_to_monitor_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_calculate_transmission_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_wavelength_and_pixel_adjustment_builder")
        set_up_setter_forwarding_from_director_to_builder(self, "_convert_to_q_builder")

        set_up_setter_forwarding_from_director_to_builder(self, "_compatibility_builder")

    def set_user_file(self, user_file):
        file_path = find_full_file_path(user_file)
        if file_path is None:
            raise RuntimeError("UserFileStateDirector: The specified user file cannot be found. Make sure that the "
                               "directory which contains the user file is added to the Mantid path.")
        self._user_file = file_path
        reader = UserFileReader(self._user_file)
        user_file_items = reader.read_user_file()
        self.add_state_settings(user_file_items)

    def add_state_settings(self, user_file_items):
        """
        This allows for a usage of the UserFileStateDirector with externally provided user_file_items or internally
        via the set_user_file method.

        :param user_file_items: a list of parsed user file items.
        """
        # ----------------------------------------------------
        # Populate the different sub states from the user file
        # ----------------------------------------------------
        # Data state
        self._add_information_to_data_state(user_file_items)

        # Mask state
        self._set_up_mask_state(user_file_items)

        # Reduction state
        self._set_up_reduction_state(user_file_items)

        # Move state
        self._set_up_move_state(user_file_items)

        # Wavelength state
        self._set_up_wavelength_state(user_file_items)

        # Slice event state
        self._set_up_slice_event_state(user_file_items)
        # There does not seem to be a command for this currently -- this should be added in the future

        # Scale state
        self._set_up_scale_state(user_file_items)

        # Adjustment state and its substates
        self._set_up_adjustment_state(user_file_items)
        self._set_up_normalize_to_monitor_state(user_file_items)
        self._set_up_calculate_transmission(user_file_items)
        self._set_up_wavelength_and_pixel_adjustment(user_file_items)

        # Convert to Q state
        self._set_up_convert_to_q_state(user_file_items)

        # Compatibility state
        self._set_up_compatibility(user_file_items)

        # Save state
        self._set_up_save(user_file_items)

    def construct(self):
        # Create the different sub states and add them to the state
        # Mask state
        mask_state = self._mask_builder.build()
        mask_state.validate()
        self._state_builder.set_mask(mask_state)

        # Reduction state
        reduction_state = self._reduction_builder.build()
        reduction_state.validate()
        self._state_builder.set_reduction(reduction_state)

        # Move state
        move_state = self._move_builder.build()
        move_state.validate()
        self._state_builder.set_move(move_state)

        # Slice Event state
        slice_event_state = self._slice_event_builder.build()
        slice_event_state.validate()
        self._state_builder.set_slice(slice_event_state)

        # Wavelength conversion state
        wavelength_state = self._wavelength_builder.build()
        wavelength_state.validate()
        self._state_builder.set_wavelength(wavelength_state)

        # Save state
        save_state = self._save_builder.build()
        save_state.validate()
        self._state_builder.set_save(save_state)

        # Scale state
        scale_state = self._scale_builder.build()
        scale_state.validate()
        self._state_builder.set_scale(scale_state)

        # Adjustment state with the sub states
        normalize_to_monitor_state = self._normalize_to_monitor_builder.build()
        self._adjustment_builder.set_normalize_to_monitor(normalize_to_monitor_state)

        calculate_transmission_state = self._calculate_transmission_builder.build()
        self._adjustment_builder.set_calculate_transmission(calculate_transmission_state)

        wavelength_and_pixel_adjustment_state = self._wavelength_and_pixel_adjustment_builder.build()
        self._adjustment_builder.set_wavelength_and_pixel_adjustment(wavelength_and_pixel_adjustment_state)

        adjustment_state = self._adjustment_builder.build()
        adjustment_state.validate()

        self._state_builder.set_adjustment(adjustment_state)

        # Convert to Q state
        convert_to_q_state = self._convert_to_q_builder.build()
        convert_to_q_state.validate()
        self._state_builder.set_convert_to_q(convert_to_q_state)

        # Compatibility state
        compatibility_state = self._compatibility_builder.build()
        compatibility_state.validate()
        self._state_builder.set_compatibility(compatibility_state)

        # Data state
        self._state_builder.set_data(self._data)

        return self._state_builder.build()

    def _add_information_to_data_state(self, user_file_items):
        # The only thing that should be set on the data is the tube calibration file which is specified in
        # the user file.
        if TubeCalibrationFileId.FILE in user_file_items:
            tube_calibration = user_file_items[TubeCalibrationFileId.FILE]
            check_if_contains_only_one_element(tube_calibration, TubeCalibrationFileId.FILE)
            tube_calibration = tube_calibration[-1]
            self._data.calibration = tube_calibration

    def convert_pos1(self, pos1):
        """
        Performs a conversion of position 1 of the beam centre. This is forwarded to the move builder.

        :param pos1: the first position (this can be x in mm or for LARMOR and angle)
        :return: the correctly scaled position
        """
        return self._move_builder.convert_pos1(pos1)

    def convert_pos2(self, pos2):
        """
        Performs a conversion of position 2 of the beam centre. This is forwarded to the move builder.

        :param pos2: the second position
        :return: the correctly scaled position
        """
        return self._move_builder.convert_pos2(pos2)
