# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" Defines the state of saving."""

from __future__ import (absolute_import, division, print_function)
import copy
from sans.state.state_base import (StateBase, BoolParameter, StringParameter, StringWithNoneParameter,
                                   EnumListParameter, rename_descriptor_names)
from sans.common.enums import (SaveType, SANSFacility)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateSave(StateBase):
    zero_free_correction = BoolParameter()
    file_format = EnumListParameter(SaveType)

    # Settings for the output name
    user_specified_output_name = StringWithNoneParameter()
    user_specified_output_name_suffix = StringParameter()
    use_reduction_mode_as_suffix = BoolParameter()

    def __init__(self):
        super(StateSave, self).__init__()
        self.zero_free_correction = True

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateSaveBuilder(object):
    @automatic_setters(StateSave)
    def __init__(self):
        super(StateSaveBuilder, self).__init__()
        self.state = StateSave()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_save_builder(data_info):
    # The data state has most of the information that we require to define the save. For the factory method, only
    # the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateSaveBuilder()
    else:
        raise NotImplementedError("StateSaveBuilder: Could not find any valid save builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
