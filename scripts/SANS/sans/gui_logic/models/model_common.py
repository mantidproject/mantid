# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
""" The settings diagnostic tab which visualizes the SANS state object. """
from abc import ABCMeta
from sans.common.enums import SANSInstrument
from sans.state.AllStates import AllStates


class ModelCommon(metaclass=ABCMeta):
    def __init__(self, user_file_items : AllStates):
        # Workaround to avoid refactoring becoming impossibly large
        if user_file_items is None:
            # Cannot iterate a None type when doing lookups
            self._user_file_items = AllStates()
            self._instrument = SANSInstrument.NO_INSTRUMENT
        else:
            self._user_file_items = user_file_items
            self._instrument = user_file_items.data.instrument

    @property
    def instrument(self):
        return self._instrument

    @instrument.setter
    def instrument(self, value):
        assert isinstance(value, SANSInstrument)
        self._instrument = value
