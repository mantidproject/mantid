# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the conversion to momentum transfer"""

import json
import copy

from sans_core.state.JsonSerializable import JsonSerializable
from sans_core.common.enums import ReductionDimensionality, SANSFacility
from sans_core.state.automatic_setters import automatic_setters
from sans_core.state.state_functions import is_pure_none_or_not_none, is_not_none_and_first_larger_than_second, validation_message


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StateConvertToQ(metaclass=JsonSerializable):
    def __init__(self):
        super(StateConvertToQ, self).__init__()
        self.reduction_dimensionality = ReductionDimensionality.ONE_DIM
        self.use_gravity = True  # : Bool
        self.gravity_extra_length = 0.0  # : Float (Positive)
        self.radius_cutoff = 0.0  # : Float (Positive)
        self.wavelength_cutoff = 0.0  # : Float (Positive)

        # 1D settings
        self.q_min = None  # : Float (Positive)
        self.q_max = None  # : Float (Positive)
        self.q_1d_rebin_string = None  # : Str()

        # 2D settings
        self.q_xy_max = None  # : Float (Positive)
        self.q_xy_step = None  # : Float (Positive)

        # -----------------------
        # Q Resolution specific
        # ---------------------
        self.use_q_resolution = False  # : Bool
        self.q_resolution_collimation_length = None  # : Float (Positive)
        self.q_resolution_delta_r = None  # : Float (Positive)
        self.moderator_file = None  # : Str()

        # Circular aperture settings
        # Source aperture diameter
        self.q_resolution_a1 = None  # : Float (Positive)
        # Sample aperture diameter
        self.q_resolution_a2 = None  # : Float (Positive)

        # Rectangular aperture settings
        self.q_resolution_h1 = None  # : Float (Positive)
        self.q_resolution_h2 = None  # : Float (Positive)
        self.q_resolution_w1 = None  # : Float (Positive)
        self.q_resolution_w2 = None  # : Float (Positive)

    def validate(self):
        is_invalid = {}

        # 1D Q settings
        if not is_pure_none_or_not_none([self.q_min, self.q_max]):
            entry = validation_message(
                "The q boundaries for the 1D reduction are inconsistent.",
                "Make sure that both q boundaries are set (or none).",
                {"q_min": self.q_min, "q_max": self.q_max},
            )
            is_invalid.update(entry)
        if is_not_none_and_first_larger_than_second([self.q_min, self.q_max]):
            entry = validation_message(
                "Incorrect q bounds for 1D reduction.",
                "Make sure that the lower q bound is smaller than the upper q bound.",
                {"q_min": self.q_min, "q_max": self.q_max},
            )
            is_invalid.update(entry)

        if self.reduction_dimensionality is ReductionDimensionality.ONE_DIM:
            if self.q_min is None or self.q_max is None:
                entry = validation_message(
                    "Q bounds not set for 1D reduction.",
                    "Make sure to set the q boundaries when using a 1D reduction.",
                    {"q_min": self.q_min, "q_max": self.q_max},
                )
                is_invalid.update(entry)

        if self.q_1d_rebin_string is not None:
            if self.q_1d_rebin_string == "":
                entry = validation_message(
                    "Q rebin string does not seem to be valid.",
                    "Make sure to provide a valid rebin string",
                    {"q_1d_rebin_string": self.q_1d_rebin_string},
                )
                is_invalid.update(entry)
            elif not is_valid_rebin_string(self.q_1d_rebin_string):
                entry = validation_message(
                    "Q rebin string does not seem to be valid.",
                    "Make sure to provide a valid rebin string",
                    {"q_1d_rebin_string": self.q_1d_rebin_string},
                )
                is_invalid.update(entry)

        # QXY settings
        if self.reduction_dimensionality is ReductionDimensionality.TWO_DIM:
            if self.q_xy_max is None or self.q_xy_step is None:
                entry = validation_message(
                    "Q bounds not set for 2D reduction.",
                    "Make sure that the q_max value bound and the step for the 2D reduction.",
                    {"q_xy_max": self.q_xy_max, "q_xy_step": self.q_xy_step},
                )
                is_invalid.update(entry)

        # Q Resolution settings
        if self.use_q_resolution:
            if not is_pure_none_or_not_none([self.q_resolution_a1, self.q_resolution_a2]):
                entry = validation_message(
                    "Inconsistent circular geometry.",
                    "Make sure that both diameters for the circular apertures are set.",
                    {"q_resolution_a1": self.q_resolution_a1, "q_resolution_a2": self.q_resolution_a2},
                )
                is_invalid.update(entry)
            if not is_pure_none_or_not_none([self.q_resolution_h1, self.q_resolution_h2, self.q_resolution_w1, self.q_resolution_w2]):
                entry = validation_message(
                    "Inconsistent rectangular geometry.",
                    "Make sure that both diameters for the circular apertures are set.",
                    {
                        "q_resolution_h1": self.q_resolution_h1,
                        "q_resolution_h2": self.q_resolution_h2,
                        "q_resolution_w1": self.q_resolution_w1,
                        "q_resolution_w2": self.q_resolution_w2,
                    },
                )
                is_invalid.update(entry)

            if all(
                element is None
                for element in [
                    self.q_resolution_a1,
                    self.q_resolution_a2,
                    self.q_resolution_w1,
                    self.q_resolution_w2,
                    self.q_resolution_h1,
                    self.q_resolution_h2,
                ]
            ):
                entry = validation_message(
                    "Aperture is undefined.",
                    "Make sure that you set the geometry for a circular or a " "rectangular aperture.",
                    {
                        "q_resolution_a1": self.q_resolution_a1,
                        "q_resolution_a2": self.q_resolution_a2,
                        "q_resolution_h1": self.q_resolution_h1,
                        "q_resolution_h2": self.q_resolution_h2,
                        "q_resolution_w1": self.q_resolution_w1,
                        "q_resolution_w2": self.q_resolution_w2,
                    },
                )
                is_invalid.update(entry)
            if self.moderator_file is None:
                entry = validation_message(
                    "Missing moderator file.",
                    "Make sure to specify a moderator file when using q resolution.",
                    {"moderator_file": self.moderator_file},
                )
                is_invalid.update(entry)
                is_invalid.update({"moderator_file": "A moderator file is required for the q resolution calculation."})

        if is_invalid:
            raise ValueError("StateMoveDetectorISIS: The provided inputs are illegal. " "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateConvertToQBuilder(object):
    @automatic_setters(StateConvertToQ)
    def __init__(self):
        super(StateConvertToQBuilder, self).__init__()
        self.state = StateConvertToQ()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def set_reduction_dimensionality(self, val):
        self.state.reduction_dimensionality = val


# ------------------------------------------
# Factory method for StateConvertToQBuilder
# ------------------------------------------
def get_convert_to_q_builder(data_info):
    # The data state has most of the information that we require to define the q conversion.
    # For the factory method, only the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateConvertToQBuilder()
    else:
        raise NotImplementedError(
            "StateConvertToQBuilder: Could not find any valid save builder for the " "specified StateData object {0}".format(str(data_info))
        )


# -------------------------------------------
# Free functions
# -------------------------------------------
def is_valid_rebin_string(rebin_string):
    is_valid = True

    try:
        values = [float(el) for el in rebin_string.split(",")]
        if len(values) < 2:
            is_valid = False
        elif len(values) == 2:
            if values[0] > values[1]:
                is_valid = False
        elif len(values) % 2 == 1:  # odd number of entries
            step_points = values[::2]
            if not is_increasing(step_points):
                is_valid = False
        else:
            is_valid = False

    except:
        is_valid = False
    return is_valid


def is_increasing(step_points):
    return all(el1 <= el2 for el1, el2 in zip(step_points, step_points[1:]))
