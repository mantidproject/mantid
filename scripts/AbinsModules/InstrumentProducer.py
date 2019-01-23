# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from AbinsModules.Instruments import ToscaInstrument
from AbinsModules import AbinsConstants


# noinspection PyMethodMayBeStatic
class InstrumentProducer(object):
    def __init__(self):
        pass

    def produce_instrument(self, name=None):

        if name not in AbinsConstants.ALL_INSTRUMENTS:
            raise ValueError("Unknown instrument: %s" % name)
        elif name == "TOSCA":
            return ToscaInstrument("TOSCA")
