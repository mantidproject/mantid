# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""Helpers for CropToComponent for SANS."""

from SANS.sans.common.general_functions import convert_instrument_and_detector_type_to_bank_name
from SANS.sans.common.enums import SANSInstrument

# TODO move this into common and move to instrument 2.0


def get_component_name(workspace, detector_type):
    instrument = workspace.getInstrument()
    instrument_name = instrument.getName().strip()
    instrument_name = instrument_name.upper()
    instrument = SANSInstrument[instrument_name]
    return convert_instrument_and_detector_type_to_bank_name(instrument, detector_type)
