# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State about the actual data which is to be reduced."""
from __future__ import (absolute_import, division, print_function)
import json
import copy

from sans.state.state_base import (StateBase, StringParameter, PositiveIntegerParameter, BoolParameter,
                                   EnumParameter, rename_descriptor_names)
from sans.common.enums import (SANSInstrument, SANSFacility)
import sans.common.constants
from sans.state.state_functions import (is_pure_none_or_not_none, validation_message)
from sans.state.automatic_setters import automatic_setters


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateData(StateBase):
    ALL_PERIODS = sans.common.constants.ALL_PERIODS
    sample_scatter = StringParameter()
    sample_scatter_period = PositiveIntegerParameter()
    sample_transmission = StringParameter()
    sample_transmission_period = PositiveIntegerParameter()
    sample_direct = StringParameter()
    sample_direct_period = PositiveIntegerParameter()

    can_scatter = StringParameter()
    can_scatter_period = PositiveIntegerParameter()
    can_transmission = StringParameter()
    can_transmission_period = PositiveIntegerParameter()
    can_direct = StringParameter()
    can_direct_period = PositiveIntegerParameter()

    calibration = StringParameter()

    sample_scatter_run_number = PositiveIntegerParameter()
    sample_scatter_is_multi_period = BoolParameter()
    instrument = EnumParameter(SANSInstrument)
    facility = EnumParameter(SANSFacility)
    idf_file_path = StringParameter()
    ipf_file_path = StringParameter()

    user_file = StringParameter()

    def __init__(self):
        super(StateData, self).__init__()

        # Setup default values for periods
        self.sample_scatter_period = StateData.ALL_PERIODS
        self.sample_transmission_period = StateData.ALL_PERIODS
        self.sample_direct_period = StateData.ALL_PERIODS

        self.can_scatter_period = StateData.ALL_PERIODS
        self.can_transmission_period = StateData.ALL_PERIODS
        self.can_direct_period = StateData.ALL_PERIODS

        # This should be reset by the builder. Setting this to NoInstrument ensure that we will trip early on,
        # in case this is not set, for example by not using the builders.
        self.instrument = SANSInstrument.NoInstrument
        self.facility = SANSFacility.NoFacility
        self.user_file = ""

    def validate(self):
        is_invalid = dict()

        # A sample scatter must be specified
        if not self.sample_scatter:
            entry = validation_message("Sample scatter was not specified.",
                                       "Make sure that the sample scatter file is specified.",
                                       {"sample_scatter": self.sample_scatter})
            is_invalid.update(entry)

        # If the sample transmission/direct was specified, then a sample direct/transmission is required
        if not is_pure_none_or_not_none([self.sample_transmission, self.sample_direct]):
            entry = validation_message("If the sample transmission is specified then, the direct run needs to be "
                                       "specified too.",
                                       "Make sure that the transmission and direct runs are both specified (or none).",
                                       {"sample_transmission": self.sample_transmission,
                                        "sample_direct": self.sample_direct})
            is_invalid.update(entry)

        # If the can transmission/direct was specified, then this requires the can scatter
        if (self.can_direct or self.can_transmission) and (not self.can_scatter):
            entry = validation_message("If the can transmission is specified then the can scatter run needs to be "
                                       "specified too.",
                                       "Make sure that the can scatter file is set.",
                                       {"can_scatter": self.can_scatter,
                                        "can_transmission": self.can_transmission,
                                        "can_direct": self.can_direct})
            is_invalid.update(entry)

        # If a can transmission/direct was specified, then the other can entries need to be specified as well.
        if self.can_scatter and not is_pure_none_or_not_none([self.can_transmission, self.can_direct]):
            entry = validation_message("Inconsistent can transmission setting.",
                                       "Make sure that the can transmission and can direct runs are set (or none of"
                                       " them).",
                                       {"can_transmission": self.can_transmission,
                                        "can_direct": self.can_direct})
            is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateData: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
def set_information_from_file(data_info, file_information, user_file):
    instrument = file_information.get_instrument()
    facility = file_information.get_facility()
    run_number = file_information.get_run_number()
    data_info.instrument = instrument
    data_info.facility = facility
    data_info.sample_scatter_run_number = run_number
    data_info.sample_scatter_is_multi_period = file_information.get_number_of_periods() > 1
    data_info.idf_file_path = file_information.get_idf_file_path()
    data_info.ipf_file_path = file_information.get_ipf_file_path()
    data_info.user_file = user_file


class StateDataBuilder(object):
    @automatic_setters(StateData)
    def __init__(self, file_information, user_file):
        super(StateDataBuilder, self).__init__()
        self.state = StateData()
        self._file_information = file_information
        self._user_file = user_file

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()

        # There are some elements which need to be read from the file information object.
        #  This is currently:
        # 1. instrument
        # 2. sample_scatter_run_number
        set_information_from_file(self.state, self._file_information, self._user_file)

        return copy.copy(self.state)


# ------------------------------------------
# Factory method for StateDataBuilder
# ------------------------------------------
def get_data_builder(facility, file_information=None, user_file=""):
    if facility is SANSFacility.ISIS:
        return StateDataBuilder(file_information, user_file)
    else:
        raise NotImplementedError("StateDataBuilder: The selected facility {0} does not seem"
                                  " to exist".format(str(facility)))
