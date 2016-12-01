# pylint: disable=too-few-public-methods

"""State describing the calculation of the transmission for SANS reduction."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, PositiveIntegerParameter,
                                       BoolParameter, PositiveFloatParameter, ClassTypeParameter,
                                       FloatParameter, DictParameter, StringListParameter, StringParameter,
                                       PositiveFloatWithNoneParameter)
from SANS2.Common.SANSType import (RebinType, RangeStepType, FitType, DataType,
                                           convert_reduction_data_type_to_string)
from SANS2.Common.SANSConfigurations import SANSConfigurations
from SANS2.State.SANSStateFunctions import (is_pure_none_or_not_none, validation_message,
                                            is_not_none_and_first_larger_than_second, one_is_none)


# ------------------------------------------------
# SANSStateAdjustment
# ------------------------------------------------
class SANSStateCalculateTransmission(object):
    pass


@sans_parameters
class SANSStateTransmissionFit(SANSStateBase):
    fit_type = ClassTypeParameter(FitType)
    polynomial_order = PositiveIntegerParameter()
    wavelength_low = PositiveFloatWithNoneParameter()
    wavelength_high = PositiveFloatWithNoneParameter()

    def __init__(self):
        super(SANSStateTransmissionFit, self).__init__()
        self.fit_type = FitType.Linear
        self.polynomial_order = 0

    def validate(self):
        is_invalid = {}
        if self.fit_type is not FitType.Polynomial and self.polynomial_order != 0:
            entry = validation_message("You can only set a plynomial order of you selected polynomial fitting.",
                                       "Make sure that you select polynomial fitting.",
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
            raise ValueError("SANSStateTransmissionFit: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@sans_parameters
class SANSStateCalculateTransmissionISIS(SANSStateBase, SANSStateCalculateTransmission):
    # -----------------------
    # Transmission
    # -----------------------
    transmission_radius_on_detector = PositiveFloatParameter()
    transmission_roi_files = StringListParameter()
    transmission_mask_files = StringListParameter()

    default_transmission_monitor = PositiveIntegerParameter()
    transmission_monitor = PositiveIntegerParameter()

    default_incident_monitor = PositiveIntegerParameter()
    incident_monitor = PositiveIntegerParameter()

    # ----------------------
    # Prompt peak correction
    # ----------------------
    prompt_peak_correction_min = PositiveFloatParameter()
    prompt_peak_correction_max = PositiveFloatParameter()

    # ----------------
    # Wavelength rebin
    # ----------------
    rebin_type = ClassTypeParameter(RebinType)
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    use_full_wavelength_range = BoolParameter()
    wavelength_full_range_low = PositiveFloatParameter()
    wavelength_full_range_high = PositiveFloatParameter()

    # -----------------------
    # Background correction
    # ----------------------
    background_TOF_general_start = FloatParameter()
    background_TOF_general_stop = FloatParameter()
    background_TOF_monitor_start = DictParameter()
    background_TOF_monitor_stop = DictParameter()
    background_TOF_roi_start = FloatParameter()
    background_TOF_roi_stop = FloatParameter()

    # -----------------------
    # Fit
    # ----------------------
    fit = DictParameter()

    def __init__(self):
        super(SANSStateCalculateTransmissionISIS, self).__init__()
        self.background_TOF_monitor_start = {}
        self.background_TOF_monitor_stop = {}
        self.fit = {convert_reduction_data_type_to_string(DataType.Sample): SANSStateTransmissionFit(),
                    convert_reduction_data_type_to_string(DataType.Can): SANSStateTransmissionFit()}
        self.use_full_wavelength_range = False

    def validate(self):
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
        has_no_transmission_monitor_setting = self.transmission_monitor is None and\
                                              self.default_transmission_monitor is None
        has_no_transmission_roi_setting = self.transmission_radius_on_detector is None and \
                                          self.transmission_roi_files is None
        if has_no_transmission_monitor_setting and has_no_transmission_roi_setting:
            entry = validation_message("No transmission settings were specified.",
                                       "Make sure that transmission settings are specified.",
                                       {"transmission_monitor": self.transmission_monitor,
                                        "default_transmission_monitor": self.default_transmission_monitor,
                                        "transmission_radius_on_detector": self.transmission_radius_on_detector,
                                        "transmission_roi_files": self.transmission_roi_files,})
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
        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step, self.wavelength_step_type,
                        self.rebin_type]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step,
                                        "wavelength_step_type": self.wavelength_step_type,
                                        "rebin_type": self.rebin_type})
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
            if is_not_none_and_first_larger_than_second([self.wavelength_full_range_low, self.wavelength_full_range_high]):
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

        # -----
        # Fit
        # -----
        self.fit[convert_reduction_data_type_to_string(DataType.Sample)].validate()
        self.fit[convert_reduction_data_type_to_string(DataType.Can)].validate()

        if is_invalid:
            raise ValueError("SANSStateCalculateTransmissionISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


class SANSStateCalculateTransmissionLOQ(SANSStateCalculateTransmissionISIS):
    def __init__(self):
        super(SANSStateCalculateTransmissionLOQ, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = SANSConfigurations.LOQ.wavelength_full_range_low
        self.wavelength_full_range_high = SANSConfigurations.LOQ.wavelength_full_range_high

        # Set the LOQ default range for prompt peak correction
        self.prompt_peak_correction_min = SANSConfigurations.LOQ.prompt_peak_correction_min
        self.prompt_peak_correction_max = SANSConfigurations.LOQ.prompt_peak_correction_max

    def validate(self):
        super(SANSStateCalculateTransmissionLOQ, self).validate()


class SANSStateCalculateTransmissionSANS2D(SANSStateCalculateTransmissionISIS):
    def __init__(self):
        super(SANSStateCalculateTransmissionSANS2D, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = SANSConfigurations.SANS2D.wavelength_full_range_low
        self.wavelength_full_range_high = SANSConfigurations.SANS2D.wavelength_full_range_high

    def validate(self):
        super(SANSStateCalculateTransmissionSANS2D, self).validate()


class SANSStateCalculateTransmissionLARMOR(SANSStateCalculateTransmissionISIS):
    def __init__(self):
        super(SANSStateCalculateTransmissionLARMOR, self).__init__()
        # Set the LOQ full wavelength range
        self.wavelength_full_range_low = SANSConfigurations.LARMOR.wavelength_full_range_low
        self.wavelength_full_range_high = SANSConfigurations.LARMOR.wavelength_full_range_high

    def validate(self):
        super(SANSStateCalculateTransmissionLARMOR, self).validate()

# -----------------------------------------------
# SANSStateCalculateTransmission setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateCalculateTransmission and SANSStateBase and fulfill its contract.
# -----------------------------------------------
