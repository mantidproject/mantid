# pylint: disable=too-few-public-methods

"""State about the actual data which is to be reduced."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, StringParameter, PositiveIntegerParameter,
                                       ClassTypeParameter, sans_parameters)
from SANS2.Common.SANSEnumerations import SANSInstrument


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateData(object):
    ALL_PERIODS = 0


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
            is_invalid.update({"sample_scatter": "The sample scatter workspace must be set."})

        # If the sample transmission/direct was specified, then a sample direct/transmission is required
        if (self.sample_transmission and not self.sample_direct) or \
                (not self.sample_transmission and self.sample_direct):
            is_invalid.update({"sample_transmission": "The sample transmission/direct was specified, "
                                                      "then the direct/transmission must be specified as well."})

        # If the can transmission/direct was specified, then this requires the can scatter
        if (self.can_direct or self.can_transmission) and (not self.can_scatter):
            is_invalid.update({"can_scatter": "The can transmission/direct was specified, then the "
                                              "scan scatter must be specified as well."})

        # If a can transmission/direct was specified, then the other can entries need to be specified as well.
        if self.can_scatter and (self.can_transmission and not self.can_direct) or \
                (not self.can_transmission and self.can_direct):
            is_invalid.update({"can_transmission": "The can transmission/direct was specified, then the "
                                                   "direct/transmission must be specified as well."})

        if is_invalid:
            raise ValueError("SANSStateData: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))

# -----------------------------------------------
# SANSStateData setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateData and SANSStateBase and fulfill its contract.
# -----------------------------------------------
