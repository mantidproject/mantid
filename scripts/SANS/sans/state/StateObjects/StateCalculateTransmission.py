# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the calculation of the transmission for SANS reduction."""

from __future__ import (absolute_import, division, print_function)
import json
import copy
import abc
from six import with_metaclass, itervalues, add_metaclass

from sans.state.JsonSerializable import JsonSerializable
from sans.common.enums import (RebinType, RangeStepType, FitType, DataType, SANSInstrument)
from sans.common.configurations import Configurations
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import (is_pure_none_or_not_none, validation_message,
                                        is_not_none_and_first_larger_than_second, one_is_none)
from sans.common.xml_parsing import get_named_elements_from_ipf_file


class StateTransmissionFit(with_metaclass(JsonSerializable)):

    def __init__(self):
        super(StateTransmissionFit, self).__init__()
        self.fit_type = FitType.LOGARITHMIC
        self.polynomial_order = 0  # : Int (Positive)
        self.wavelength_low = None  # : Float (Optional)
        self.wavelength_high = None  # : Float (Optional)

    def validate(self):
        is_invalid = {}
        if self.fit_type is FitType.POLYNOMIAL and self.polynomial_order == 0:
            entry = validation_message("You can only select a polynomial fit if you set a polynomial order (2 to 6).",
                                       "Make sure that you select a polynomial order.",
                                       {"fit_type": self.fit_type,
                                        "polynomial_order": self.polynomial_order})
            is_invalid.update(entry)

        if not is_pure_none_or_not_none([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Inconsistent wavelength setting.",
                                       "Make sure that you have specified both wavelength bounds (or none).",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)
        if is_invalid:
            raise ValueError("StateTransmissionFit: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


class StateCalculateTransmission(with_metaclass(JsonSerializable)):
    def __init__(self):
        super(StateCalculateTransmission, self).__init__()
        # -----------------------
        # Transmission
        # -----------------------
        self.transmission_radius_on_detector = None  # : Float (Positive)
        self.transmission_roi_files = None  # : List[Str]
        self.transmission_mask_files = None  # : List[Str]

        self.default_transmission_monitor = None  # : Int (Positive)
        self.transmission_monitor = None  # : Int (Positive)

        self.default_incident_monitor = None  # : Int (Positive)
        self.incident_monitor = None  # : Int (Positive)

        # ----------------------
        # Prompt peak correction
        # ----------------------
        self.prompt_peak_correction_min = None  # : Float (Positive)
        self.prompt_peak_correction_max = None  # : Float (Positive)
        self.prompt_peak_correction_enabled = False  # : Bool

        # ----------------
        # Wavelength rebin
        # ----------------
        self.wavelength_low = None  # : List[Float] (Positive)
        self.wavelength_high = None  # : List[Float] (Positive)
        self.wavelength_step = None  # : Float (Positive)
        self.rebin_type = RebinType.REBIN
        self.wavelength_step_type = RangeStepType.NOT_SET

        self.use_full_wavelength_range = False  # : Bool
        self.wavelength_full_range_low = None  # : Float (Positive)
        self.wavelength_full_range_high = None  # : Float (Positive)

        # -----------------------
        # Background correction
        # ----------------------
        self.background_TOF_general_start = None  # : Float
        self.background_TOF_general_stop = None  # : Float
        self.background_TOF_monitor_start = {}  # : Dict
        self.background_TOF_monitor_stop = {}  # : Dict
        self.background_TOF_roi_start = None  # : Float
        self.background_TOF_roi_stop = None  # : Float

        self.fit = {DataType.CAN.value: StateTransmissionFit(),
                    DataType.SAMPLE.value: StateTransmissionFit()}

    def validate(self):  # noqa
        is_invalid = {}
        # -----------------
        # Incident monitor
        # -----------------
        if self.incident_monitor is None and self.default_incident_monitor is None:
            entry = validation_message("No incident monitor was specified.",
                                       "Make sure that incident monitor has been specified.",
                                       {"incident_monitor": self.incident_monitor,
                                        "default_incident_monitor": self.default_incident_monitor})
            is_invalid.update(entry)

        # --------------
        # Transmission, either we need some ROI (ie radius, roi files /mask files) or a transmission monitor
        # --------------
        has_no_transmission_monitor_setting = self.transmission_monitor is None and \
                                              self.default_transmission_monitor is None  # noqa
        has_no_transmission_roi_setting = self.transmission_radius_on_detector is None and \
                                          self.transmission_roi_files is None  # noqa
        if has_no_transmission_monitor_setting and has_no_transmission_roi_setting:
            entry = validation_message("No transmission settings were specified.",
                                       "Make sure that transmission settings are specified.",
                                       {"transmission_monitor": self.transmission_monitor,
                                        "default_transmission_monitor": self.default_transmission_monitor,
                                        "transmission_radius_on_detector": self.transmission_radius_on_detector,
                                        "transmission_roi_files": self.transmission_roi_files})
            is_invalid.update(entry)

        # -----------------
        # Prompt peak
        # -----------------
        if not is_pure_none_or_not_none([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message("Inconsistent prompt peak setting.",
                                       "Make sure that you have specified both prompt peak bounds (or none).",
                                       {"prompt_peak_correction_min": self.prompt_peak_correction_min,
                                        "prompt_peak_correction_max": self.prompt_peak_correction_max})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message("Incorrect prompt peak bounds.",
                                       "Make sure that lower prompt peak bound is smaller then upper bound.",
                                       {"prompt_peak_correction_min": self.prompt_peak_correction_min,
                                        "prompt_peak_correction_max": self.prompt_peak_correction_max})
            is_invalid.update(entry)

        # -----------------
        # Wavelength rebin
        # -----------------
        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step, self.rebin_type]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step,
                                        "wavelength_step_type": self.wavelength_step_type,
                                        "rebin_type": self.rebin_type})
            is_invalid.update(entry)

        if self.wavelength_step_type is RangeStepType.NOT_SET:
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_step_type": self.wavelength_step_type})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)

        if self.use_full_wavelength_range:
            if self.wavelength_full_range_low is None or self.wavelength_full_range_high is None:
                entry = validation_message("Incorrect full wavelength settings.",
                                           "Make sure that both full wavelength entries have been set.",
                                           {"wavelength_full_range_low": self.wavelength_full_range_low,
                                            "wavelength_full_range_high": self.wavelength_full_range_high})
                is_invalid.update(entry)
            if is_not_none_and_first_larger_than_second([self.wavelength_full_range_low,
                                                         self.wavelength_full_range_high]):
                entry = validation_message("Incorrect wavelength bounds.",
                                           "Make sure that lower full wavelength bound is smaller then upper bound.",
                                           {"wavelength_full_range_low": self.wavelength_full_range_low,
                                            "wavelength_full_range_high": self.wavelength_full_range_high})
                is_invalid.update(entry)

        # ----------------------
        # Background correction
        # ----------------------
        if not is_pure_none_or_not_none([self.background_TOF_general_start, self.background_TOF_general_stop]):
            entry = validation_message("A general background TOF entry has not been set.",
                                       "Make sure that either all general background TOF entries are set or none.",
                                       {"background_TOF_general_start": self.background_TOF_general_start,
                                        "background_TOF_general_stop": self.background_TOF_general_stop})
            is_invalid.update(entry)
        if is_not_none_and_first_larger_than_second([self.background_TOF_general_start,
                                                     self.background_TOF_general_stop]):
            entry = validation_message("Incorrect general background TOF bounds.",
                                       "Make sure that lower general background TOF bound is smaller then upper bound.",
                                       {"background_TOF_general_start": self.background_TOF_general_start,
                                        "background_TOF_general_stop": self.background_TOF_general_stop})
            is_invalid.update(entry)

        if not is_pure_none_or_not_none([self.background_TOF_roi_start, self.background_TOF_roi_stop]):
            entry = validation_message("A ROI background TOF entry has not been set.",
                                       "Make sure that either all ROI background TOF entries are set or none.",
                                       {"background_TOF_roi_start": self.background_TOF_roi_start,
                                        "background_TOF_roi_stop": self.background_TOF_roi_stop})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.background_TOF_roi_start,
                                                     self.background_TOF_roi_stop]):
            entry = validation_message("Incorrect ROI background TOF bounds.",
                                       "Make sure that lower ROI background TOF bound is smaller then upper bound.",
                                       {"background_TOF_roi_start": self.background_TOF_roi_start,
                                        "background_TOF_roi_stop": self.background_TOF_roi_stop})
            is_invalid.update(entry)

        if not is_pure_none_or_not_none([self.background_TOF_monitor_start, self.background_TOF_monitor_stop]):
            entry = validation_message("A monitor background TOF entry has not been set.",
                                       "Make sure that either all monitor background TOF entries are set or none.",
                                       {"background_TOF_monitor_start": self.background_TOF_monitor_start,
                                        "background_TOF_monitor_stop": self.background_TOF_monitor_stop})
            is_invalid.update(entry)

        if self.background_TOF_monitor_start is not None and self.background_TOF_monitor_stop is not None:
            if len(self.background_TOF_monitor_start) != len(self.background_TOF_monitor_stop):
                entry = validation_message("The monitor background TOF entries have a length mismatch.",
                                           "Make sure that all monitor background TOF entries have the same length.",
                                           {"background_TOF_monitor_start": self.background_TOF_monitor_start,
                                            "background_TOF_monitor_stop": self.background_TOF_monitor_stop})
                is_invalid.update(entry)
            for key_start, value_start in list(self.background_TOF_monitor_start.items()):
                if key_start not in self.background_TOF_monitor_stop:
                    entry = validation_message("The monitor background TOF had spectrum number mismatch.",
                                               "Make sure that all monitors have entries for start and stop.",
                                               {"background_TOF_monitor_start": self.background_TOF_monitor_start,
                                                "background_TOF_monitor_stop": self.background_TOF_monitor_stop})
                    is_invalid.update(entry)
                else:
                    value_stop = self.background_TOF_monitor_stop[key_start]
                    if value_start > value_stop:
                        entry = validation_message("Incorrect monitor background TOF bounds.",
                                                   "Make sure that lower monitor background TOF bound is"
                                                   " smaller then upper bound.",
                                                   {"background_TOF_monitor_start": self.background_TOF_monitor_start,
                                                    "background_TOF_monitor_stop": self.background_TOF_monitor_stop})
                        is_invalid.update(entry)

        for fit_type in itervalues(self.fit):
            fit_type.validate()

        if is_invalid:
            raise ValueError("StateCalculateTransmission: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


class StateCalculateTransmissionLOQ(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionLOQ, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.LOQ.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.LOQ.wavelength_full_range_high

        # Set the LOQ default range for prompt peak correction
        self.prompt_peak_correction_min = Configurations.LOQ.prompt_peak_correction_min
        self.prompt_peak_correction_max = Configurations.LOQ.prompt_peak_correction_max
        self.prompt_peak_correction_enabled = True

    def validate(self):
        super(StateCalculateTransmissionLOQ, self).validate()


class StateCalculateTransmissionSANS2D(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionSANS2D, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.SANS2D.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.SANS2D.wavelength_full_range_high

    def validate(self):
        super(StateCalculateTransmissionSANS2D, self).validate()


class StateCalculateTransmissionLARMOR(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionLARMOR, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.LARMOR.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.LARMOR.wavelength_full_range_high

    def validate(self):
        super(StateCalculateTransmissionLARMOR, self).validate()


class StateCalculateTransmissionZOOM(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionZOOM, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.ZOOM.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.ZOOM.wavelength_full_range_high

    def validate(self):
        super(StateCalculateTransmissionZOOM, self).validate()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_default_monitors(calculate_transmission_info, data_info):
    """
    The default incident monitor is stored on the IPF.
    :param calculate_transmission_info: a StateCalculateTransmission object on which we set the default value
    :param data_info: a StateData object
    """
    ipf_file_path = data_info.ipf_file_path
    incident_tag = "default-incident-monitor-spectrum"
    transmission_tag = "default-transmission-monitor-spectrum"
    monitors_to_search = [incident_tag, transmission_tag]
    found_monitor_spectrum = get_named_elements_from_ipf_file(ipf_file_path, monitors_to_search, int)
    if incident_tag in found_monitor_spectrum:
        calculate_transmission_info.default_incident_monitor = found_monitor_spectrum[incident_tag]
    if transmission_tag in found_monitor_spectrum:
        calculate_transmission_info.default_transmission_monitor = found_monitor_spectrum[transmission_tag]


# ---------------------------------------
# State builders
# ---------------------------------------
@add_metaclass(abc.ABCMeta)
class StateCalculateTransmissionBuilderCommon(object):
    def __init__(self, state):
        self.state = state

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val

    def set_rebin_type(self, val):
        self.state.rebin_type = val

    def set_can_fit_type(self, val):
        self.state.fit[DataType.CAN.value].fit_type = val

    def set_can_polynomial_order(self, val):
        self.state.fit[DataType.CAN.value].polynomial_order = val

    def set_can_wavelength_low(self, val):
        self.state.fit[DataType.CAN.value].wavelength_low = val

    def set_can_wavelength_high(self, val):
        self.state.fit[DataType.CAN.value].wavelength_high = val

    def set_sample_fit_type(self, val):
        self.state.fit[DataType.SAMPLE.value].fit_type = val

    def set_sample_polynomial_order(self, val):
        self.state.fit[DataType.SAMPLE.value].polynomial_order = val

    def set_sample_wavelength_low(self, val):
        self.state.fit[DataType.SAMPLE.value].wavelength_low = val

    def set_sample_wavelength_high(self, val):
        self.state.fit[DataType.SAMPLE.value].wavelength_high = val


class StateCalculateTransmissionBuilderLOQ(StateCalculateTransmissionBuilderCommon):
    @automatic_setters(StateCalculateTransmissionLOQ)
    def __init__(self, data_info):
        super(StateCalculateTransmissionBuilderLOQ, self).__init__(state=StateCalculateTransmissionLOQ())
        self._data = data_info
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class StateCalculateTransmissionBuilderSANS2D(StateCalculateTransmissionBuilderCommon):
    @automatic_setters(StateCalculateTransmissionSANS2D)
    def __init__(self, data_info):
        super(StateCalculateTransmissionBuilderSANS2D, self).__init__(state=StateCalculateTransmissionSANS2D())
        self._data = data_info
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class StateCalculateTransmissionBuilderLARMOR(StateCalculateTransmissionBuilderCommon):
    @automatic_setters(StateCalculateTransmissionLARMOR)
    def __init__(self, data_info):
        super(StateCalculateTransmissionBuilderLARMOR, self).__init__(state=StateCalculateTransmissionLARMOR())
        self._data = data_info
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


class StateCalculateTransmissionBuilderZOOM(StateCalculateTransmissionBuilderCommon):
    @automatic_setters(StateCalculateTransmissionZOOM)
    def __init__(self, data_info):
        super(StateCalculateTransmissionBuilderZOOM, self).__init__(state=StateCalculateTransmissionZOOM())
        self._data = data_info
        set_default_monitors(self.state, self._data)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for StateCalculateTransmissionBuilder
# ------------------------------------------
def get_calculate_transmission_builder(data_info):
    instrument = data_info.instrument

    if instrument is SANSInstrument.LARMOR:
        return StateCalculateTransmissionBuilderLARMOR(data_info)
    elif instrument is SANSInstrument.SANS2D:
        return StateCalculateTransmissionBuilderSANS2D(data_info)
    elif instrument is SANSInstrument.LOQ:
        return StateCalculateTransmissionBuilderLOQ(data_info)
    elif instrument is SANSInstrument.ZOOM:
        return StateCalculateTransmissionBuilderZOOM(data_info)
    else:
        raise NotImplementedError("StateCalculateTransmissionBuilder: Could not find any valid transmission "
                                  "builder for the specified StateData object {0}".format(str(data_info)))
