# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abins.constants import ALL_INSTRUMENTS
from abins.instruments import ToscaInstrument


class InstrumentProducer(object):
    def __init__(self):
        pass

    @staticmethod
    def produce_instrument(name=None):
        if name not in ALL_INSTRUMENTS:
            raise ValueError("Unknown instrument: %s" % name)
        elif name == "TOSCA":
            return ToscaInstrument("TOSCA")
