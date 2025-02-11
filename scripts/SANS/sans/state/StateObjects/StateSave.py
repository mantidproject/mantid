# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""Defines the state of saving."""

import copy

from sans.state.JsonSerializable import JsonSerializable
from sans.common.enums import SaveType, SANSFacility
from sans.state.automatic_setters import automatic_setters


class StateSave(metaclass=JsonSerializable):
    def __init__(self):
        super(StateSave, self).__init__()
        self.zero_free_correction = True  # : Bool
        self.file_format: SaveType = None

        # Settings for the output name
        self.user_specified_output_name = None  # : Str
        self.user_specified_output_name_suffix = None  # : Str()
        self.use_reduction_mode_as_suffix = None  # : Bool

        # Settings for the main user file and batch file
        self.user_file = None  # : Str
        self.batch_file = None  # : Str

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

    def set_file_format(self, val):
        self.state.file_format = val


def get_save_builder(data_info):
    # The data state has most of the information that we require to define the save. For the factory method, only
    # the facility/instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateSaveBuilder()
    else:
        raise NotImplementedError(
            "StateSaveBuilder: Could not find any valid save builder for the specified StateData object {0}".format(str(data_info))
        )
