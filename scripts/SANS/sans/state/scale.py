""" Defines the state of the geometry and unit scaling."""
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, PositiveFloatParameter, ClassTypeParameter)
from sans.common.enums import (SampleShape, SANSInstrument)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
#  State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateScale(StateBase):
    shape = ClassTypeParameter(SampleShape)
    thickness = PositiveFloatParameter()
    width = PositiveFloatParameter()
    height = PositiveFloatParameter()
    scale = PositiveFloatParameter()

    def __init__(self):
        super(StateScale, self).__init__()

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateScaleBuilder(object):
    @automatic_setters(StateScale, exclusions=[])
    def __init__(self):
        super(StateScaleBuilder, self).__init__()
        self.state = StateScale()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ------------------------------------------
# Factory method for SANStateScaleBuilder
# ------------------------------------------
def get_scale_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateScaleBuilder()
    else:
        raise NotImplementedError("StateScaleBuilder: Could not find any valid scale builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
