# pylint: disable=too-few-public-methods

""" Defines the state of the reduction."""

from abc import (ABCMeta, abstractmethod)

from SANS2.State.SANSStateBase import (SANSStateBase, ClassTypeParameter, FloatParameter, DictParameter,
                                       FloatWithNoneParameter, sans_parameters)
from SANS2.Common.SANSEnumerations import (ReductionMode, ISISReductionMode, ReductionDimensionality, FitModeForMerge)
from SANS2.Common.SANSConstants import SANSConstants


# ------------------------------------------------
# SANSStateReduction
# ------------------------------------------------
class SANSStateReduction(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def get_merge_strategy(self):
        pass

    @abstractmethod
    def get_detector_name_for_reduction_mode(self, reduction_mode):
        pass

    @abstractmethod
    def get_all_reduction_modes(self):
        pass


# -----------------------------------------------
#  SANSStateReduction for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateReductionISIS(SANSStateReduction, SANSStateBase):
    reduction_mode = ClassTypeParameter(ReductionMode)
    dimensionality = ClassTypeParameter(ReductionDimensionality)

    # Fitting
    merge_fit_mode = ClassTypeParameter(FitModeForMerge)
    merge_shift = FloatParameter()
    merge_scale = FloatParameter()
    merge_range_min = FloatWithNoneParameter()
    merge_range_max = FloatWithNoneParameter()

    # Map from detector type to detector name
    detector_names = DictParameter()

    def __init__(self):
        super(SANSStateReductionISIS, self).__init__()
        self.reduction_mode = ISISReductionMode.Lab
        self.dimensionality = ReductionDimensionality.OneDim

        # Set the shifts to defaults which essentially don't do anything.
        self.merge_shift = 0.0
        self.merge_scale = 1.0
        self.merge_fit_mode = FitModeForMerge.None
        self.merge_range_min = None
        self.merge_range_max = None

        # Set the detector names to empty strings
        self.detector_names = {SANSConstants.low_angle_bank: "",
                               SANSConstants.high_angle_bank: ""}

    def get_merge_strategy(self):
        return [ISISReductionMode.Lab, ISISReductionMode.Hab]

    def get_all_reduction_modes(self):
        return [ISISReductionMode.Lab, ISISReductionMode.Hab]

    def get_detector_name_for_reduction_mode(self, reduction_mode):
        if reduction_mode is ISISReductionMode.Lab:
            bank_type = SANSConstants.low_angle_bank
        elif reduction_mode is ISISReductionMode.Hab:
            bank_type = SANSConstants.high_angle_bank
        else:
            raise RuntimeError("SANStateReductionISIS: There is no detector available for the"
                               " reduction mode {0}.".format(reduction_mode))
        return self.detector_names[bank_type]

    def validate(self):
        pass


# -----------------------------------------------
# SANSStateReduction setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateReduction and SANSStateBase and fulfill its contract.
# -----------------------------------------------
