# pylint: disable=too-few-public-methods

"""State describing the conversion to momentum transfer"""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, BoolParameter, PositiveFloatParameter,
                                       ClassTypeParameter, StringParameter)
from SANS2.Common.SANSType import (ReductionDimensionality, RangeStepType)
from SANS2.State.SANSStateFunctions import (is_pure_none_or_not_none, is_not_none_and_first_larger_than_second,
                                            validation_message)


# ------------------------------------------------
# SANSStateConvertToQ
# ------------------------------------------------
class SANSStateConvertToQ(object):
    pass


@sans_parameters
class SANSStateConvertToQISIS(SANSStateBase, SANSStateConvertToQ):
    reduction_dimensionality = ClassTypeParameter(ReductionDimensionality)
    use_gravity = BoolParameter()
    gravity_extra_length = PositiveFloatParameter()
    radius_cutoff = PositiveFloatParameter()
    wavelength_cutoff = PositiveFloatParameter()

    # 1D settings
    # The complex binning instructions require a second step and a mid point, which produces:
    #   start -> step -> mid -> step2 -> stop
    # The simple form is:
    #   start -> step -> stop
    q_min = PositiveFloatParameter()
    q_max = PositiveFloatParameter()
    q_step = PositiveFloatParameter()
    q_step_type = ClassTypeParameter(RangeStepType)
    q_step2 = PositiveFloatParameter()
    q_step_type2 = ClassTypeParameter(RangeStepType)
    q_mid = PositiveFloatParameter()

    # 2D settings
    q_xy_max = PositiveFloatParameter()
    q_xy_step = PositiveFloatParameter()
    q_xy_step_type = ClassTypeParameter(RangeStepType)

    # -----------------------
    # Q Resolution specific
    # ---------------------
    use_q_resolution = BoolParameter()
    q_resolution_collimation_length = PositiveFloatParameter()
    q_resolution_delta_r = PositiveFloatParameter()
    moderator_file = StringParameter()

    # Circular aperture settings
    q_resolution_a1 = PositiveFloatParameter()
    q_resolution_a2 = PositiveFloatParameter()

    # Rectangular aperture settings
    q_resolution_h1 = PositiveFloatParameter()
    q_resolution_h2 = PositiveFloatParameter()
    q_resolution_w1 = PositiveFloatParameter()
    q_resolution_w2 = PositiveFloatParameter()

    def __init__(self):
        super(SANSStateConvertToQISIS, self).__init__()
        self.reduction_dimensionality = ReductionDimensionality.OneDim
        self.use_gravity = False
        self.gravity_extra_length = 0.0
        self.use_q_resolution = False
        self.radius_cutoff = 0.0
        self.wavelength_cutoff = 0.0

    def validate(self):
        is_invalid = {}

        # 1D Q settings
        if not is_pure_none_or_not_none([self.q_min, self.q_max]):
            entry = validation_message("The q boundaries for the 1D reduction are inconsistent.",
                                       "Make sure that both q boundaries are set (or none).",
                                       {"q_min": self.q_min,
                                        "q_max": self.q_max})
            is_invalid.update(entry)
        if is_not_none_and_first_larger_than_second([self.q_min, self.q_max]):
            entry = validation_message("Incorrect q bounds for 1D reduction.",
                                       "Make sure that the lower q bound is smaller than the upper q bound.",
                                       {"q_min": self.q_min,
                                        "q_max": self.q_max})
            is_invalid.update(entry)

        if self.reduction_dimensionality is ReductionDimensionality.OneDim:
            if self.q_min is None or self.q_max is None:
                entry = validation_message("Q bounds not set for 1D reduction.",
                                           "Make sure to set the q boundaries when using a 1D reduction.",
                                           {"q_min": self.q_min,
                                            "q_max": self.q_max})
                is_invalid.update(entry)

        if self.reduction_dimensionality is ReductionDimensionality.TwoDim:
            if self.q_xy_max is None or self.q_xy_step is None:
                entry = validation_message("Q bounds not set for 2D reduction.",
                                           "Make sure that the q_max value bound and the step for the 2D reduction.",
                                           {"q_xy_max": self.q_xy_max,
                                            "q_xy_step": self.q_xy_step})
                is_invalid.update(entry)

        # Q Resolution settings
        if self.use_q_resolution:
            if not is_pure_none_or_not_none([self.q_resolution_a1, self.q_resolution_a2]):
                entry = validation_message("Inconsistent circular geometry.",
                                           "Make sure that both diameters for the circular apertures are set.",
                                           {"q_resolution_a1": self.q_resolution_a1,
                                            "q_resolution_a2": self.q_resolution_a2})
                is_invalid.update(entry)
            if not is_pure_none_or_not_none([self.q_resolution_h1, self.q_resolution_h2, self.q_resolution_w1,
                                             self.q_resolution_w2]):
                entry = validation_message("Inconsistent rectangular geometry.",
                                           "Make sure that both diameters for the circular apertures are set.",
                                           {"q_resolution_h1": self.q_resolution_h1,
                                            "q_resolution_h2": self.q_resolution_h2,
                                            "q_resolution_w1": self.q_resolution_w1,
                                            "q_resolution_w2": self.q_resolution_w2})
                is_invalid.update(entry)

            if all(element is None for element in [self.q_resolution_a1, self.q_resolution_a2, self.q_resolution_w1,
                                                   self.q_resolution_w2, self.q_resolution_h1, self.q_resolution_h2]):
                entry = validation_message("Aperture is undefined.",
                                           "Make sure that you set the geometry for a circular or a "
                                           "rectangular aperture.",
                                           {"q_resolution_a1": self.q_resolution_a1,
                                            "q_resolution_a2": self.q_resolution_a2,
                                            "q_resolution_h1": self.q_resolution_h1,
                                            "q_resolution_h2": self.q_resolution_h2,
                                            "q_resolution_w1": self.q_resolution_w1,
                                            "q_resolution_w2": self.q_resolution_w2})
                is_invalid.update(entry)
            if self.moderator_file is None:
                entry = validation_message("Missing moderator file.",
                                           "Make sure to specify a moderator file when using q resolution.",
                                           {"moderator_file": self.moderator_file})
                is_invalid.update(entry)
                is_invalid.update({"moderator_file": "A moderator file is required for the q resolution calculation."})

        if is_invalid:
            raise ValueError("SANSStateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# -----------------------------------------------
# SANSStateConvertToQ setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateConvertToQ and SANSStateBase and fulfill its contract.
# -----------------------------------------------
