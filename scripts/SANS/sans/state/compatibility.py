# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State which governs the SANS compatibility mode. This is not part of the reduction itself and should be removed
   once the transition to the new reducer is satisfactory and complete. This feature allows users to have the
   two reduction approaches produce the exact same results. If the results are different then that is a hint
   that we are dealing with a bug
"""

from __future__ import (absolute_import, division, print_function)
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, BoolParameter, StringParameter)
from sans.state.automatic_setters import (automatic_setters)
from sans.common.enums import SANSFacility


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateCompatibility(StateBase):
    use_compatibility_mode = BoolParameter()
    time_rebin_string = StringParameter()
    use_event_slice_optimisation = BoolParameter()

    def __init__(self):
        super(StateCompatibility, self).__init__()
        self.use_compatibility_mode = False
        self.use_event_slice_optimisation = False
        self.time_rebin_string = ""

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateCompatibilityBuilder(object):
    @automatic_setters(StateCompatibility)
    def __init__(self):
        super(StateCompatibilityBuilder, self).__init__()
        self.state = StateCompatibility()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_compatibility_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateCompatibilityBuilder()
    else:
        raise NotImplementedError("StateCompatibilityBuilder: Could not find any valid compatibility builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
