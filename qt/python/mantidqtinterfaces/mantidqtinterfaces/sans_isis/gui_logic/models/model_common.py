# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The settings diagnostic tab which visualizes the SANS state object."""

from abc import ABCMeta
from sans.common.enums import SANSInstrument
from sans.state.AllStates import AllStates


class ModelCommon(metaclass=ABCMeta):
    def __init__(self, all_states: AllStates):
        if all_states is None:
            self._all_states = AllStates()
            self._instrument = SANSInstrument.NO_INSTRUMENT
        else:
            self._all_states = all_states
            self._instrument = all_states.data.instrument

    @property
    def instrument(self):
        return self._instrument

    @instrument.setter
    def instrument(self, value):
        assert isinstance(value, SANSInstrument)
        self._instrument = value

    @staticmethod
    def _get_val_or_default(val, default_val=""):
        # These are Falsey in Python but are acceptable
        good_vals = [0.0, 0, False, [0]]

        if not val and val not in good_vals:
            return default_val
        return val
