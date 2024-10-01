# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the calculation of the transmission for SANS reduction."""

import json
from typing import Dict

from sans.common.configurations import Configurations
from sans.common.enums import RebinType, RangeStepType, FitType, DataType, SANSInstrument
from sans.state.JsonSerializable import JsonSerializable
from sans.state.StateObjects.wavelength_interval import WavelengthInterval
from sans.state.state_functions import (
    is_pure_none_or_not_none,
    validation_message,
    is_not_none_and_first_larger_than_second,
    one_is_none,
)


class StateTransmissionFit(metaclass=JsonSerializable):
    def __init__(self):
        super(StateTransmissionFit, self).__init__()
        self.fit_type = FitType.LOGARITHMIC
        self.polynomial_order = 0  # : Int (Positive)
        self.wavelength_low = None  # : Float (Optional)
        self.wavelength_high = None  # : Float (Optional)

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        else:
            return False

    def __hash__(self):
        return hash((self.fit_type, self.polynomial_order, self.wavelength_low, self.wavelength_high))

    def validate(self):
        is_invalid = {}
        if self.fit_type is FitType.POLYNOMIAL and self.polynomial_order == 0:
            entry = validation_message(
                "You can only select a polynomial fit if you set a polynomial order (2 to 6).",
                "Make sure that you select a polynomial order.",
                {"fit_type": self.fit_type, "polynomial_order": self.polynomial_order},
            )
            is_invalid.update(entry)

        if not is_pure_none_or_not_none([self.wavelength_low, self.wavelength_high]):
            entry = validation_message(
                "Inconsistent wavelength setting.",
                "Make sure that you have specified both wavelength bounds (or none).",
                {"wavelength_low": self.wavelength_low, "wavelength_high": self.wavelength_high},
            )
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message(
                "Incorrect wavelength bounds.",
                "Make sure that lower wavelength bound is smaller then upper bound.",
                {"wavelength_low": self.wavelength_low, "wavelength_high": self.wavelength_high},
            )
            is_invalid.update(entry)
        if is_invalid:
            raise ValueError("StateTransmissionFit: The provided inputs are illegal. " "Please see: {0}".format(json.dumps(is_invalid)))


class StateCalculateTransmission(metaclass=JsonSerializable):
    def __init__(self):
        super(StateCalculateTransmission, self).__init__()
        # -----------------------
        # Transmission
        # -----------------------
        self.transmission_radius_on_detector = None  # : Float (Positive)
        self.transmission_roi_files = None  # : List[Str]
        self.transmission_mask_files = None  # : List[Str]

        self.incident_monitor = None  # : Int (Positive)
        self.transmission_monitor = None  # : Int (Positive)

        # ----------------------
        # Prompt peak correction
        # ----------------------
        self.prompt_peak_correction_min = None  # : Float (Positive)
        self.prompt_peak_correction_max = None  # : Float (Positive)
        self.prompt_peak_correction_enabled = False  # : Bool

        # ----------------
        # Wavelength rebin
        # ----------------
        self.wavelength_interval: WavelengthInterval = WavelengthInterval()
        self.rebin_type = RebinType.REBIN
        self.wavelength_step_type = RangeStepType.NOT_SET

        self.use_full_wavelength_range = False  # : Bool

        # -----------------------
        # Background correction
        # ----------------------
        self.background_TOF_general_start: float = None
        self.background_TOF_general_stop: float = None
        self.background_TOF_monitor_start: Dict[int, float] = {}
        self.background_TOF_monitor_stop: Dict[int, float] = {}
        self.background_TOF_roi_start: float = None
        self.background_TOF_roi_stop: float = None

        self.fit = {DataType.CAN.value: StateTransmissionFit(), DataType.SAMPLE.value: StateTransmissionFit()}

    def set_wavelength_step_type(self, val):
        self.wavelength_step_type = val

    @property
    def wavelength_step_type_lin_log(self):
        # Return the wavelength step type, converting RANGE_LIN/RANGE_LOG to
        # LIN/LOG. This is not ideal but is required for workflow algorithms
        # which only accept a subset of the values in the enum
        value = self.wavelength_step_type
        result = (
            RangeStepType.LIN
            if value in [RangeStepType.LIN, RangeStepType.RANGE_LIN]
            else RangeStepType.LOG
            if value in [RangeStepType.LOG, RangeStepType.RANGE_LOG]
            else RangeStepType.NOT_SET
        )
        return result

    def set_rebin_type(self, val):
        self.rebin_type = val

    def set_can_fit_type(self, val):
        self.fit[DataType.CAN.value].fit_type = val

    def set_can_polynomial_order(self, val):
        self.fit[DataType.CAN.value].polynomial_order = val

    def set_can_wavelength_low(self, val):
        self.fit[DataType.CAN.value].wavelength_low = val

    def set_can_wavelength_high(self, val):
        self.fit[DataType.CAN.value].wavelength_high = val

    def set_sample_fit_type(self, val):
        self.fit[DataType.SAMPLE.value].fit_type = val

    def set_sample_polynomial_order(self, val):
        self.fit[DataType.SAMPLE.value].polynomial_order = val

    def set_sample_wavelength_low(self, val):
        self.fit[DataType.SAMPLE.value].wavelength_low = val

    def set_sample_wavelength_high(self, val):
        self.fit[DataType.SAMPLE.value].wavelength_high = val

    def validate(self):
        is_invalid = {}
        # -----------------
        # Incident monitor
        # -----------------
        if self.incident_monitor is None:
            entry = validation_message(
                "No incident monitor was specified.",
                "Make sure that incident monitor has been specified.",
                {"incident_monitor": self.incident_monitor},
            )
            is_invalid.update(entry)

        # --------------
        # Transmission, either we need some ROI (ie radius, roi files /mask files) or a transmission monitor
        # --------------
        has_no_transmission_monitor_setting = self.transmission_monitor is None
        has_no_transmission_roi_setting = self.transmission_radius_on_detector is None and self.transmission_roi_files is None
        if has_no_transmission_monitor_setting and has_no_transmission_roi_setting:
            entry = validation_message(
                "No transmission settings were specified.",
                "Make sure that transmission settings are specified.",
                {
                    "transmission_monitor": self.transmission_monitor,
                    "transmission_radius_on_detector": self.transmission_radius_on_detector,
                    "transmission_roi_files": self.transmission_roi_files,
                },
            )
            is_invalid.update(entry)

        # -----------------
        # Prompt peak
        # -----------------
        if not is_pure_none_or_not_none([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message(
                "Inconsistent prompt peak setting.",
                "Make sure that you have specified both prompt peak bounds (or none).",
                {
                    "prompt_peak_correction_min": self.prompt_peak_correction_min,
                    "prompt_peak_correction_max": self.prompt_peak_correction_max,
                },
            )
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.prompt_peak_correction_min, self.prompt_peak_correction_max]):
            entry = validation_message(
                "Incorrect prompt peak bounds.",
                "Make sure that lower prompt peak bound is smaller then upper bound.",
                {
                    "prompt_peak_correction_min": self.prompt_peak_correction_min,
                    "prompt_peak_correction_max": self.prompt_peak_correction_max,
                },
            )
            is_invalid.update(entry)

        # -----------------
        # Wavelength rebin
        # -----------------
        if one_is_none([self.wavelength_interval, self.rebin_type]):
            entry = validation_message(
                "A wavelength entry has not been set.",
                "Make sure that all entries are set.",
                {
                    "wavelength_low": self.wavelength_interval,
                    "wavelength_step_type": self.wavelength_step_type,
                    "rebin_type": self.rebin_type,
                },
            )
            is_invalid.update(entry)

        if self.wavelength_step_type is RangeStepType.NOT_SET:
            entry = validation_message(
                "A wavelength entry has not been set.",
                "Make sure that all entries are set.",
                {"wavelength_step_type": self.wavelength_step_type},
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

        if not is_pure_none_or_not_none([self.background_TOF_roi_start, self.background_TOF_roi_stop]):
            entry = validation_message(
                "A ROI background TOF entry has not been set.",
                "Make sure that either all ROI background TOF entries are set or none.",
                {"background_TOF_roi_start": self.background_TOF_roi_start, "background_TOF_roi_stop": self.background_TOF_roi_stop},
            )
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.background_TOF_roi_start, self.background_TOF_roi_stop]):
            entry = validation_message(
                "Incorrect ROI background TOF bounds.",
                "Make sure that lower ROI background TOF bound is smaller then upper bound.",
                {"background_TOF_roi_start": self.background_TOF_roi_start, "background_TOF_roi_stop": self.background_TOF_roi_stop},
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
                            "Make sure that lower monitor background TOF bound is" " smaller then upper bound.",
                            {
                                "background_TOF_monitor_start": self.background_TOF_monitor_start,
                                "background_TOF_monitor_stop": self.background_TOF_monitor_stop,
                            },
                        )
                        is_invalid.update(entry)

        for fit_type in self.fit.values():
            fit_type.validate()

        if is_invalid:
            raise ValueError(
                "StateCalculateTransmission: The provided inputs are illegal. " "Please see: {0}".format(json.dumps(is_invalid))
            )


