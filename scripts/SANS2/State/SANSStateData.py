# pylint: disable=too-few-public-methods

"""State about the actual data which is to be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, StringParameter, PositiveIntegerParameter,
                                       ClassTypeParameter, sans_parameters)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.State.SANSStateFunctions import (is_pure_none_or_not_none, is_not_none_and_first_larger_than_second,
                                            one_is_none, validation_message)


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateData(object):
    ALL_PERIODS = SANSConstants.ALL_PERIODS


# -----------------------------------------------
#  SANSStateData Setup for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateDataISIS(SANSStateBase, SANSStateData):
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
    instrument = ClassTypeParameter(SANSInstrument)

    def __init__(self):
        super(SANSStateDataISIS, self).__init__()

        # Setup default values for periods
        self.sample_scatter_period = SANSStateData.ALL_PERIODS
        self.sample_transmission_period = SANSStateData.ALL_PERIODS
        self.sample_direct_period = SANSStateData.ALL_PERIODS

        self.can_scatter_period = SANSStateData.ALL_PERIODS
        self.can_transmission_period = SANSStateData.ALL_PERIODS
        self.can_direct_period = SANSStateData.ALL_PERIODS

        # This should be reset by the builder. Setting this to NoInstrument ensure that we will trip early on,
        # in case this is not set, for example by not using the builders.
        self.instrument = SANSInstrument.NoInstrument

    def validate(self):
        is_invalid = dict()

        # A sample scatter must be specified
        if self.sample_scatter is None:
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
            raise ValueError("SANSStateData: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))

# -----------------------------------------------
# SANSStateData setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateData and SANSStateBase and fulfill its contract.
# -----------------------------------------------
