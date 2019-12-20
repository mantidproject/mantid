# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

""" Defines the state of the reduction."""

from __future__ import (absolute_import, division, print_function)

import copy
import json
from abc import (ABCMeta, abstractmethod)

from six import (with_metaclass)

from sans.common.enums import (ReductionMode, ReductionDimensionality, FitModeForMerge,
                               SANSFacility, DetectorType)
from sans.common.xml_parsing import get_named_elements_from_ipf_file
from sans.state.automatic_setters import (automatic_setters)
from sans.state.state_base import (StateBase, FloatParameter, DictParameter,
                                   FloatWithNoneParameter, rename_descriptor_names, BoolParameter)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionBase(with_metaclass(ABCMeta, object)):
    @abstractmethod
    def get_merge_strategy(self):
        pass

    @abstractmethod
    def get_detector_name_for_reduction_mode(self, reduction_mode):
        pass

    @abstractmethod
    def get_all_reduction_modes(self):
        pass


@rename_descriptor_names
class StateReductionMode(StateReductionBase, StateBase):
    reduction_mode = ReductionMode.NOT_SET

    reduction_dimensionality = ReductionDimensionality.ONE_DIM
    merge_max = FloatWithNoneParameter()
    merge_min = FloatWithNoneParameter()
    merge_mask = BoolParameter()

    # Fitting
    merge_fit_mode = FitModeForMerge.NO_FIT
    merge_shift = FloatParameter()
    merge_scale = FloatParameter()
    merge_range_min = FloatWithNoneParameter()
    merge_range_max = FloatWithNoneParameter()

    # Map from detector type to detector name
    detector_names = DictParameter()

    def __init__(self):
        super(StateReductionMode, self).__init__()
        self.reduction_mode = ReductionMode.LAB
        self.reduction_dimensionality = ReductionDimensionality.ONE_DIM

        # Set the shifts to defaults which essentially don't do anything.
        self.merge_shift = 0.0
        self.merge_scale = 1.0
        self.merge_fit_mode = FitModeForMerge.NO_FIT
        self.merge_range_min = None
        self.merge_range_max = None
        self.merge_max = None
        self.merge_min = None
        self.merge_mask = False

        # Set the detector names to empty strings
        self.detector_names = {DetectorType.LAB.value: "",
                               DetectorType.HAB.value: ""}

    def get_merge_strategy(self):
        return [ReductionMode.LAB, ReductionMode.HAB]

    def get_all_reduction_modes(self):
        return [ReductionMode.LAB, ReductionMode.HAB]

    def get_detector_name_for_reduction_mode(self, reduction_mode):
        if reduction_mode is ReductionMode.LAB:
            bank_type = DetectorType.LAB.value
        elif reduction_mode is ReductionMode.HAB:
            bank_type = DetectorType.HAB.value
        else:
            raise RuntimeError("SANStateReductionISIS: There is no detector available for the"
                               " reduction mode {0}.".format(reduction_mode))
        return self.detector_names[bank_type]

    def validate(self):
        is_invalid = {}
        if self.merge_max and self.merge_min:
            if self.merge_min > self.merge_max:
                is_invalid.update({"StateReduction": "The minimum of the merge region is greater than the maximum."})

        if is_invalid:
            raise ValueError("StateReduction: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def setup_detectors_from_ipf(reduction_info, data_info):
    ipf_file_path = data_info.ipf_file_path

    detector_names = {DetectorType.LAB.value: "low-angle-detector-name",
                      DetectorType.HAB.value: "high-angle-detector-name"}

    names_to_search = []
    names_to_search.extend(list(detector_names.values()))

    found_detector_names = get_named_elements_from_ipf_file(ipf_file_path, names_to_search, str)

    for detector_type in list(reduction_info.detector_names.keys()):
        try:
            detector_name_tag = detector_names[detector_type]
            detector_name = found_detector_names[detector_name_tag]
        except KeyError:
            continue
        reduction_info.detector_names[detector_type] = detector_name


class StateReductionModeBuilder(object):
    @automatic_setters(StateReductionMode, exclusions=["detector_names"])
    def __init__(self, data_info):
        self.state = StateReductionMode()
        setup_detectors_from_ipf(self.state, data_info)

    # TODO this whole class is a shim around state, so we should remove it at a later date
    def set_reduction_mode(self, val):
        self.state.reduction_mode = val

    def set_reduction_dimensionality(self, val):
        self.state.reduction_dimensionality = val

    def set_merge_fit_mode(self, val):
        self.state.merge_fit_mode = val

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_reduction_mode_builder(data_info):
    # The data state has most of the information that we require to define the reduction_mode.
    # For the factory method, only the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateReductionModeBuilder(data_info)
    else:
        raise NotImplementedError("StateReductionBuilder: Could not find any valid reduction builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