class StateCalculateTransmissionLOQ(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionLOQ, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.LOQ.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.LOQ.wavelength_full_range_high

        self.incident_monitor = Configurations.LOQ.default_incident_monitor
        self.transmission_monitor = Configurations.LOQ.default_transmission_monitor

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
        self.incident_monitor = Configurations.SANS2D.default_incident_monitor
        self.transmission_monitor = Configurations.SANS2D.default_transmission_monitor

    def validate(self):
        super(StateCalculateTransmissionSANS2D, self).validate()


class StateCalculateTransmissionLARMOR(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionLARMOR, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.LARMOR.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.LARMOR.wavelength_full_range_high
        self.incident_monitor = Configurations.LARMOR.default_incident_monitor
        self.transmission_monitor = Configurations.LARMOR.default_transmission_monitor

    def validate(self):
        super(StateCalculateTransmissionLARMOR, self).validate()


class StateCalculateTransmissionZOOM(StateCalculateTransmission):
    def __init__(self):
        super(StateCalculateTransmissionZOOM, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = Configurations.ZOOM.wavelength_full_range_low
        self.wavelength_full_range_high = Configurations.ZOOM.wavelength_full_range_high
        self.incident_monitor = Configurations.ZOOM.default_incident_monitor
        self.transmission_monitor = Configurations.ZOOM.default_transmission_monitor

    def validate(self):
        super(StateCalculateTransmissionZOOM, self).validate()


# ------------------------------------------
# Factory method for StateCalculateTransmissionBuilder
# ------------------------------------------


def get_calculate_transmission(instrument):
    if instrument is SANSInstrument.LARMOR:
        return StateCalculateTransmissionLARMOR()
    elif instrument is SANSInstrument.SANS2D:
        return StateCalculateTransmissionSANS2D()
    elif instrument is SANSInstrument.LOQ:
        return StateCalculateTransmissionLOQ()
    elif instrument is SANSInstrument.ZOOM:
        return StateCalculateTransmissionZOOM()
    elif instrument is SANSInstrument.NO_INSTRUMENT:
        return StateCalculateTransmission()
    else:
        raise NotImplementedError(
            "StateCalculateTransmissionBuilder: Could not find any valid transmission "
            "builder for the specified StateData object {0}".format(str(instrument))
        )
