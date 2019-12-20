# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Defines the state of the geometry and unit scaling."""

from __future__ import (absolute_import, division, print_function)
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, PositiveFloatParameter)
from sans.common.enums import (SampleShape, SANSFacility)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
#  State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateScale(StateBase):
    shape = None

    thickness = PositiveFloatParameter()
    width = PositiveFloatParameter()
    height = PositiveFloatParameter()
    scale = PositiveFloatParameter()

    # Geometry from the file
    shape_from_file = SampleShape.DISC
    thickness_from_file = PositiveFloatParameter()
    width_from_file = PositiveFloatParameter()
    height_from_file = PositiveFloatParameter()

    def __init__(self):
        super(StateScale, self).__init__()
        # The default values are 1mm
        self.thickness_from_file = 1.
        self.width_from_file = 1.
        self.height_from_file = 1.

    def validate(self):
        pass


# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_geometry_from_file(state, file_information):
    # Get the geometry
    state.height_from_file = file_information.get_height()
    state.width_from_file = file_information.get_width()
    state.thickness_from_file = file_information.get_thickness()
    state.shape_from_file = file_information.get_shape()


class StateScaleBuilder(object):
    @automatic_setters(StateScale, exclusions=[])
    def __init__(self, file_information):
        super(StateScaleBuilder, self).__init__()
        self.state = StateScale()
        set_geometry_from_file(self.state, file_information)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def set_shape(self, val):
        self.state.shape = val

    def set_shape_from_file(self, val):
        self.state.shape_from_file(val)


# ---------------------------------------
# Factory method for SANStateScaleBuilder
# ---------------------------------------
def get_scale_builder(data_info, file_information=None):
    # The data state has most of the information that we require to define the scaling. For the factory method, only
    # the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateScaleBuilder(file_information)
    else:
        raise NotImplementedError("StateScaleBuilder: Could not find any valid scale builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
