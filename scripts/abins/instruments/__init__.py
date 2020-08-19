# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# flake8: noqa
from abins.constants import ALL_INSTRUMENTS
from .lagrangeinstrument import LagrangeInstrument
from .toscainstrument import ToscaInstrument
from .twodmap import TwoDMap
from .instrument import Instrument

def get_instrument(name: str, setting: str = '') -> Instrument:
    """Instantiate a named Instrument

    Instruments inherit from abins.instruments.instrument.Instrument and
    implement models and relationships specific to the corresponding instrument
    geometry.

    Args:
        name: Instrument name as defined in abins.constants.ALL_INSTRUMENTS
        setting: Instrument setting as supported by instrument parameters

    Returns:
        Instrument object

    """
    if name not in ALL_INSTRUMENTS:
        raise ValueError("Unknown instrument: %s" % name)
    elif name == "Lagrange":
        return LagrangeInstrument("Lagrange", setting=setting)
    elif name == "TOSCA":
        return ToscaInstrument("TOSCA")
    elif name == "TwoDMap":
        return TwoDMap("TwoDMap")
