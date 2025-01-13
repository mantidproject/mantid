# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the normalization to the incident monitor for SANS reduction."""

import json
import copy

from sans.state.JsonSerializable import JsonSerializable
from sans.common.enums import SANSInstrument, RebinType
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import is_pure_none_or_not_none, is_not_none_and_first_larger_than_second, validation_message
from sans.common.xml_parsing import get_named_elements_from_ipf_file


class StateNormalizeToMonitor(metaclass=JsonSerializable):
    def __init__(self):
        super(StateNormalizeToMonitor, self).__init__()
        self.prompt_peak_correction_min = None  # : Float (Optional)
        self.prompt_peak_correction_max = None  # : Float (Optional)
        self.prompt_peak_correction_enabled = False  # : Bool

        self.rebin_type: RebinType = RebinType.REBIN

        self.background_TOF_general_start = None  # : Float
        self.background_TOF_general_stop = None  # : Float
        self.background_TOF_monitor_start = {}  # : Dict
        self.background_TOF_monitor_stop = {}  # : Dict

        self.incident_monitor = None  # : Int (Positive)

    def validate(self):
        is_invalid = {}
        # -----------------
        # incident Monitor
        # -----------------
        if self.incident_monitor is None:
            is_invalid.update({"incident_monitor": "An incident monitor must be specified."})

        # -----------------
        # Prompt peak
        # -----------------
        if not is_pure_none_or_not_none([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message(
                "A prompt peak correction entry has not been set.",
                "Make sure that either all prompt peak entries have been set or none.",
                {
                    "prompt_peak_correction_min": self.prompt_peak_correction_min,
                    "prompt_peak_correction_max": self.prompt_peak_correction_max,
                },
            )
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message(
                "Incorrect prompt peak correction bounds.",
                "Make sure that lower prompt peak time bound is smaller then upper bound.",
                {
                    "prompt_peak_correction_min": self.prompt_peak_correction_min,
                    "prompt_peak_correction_max": self.prompt_peak_correction_max,
                },
            )
            is_invalid.update(entry)
        # ----------------------
        # Background correction
        # ----------------------
        if not is_pure_none_or_not_none([self.background_TOF_general_start, self.background_TOF_general_stop]):
            entry = validation_message(
                "A general background TOF entry has not been set.",
                "Make sure that either all general background TOF entries are set or none.",
                {
                    "background_TOF_general_start": self.background_TOF_general_start,
                    "background_TOF_general_stop": self.background_TOF_general_stop,
                },
            )
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.background_TOF_general_start, self.background_TOF_general_stop]):
            entry = validation_message(
                "Incorrect general background TOF bounds.",
                "Make sure that lower general background TOF bound is smaller then upper bound.",
                {
                    "background_TOF_general_start": self.background_TOF_general_start,
                    "background_TOF_general_stop": self.background_TOF_general_stop,
                },
            )
            is_invalid.update(entry)

        if not is_pure_none_or_not_none([self.background_TOF_monitor_start, self.background_TOF_monitor_stop]):
            entry = validation_message(
                "A monitor background TOF entry has not been set.",
                "Make sure that either all monitor background TOF entries are set or none.",
                {
                    "background_TOF_monitor_start": self.background_TOF_monitor_start,
                    "background_TOF_monitor_stop": self.background_TOF_monitor_stop,
                },
            )
            is_invalid.update(entry)

        if self.background_TOF_monitor_start is not None and self.background_TOF_monitor_stop is not None:
            if len(self.background_TOF_monitor_start) != len(self.background_TOF_monitor_stop):
                entry = validation_message(
                    "The monitor background TOF entries have a length mismatch.",
                    "Make sure that all monitor background TOF entries have the same length.",
                    {
                        "background_TOF_monitor_start": self.background_TOF_monitor_start,
                        "background_TOF_monitor_stop": self.background_TOF_monitor_stop,
                    },
                )
                is_invalid.update(entry)
            for key_start, value_start in list(self.background_TOF_monitor_start.items()):
                if key_start not in self.background_TOF_monitor_stop:
                    entry = validation_message(
                        "The monitor background TOF had spectrum number mismatch.",
                        "Make sure that all monitors have entries for start and stop.",
                        {
                            "background_TOF_monitor_start": self.background_TOF_monitor_start,
                            "background_TOF_monitor_stop": self.background_TOF_monitor_stop,
                        },
                    )
                    is_invalid.update(entry)
                else:
                    value_stop = self.background_TOF_monitor_stop[key_start]
                    if value_start > value_stop:
                        entry = validation_message(
                            "Incorrect monitor background TOF bounds.",
                            "Make sure that lower monitor background TOF bound is smaller then upper bound.",
                            {
                                "background_TOF_monitor_start": self.background_TOF_monitor_start,
                                "background_TOF_monitor_stop": self.background_TOF_monitor_stop,
                            },
                        )
                        is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateMoveDetectors: The provided inputs are illegal. Please see: {0}".format(json.dumps(is_invalid)))


class StateNormalizeToMonitorLOQ(StateNormalizeToMonitor):
    def __init__(self):
        super(StateNormalizeToMonitorLOQ, self).__init__()
        # Set the LOQ default range for prompt peak correction
        self.prompt_peak_correction_min = 19000.0
        self.prompt_peak_correction_max = 20500.0
        self.prompt_peak_correction_enabled = True


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_default_incident_monitor(normalize_monitor_info, data_info):
    """
    The default incident monitor is stored on the IPF.
    :param normalize_monitor_info: a StateNormalizeMonitor object on which we set the default value
    :param data_info: a StateData object
    """
    ipf_file_path = data_info.ipf_file_path
    if not ipf_file_path:
        return
    named_element = "default-incident-monitor-spectrum"
    monitor_spectrum_tag_to_search = [named_element]
    found_monitor_spectrum = get_named_elements_from_ipf_file(ipf_file_path, monitor_spectrum_tag_to_search, int)
    if named_element in found_monitor_spectrum:
        normalize_monitor_info.incident_monitor = found_monitor_spectrum[named_element]


class StateNormalizeToMonitorBuilder(object):
    @automatic_setters(StateNormalizeToMonitor, exclusions=["default_incident_monitor"])
    def __init__(self, data_info):
        super(StateNormalizeToMonitorBuilder, self).__init__()
        self._data = data_info
        self.state = StateNormalizeToMonitor()
        if data_info.instrument is not SANSInstrument.NO_INSTRUMENT:
            set_default_incident_monitor(self.state, self._data)

    def build(self):
        return copy.copy(self.state)

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val

    def set_rebin_type(self, val):
        self.state.rebin_type = val


class StateNormalizeToMonitorBuilderLOQ(object):
    @automatic_setters(StateNormalizeToMonitorLOQ)
    def __init__(self, data_info):
        super(StateNormalizeToMonitorBuilderLOQ, self).__init__()
        self._data = data_info
        self.state = StateNormalizeToMonitorLOQ()
        set_default_incident_monitor(self.state, self._data)

    def build(self):
        return copy.copy(self.state)

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val

    def set_rebin_type(self, val):
        self.state.rebin_type = val


def get_normalize_to_monitor_builder(data_info):
    instrument = data_info.instrument

    if instrument is SANSInstrument.LOQ:
        return StateNormalizeToMonitorBuilderLOQ(data_info)
    else:
        return StateNormalizeToMonitorBuilder(data_info)
