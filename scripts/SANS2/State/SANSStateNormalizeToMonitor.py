# pylint: disable=too-few-public-methods

"""State describing the normalization to the incident monitor for SANS reduction."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, PositiveIntegerParameter,
                                       PositiveFloatParameter, FloatParameter, ClassTypeParameter, DictParameter,
                                       PositiveFloatWithNoneParameter)
from SANS2.Common.SANSType import (RebinType, RangeStepType)
from SANS2.State.SANSStateFunctions import (is_pure_none_or_not_none, is_not_none_and_first_larger_than_second,
                                            one_is_none, validation_message)


# ------------------------------------------------
# SANSStateAdjustment
# ------------------------------------------------
class SANSStateNormalizeToMonitor(object):
    pass


@sans_parameters
class SANSStateNormalizeToMonitorISIS(SANSStateBase, SANSStateNormalizeToMonitor):
    prompt_peak_correction_min = PositiveFloatWithNoneParameter()
    prompt_peak_correction_max = PositiveFloatWithNoneParameter()

    rebin_type = ClassTypeParameter(RebinType)
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    background_TOF_general_start = FloatParameter()
    background_TOF_general_stop = FloatParameter()
    background_TOF_monitor_start = DictParameter()
    background_TOF_monitor_stop = DictParameter()

    incident_monitor = PositiveIntegerParameter()

    def __init__(self):
        super(SANSStateNormalizeToMonitorISIS, self).__init__()
        self.background_TOF_monitor_start = {}
        self.background_TOF_monitor_stop = {}

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
            entry = validation_message("A prompt peak correction entry has not been set.",
                                       "Make sure that either all prompt peak entries have been set or none.",
                                       {"prompt_peak_correction_min": self.prompt_peak_correction_min,
                                        "prompt_peak_correction_max": self.prompt_peak_correction_max})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message("Incorrect prompt peak correction bounds.",
                                       "Make sure that lower prompt peak time bound is smaller then upper bound.",
                                       {"prompt_peak_correction_min": self.prompt_peak_correction_min,
                                        "prompt_peak_correction_max": self.prompt_peak_correction_max})
            is_invalid.update(entry)

        # -----------------
        # Wavelength rebin
        # -----------------
        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step, self.wavelength_step_type]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step,
                                        "wavelength_step_type": self.wavelength_step_type})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
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
            for key_start, value_start in self.background_TOF_monitor_start.items():
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

        if is_invalid:
            raise ValueError("SANSStateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@sans_parameters
class SANSStateNormalizeToMonitorLOQ(SANSStateNormalizeToMonitorISIS):
    def __init__(self):
        super(SANSStateNormalizeToMonitorLOQ, self).__init__()
        # Set the LOQ default range for prompt peak correction
        self.prompt_peak_correction_min = 19000.0
        self.prompt_peak_correction_max = 20500.0

    def validate(self):
        super(SANSStateNormalizeToMonitorLOQ, self).validate()

# -----------------------------------------------
# SANSStateNormalizeMonitor setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateNormalizeMonitor and SANSStateBase and fulfill its contract.
# -----------------------------------------------
