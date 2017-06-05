# pylint: disable=too-few-public-methods

""" Defines the state of the reduction."""

from __future__ import (absolute_import, division, print_function)
from abc import (ABCMeta, abstractmethod)
from six import (with_metaclass)
import copy

from sans.state.state_base import (StateBase, ClassTypeParameter, FloatParameter, DictParameter,
                                   FloatWithNoneParameter, rename_descriptor_names)
from sans.common.enums import (ReductionMode, ISISReductionMode, ReductionDimensionality, FitModeForMerge,
                               SANSInstrument, DetectorType)
from sans.common.xml_parsing import get_named_elements_from_ipf_file
from sans.state.automatic_setters import (automatic_setters)


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
    reduction_mode = ClassTypeParameter(ReductionMode)
    reduction_dimensionality = ClassTypeParameter(ReductionDimensionality)

    # Fitting
    merge_fit_mode = ClassTypeParameter(FitModeForMerge)
    merge_shift = FloatParameter()
    merge_scale = FloatParameter()
    merge_range_min = FloatWithNoneParameter()
    merge_range_max = FloatWithNoneParameter()

    # Map from detector type to detector name
    detector_names = DictParameter()

    def __init__(self):
        super(StateReductionMode, self).__init__()
        self.reduction_mode = ISISReductionMode.LAB
        self.reduction_dimensionality = ReductionDimensionality.OneDim

        # Set the shifts to defaults which essentially don't do anything.
        self.merge_shift = 0.0
        self.merge_scale = 1.0
        self.merge_fit_mode = FitModeForMerge.NoFit
        self.merge_range_min = None
        self.merge_range_max = None

        # Set the detector names to empty strings
        self.detector_names = {DetectorType.to_string(DetectorType.LAB): "",
                               DetectorType.to_string(DetectorType.HAB): ""}

    def get_merge_strategy(self):
        return [ISISReductionMode.LAB, ISISReductionMode.HAB]

    def get_all_reduction_modes(self):
        return [ISISReductionMode.LAB, ISISReductionMode.HAB]

    def get_detector_name_for_reduction_mode(self, reduction_mode):
        if reduction_mode is ISISReductionMode.LAB:
            bank_type = DetectorType.to_string(DetectorType.LAB)
        elif reduction_mode is ISISReductionMode.HAB:
            bank_type = DetectorType.to_string(DetectorType.HAB)
        else:
            raise RuntimeError("SANStateReductionISIS: There is no detector available for the"
                               " reduction mode {0}.".format(reduction_mode))
        return self.detector_names[bank_type]

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def setup_detectors_from_ipf(reduction_info, data_info):
    ipf_file_path = data_info.ipf_file_path

    detector_names = {DetectorType.to_string(DetectorType.LAB): "low-angle-detector-name",
                      DetectorType.to_string(DetectorType.HAB): "high-angle-detector-name"}

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
        super(StateReductionModeBuilder, self).__init__()
        self.state = StateReductionMode()
        setup_detectors_from_ipf(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_reduction_mode_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateReductionModeBuilder(data_info)
    else:
        raise NotImplementedError("StateReductionBuilder: Could not find any valid reduction builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
