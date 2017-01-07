# pylint: disable=too-few-public-methods

""" Defines the state of saving."""
import copy
from sans.state.state_base import (StateBase, BoolParameter, StringParameter,
                                   ClassTypeListParameter, rename_descriptor_names)
from sans.common.enums import (SaveType, SANSInstrument)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateSave(StateBase):
    file_name = StringParameter()
    zero_free_correction = BoolParameter()
    file_format = ClassTypeListParameter(SaveType)

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
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateSaveBuilder()
    else:
        raise NotImplementedError("StateSaveBuilder: Could not find any valid save builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
