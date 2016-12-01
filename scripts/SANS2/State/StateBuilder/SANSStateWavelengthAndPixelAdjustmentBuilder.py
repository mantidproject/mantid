# pylint: disable=too-few-public-methods

import copy

from SANS2.State.SANSStateWavelengthAndPixelAdjustment import (SANSStateWavelengthAndPixelAdjustmentISIS)
from SANS2.Common.SANSType import SANSInstrument
from SANS2.State.StateBuilder.AutomaticSetters import (automatic_setters)


# ---------------------------------------
# State builders
# ---------------------------------------
class SANSStateWavelengthAndPixelAdjustmentBuilderISIS(object):
    @automatic_setters(SANSStateWavelengthAndPixelAdjustmentISIS)
    def __init__(self):
        super(SANSStateWavelengthAndPixelAdjustmentBuilderISIS, self).__init__()
        self.state = SANSStateWavelengthAndPixelAdjustmentISIS()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


# -----------------------------------------------------
# Factory method for SANSStateNormalizeToMonitorBuilder
# -----------------------------------------------------
def get_wavelength_and_pixel_adjustment_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.SANS2D or instrument is SANSInstrument.LOQ:
        return SANSStateWavelengthAndPixelAdjustmentBuilderISIS()
    else:
        raise NotImplementedError("SANSStateWavelengthAndPixelAdjustmentBuilder: Could not find any valid "
                                  "wavelength and pixel adjustment builder for the specified "
                                  "SANSStateData object {0}".format(str(data_info)))
