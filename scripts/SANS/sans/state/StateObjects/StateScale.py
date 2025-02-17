# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the geometry and unit scaling."""

import copy

from sans.state.JsonSerializable import JsonSerializable
from sans.common.enums import SampleShape, SANSFacility


# ----------------------------------------------------------------------------------------------------------------------
#  State
# ----------------------------------------------------------------------------------------------------------------------
from sans.state.automatic_setters import automatic_setters


class StateScale(metaclass=JsonSerializable):
    def __init__(self):
        super(StateScale, self).__init__()
        self.shape = None

        self.thickness = None  # : Float (Positive)
        self.width = None  # : Float (Positive)
        self.height = None  # : Float (Positive)
        self.scale = None  # : Float (Positive)

        # Geometry from the file
        self.shape_from_file = SampleShape.DISC

        # The default values are 1mm
        self.thickness_from_file = 1.0  # : Float (Positive)
        self.width_from_file = 1.0  # : Float (Positive)
        self.height_from_file = 1.0  # : Float (Positive)

    def validate(self):
        pass

    def set_geometry_from_file(self, file_information):
        # Get the geometry
        self.height_from_file = file_information.get_height()
        self.width_from_file = file_information.get_width()
        self.thickness_from_file = file_information.get_thickness()
        self.shape_from_file = file_information.get_shape()


# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------


class StateScaleBuilder(object):
    @automatic_setters(StateScale)
    def __init__(self, file_information):
        super(StateScaleBuilder, self).__init__()
        self.state = StateScale()
        self.state.set_geometry_from_file(file_information)

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def set_shape(self, val):
        self.state.shape = val

    def set_shape_from_file(self, val):
        self.state.shape_from_file = SampleShape(val)


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
        raise NotImplementedError(
            "StateScaleBuilder: Could not find any valid scale builder for the specified StateData object {0}".format(str(data_info))
        )
