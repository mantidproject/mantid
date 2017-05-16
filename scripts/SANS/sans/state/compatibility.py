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
from sans.common.enums import SANSInstrument


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateCompatibility(StateBase):
    use_compatibility_mode = BoolParameter()
    time_rebin_string = StringParameter()

    def __init__(self):
        super(StateCompatibility, self).__init__()
        self.use_compatibility_mode = False
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
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateCompatibilityBuilder()
    else:
        raise NotImplementedError("StateCompatibilityBuilder: Could not find any valid compatibility builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
