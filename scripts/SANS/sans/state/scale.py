""" Defines the state of the geometry and unit scaling."""

from __future__ import (absolute_import, division, print_function)
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, PositiveFloatParameter, ClassTypeParameter)
from sans.common.enums import (SampleShape, SANSInstrument)
from sans.common.file_information import (SANSFileInformationFactory)
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

    # Geometry from the file
    shape_from_file = ClassTypeParameter(SampleShape)
    thickness_from_file = PositiveFloatParameter()
    width_from_file = PositiveFloatParameter()
    height_from_file = PositiveFloatParameter()

    def __init__(self):
        super(StateScale, self).__init__()

        # The default geometry
        self.shape_from_file = SampleShape.CylinderAxisAlong

        # The default values are 1mm
        self.thickness_from_file = 1.
        self.width_from_file = 1.
        self.height_from_file = 1.

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_geometry_from_file(state, date_info):
    sample_scatter = date_info.sample_scatter
    file_information_factory = SANSFileInformationFactory()
    file_information = file_information_factory.create_sans_file_information(sample_scatter)

    # Get the geometry
    state.height_from_file = file_information.get_height()
    state.width_from_file = file_information.get_width()
    state.thickness_from_file = file_information.get_thickness()
    state.shape_from_file = file_information.get_shape()


class StateScaleBuilder(object):
    @automatic_setters(StateScale, exclusions=[])
    def __init__(self, data_info):
        super(StateScaleBuilder, self).__init__()
        self.state = StateScale()
        set_geometry_from_file(self.state, data_info)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# ---------------------------------------
# Factory method for SANStateScaleBuilder
# ---------------------------------------
def get_scale_builder(data_info):
    # The data state has most of the information that we require to define the move. For the factory method, only
    # the instrument is of relevance.
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or instrument is SANSInstrument.SANS2D:
        return StateScaleBuilder(data_info)
    else:
        raise NotImplementedError("StateScaleBuilder: Could not find any valid scale builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
