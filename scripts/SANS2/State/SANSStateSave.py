# pylint: disable=too-few-public-methods

""" Defines the state of saving."""

from abc import (ABCMeta, abstractmethod)

from SANS2.State.SANSStateBase import (SANSStateBase, BoolParameter, StringParameter,
                                       ClassTypeListParameter, sans_parameters)
from SANS2.Common.SANSEnumerations import SaveType


# ------------------------------------------------
# SANSStateSave
# ------------------------------------------------
class SANSStateSave(object):
    pass


# -----------------------------------------------
#  SANSStateSave for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateSaveISIS(SANSStateSave, SANSStateBase):
    file_name = StringParameter()
    zero_free_correction = BoolParameter()
    file_format = ClassTypeListParameter(SaveType)

    def __init__(self):
        super(SANSStateSaveISIS, self).__init__()
        self.zero_free_correction = True

    def validate(self):
        pass


# -----------------------------------------------
# SANSStateReduction setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateReduction and SANSStateBase and fulfill its contract.
# -----------------------------------------------
