# pylint: disable=invalid-name

""" Helpers for SANSCrop."""
from __future__ import (absolute_import, division, print_function)
from sans.common.general_functions import convert_instrument_and_detector_type_to_bank_name
from sans.common.enums import SANSInstrument


def get_component_name(workspace, detector_type):
    instrument = workspace.getInstrument()
    instrument_name = instrument.getName().strip()
    instrument_name = instrument_name.upper()
    instrument = SANSInstrument.from_string(instrument_name)
    return convert_instrument_and_detector_type_to_bank_name(instrument, detector_type)
