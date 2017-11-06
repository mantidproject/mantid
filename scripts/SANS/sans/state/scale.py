""" Defines the state of the geometry and unit scaling."""

from __future__ import (absolute_import, division, print_function)
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, PositiveFloatParameter, ClassTypeParameter)
from sans.common.enums import (SampleShape, SANSFacility)
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


def ensure_all_same(property_name, items):
    try:
        first = next(items)
        if not all(x == first for x in items):
            raise RuntimeError('All file informations in a single reduction must have the same {}'\
                .format(property_name))
    except StopIteration:
        return

# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_geometry_from_file(state, data_info):
    file_informations = data_info.file_information()
    first_file_information = file_informations[0]

    ensure_all_same('height', (file_information.get_height() for file_information in file_informations))
    state.height_from_file = first_file_information.get_height()

    ensure_all_same('width', (file_information.get_width() for file_information in file_informations))
    state.width_from_file = first_file_information.get_width()

    ensure_all_same('thickness', (file_information.get_thickness() for file_information in file_informations))
    state.thickness_from_file = first_file_information.get_thickness()

    ensure_all_same('shape', (file_information.get_shape() for file_information in file_informations))
    state.shape_from_file = first_file_information.get_shape()


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
    # The data state has most of the information that we require to define the scaling. For the factory method, only
    # the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateScaleBuilder(data_info)
    else:
        raise NotImplementedError("StateScaleBuilder: Could not find any valid scale builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
