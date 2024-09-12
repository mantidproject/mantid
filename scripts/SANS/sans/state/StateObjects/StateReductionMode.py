# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the reduction."""

import json

from sans.common.enums import ReductionMode, ReductionDimensionality, FitModeForMerge
from sans.state.JsonSerializable import JsonSerializable


class StateReductionMode(metaclass=JsonSerializable):
    def __init__(self):
        super(StateReductionMode, self).__init__()
        self.reduction_mode = ReductionMode.LAB
        self.reduction_dimensionality = ReductionDimensionality.ONE_DIM

        # Set the shifts to defaults which essentially don't do anything.
        self.merge_shift = 0.0  # : Float
        self.merge_scale = 1.0  # : Float
        self.merge_fit_mode = FitModeForMerge.NO_FIT
        self.merge_range_min = None  # : Float
        self.merge_range_max = None  # : Float
        self.merge_max = None  # : Float
        self.merge_min = None  # : Float
        self.merge_mask = False  # : Bool

    def get_merge_strategy(self):
        return [ReductionMode.LAB, ReductionMode.HAB]

    def get_all_reduction_modes(self):
        return [ReductionMode.LAB, ReductionMode.HAB]

    def validate(self):
        is_invalid = {}
        if self.merge_max and self.merge_min:
            if self.merge_min > self.merge_max:
                is_invalid.update({"StateReduction": "The minimum of the merge" " region is greater than" " the maximum."})

        if is_invalid:
            raise ValueError("StateReduction: The provided inputs are illegal." " Please see: {0}".format(json.dumps(is_invalid)))
