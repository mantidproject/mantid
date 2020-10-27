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
from .pychop import PyChopInstrument
from .instrument import Instrument

instruments = {"lagrange": LagrangeInstrument,
               "tosca": ToscaInstrument,
               "twodmap": TwoDMap,
               "maps": (PyChopInstrument, {'name': 'MAPS'}),
               "mari": (PyChopInstrument, {'name': 'MARI'})}

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
    if name.lower() in instruments:
        instrument_factory = instruments.get(name.lower())
        if isinstance(instrument_factory, tuple):
            return instrument_factory[0](setting=setting,
                                         **instrument_factory[1])
        else:
            return instrument_factory(setting=setting)

    elif name not in ALL_INSTRUMENTS:
        raise ValueError(f'Unknown instrument: "{name}". Known instruments: '
                         + ', '.join(ALL_INSTRUMENTS))
    else:
        raise NotImplementedError(
            f"Instrument {name} is defined in abins.constants, but was not "
            "accessible from abins.instruments.get_instrument(). "
            "Please report this error to the Mantid team.")
