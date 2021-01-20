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
from .instrument import Instrument

instruments = {"lagrange": LagrangeInstrument,
               "tosca": ToscaInstrument}

def get_instrument(name: str, **kwargs) -> Instrument:
    """Instantiate a named Instrument

    Instruments inherit from abins.instruments.instrument.Instrument and
    implement models and relationships specific to the corresponding instrument
    geometry.

    Args:
        name: Instrument name as defined in abins.constants.ALL_INSTRUMENTS
        kwargs: remaining arguments are instrument parameters passed to 
            instrument class

    Returns:
        Instrument object

    """
    if name.lower() in instruments:
        instrument_factory = instruments.get(name.lower())
        if isinstance(instrument_factory, tuple):
            instrument_factory, extra_args = instrument_factory
            kwargs.update(extra_args)

        return instrument_factory(**kwargs)

    elif name not in ALL_INSTRUMENTS:
        raise ValueError(f'Unknown instrument: "{name}". Known instruments: '
                         + ', '.join(ALL_INSTRUMENTS))
    else:
        raise NotImplementedError(
            f"Instrument {name} is defined in abins.constants, but was not "
            "accessible from abins.instruments.get_instrument(). "
            "Please report this error to the Mantid team.")
