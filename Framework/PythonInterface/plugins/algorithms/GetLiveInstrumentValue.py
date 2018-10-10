# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class GetLiveInstrumentValue(DataProcessorAlgorithm):
    def category(self):
        return 'Utility'

    def summary(self):
        return 'Get a live data value from and instrument via EPICS'

    def seeAlso(self):
        return [ "ReflectometryReductionOneLiveData" ]

    def PyInit(self):
        instruments = sorted([item.name()
                              for item in config.getFacility().instruments()])
        instrument = config.getInstrument() if config.getInstrument() in instruments else ''
        self.declareProperty(name='Instrument', defaultValue=instrument, direction=Direction.Input,
                             validator=StringListValidator(instruments),
                             doc='Instrument to find live value for.')

        self.declareProperty(name='PropertyType', defaultValue='Run', direction=Direction.Input,
                             validator=StringListValidator(['Run', 'Block']),
                             doc='The type of property to find')

        self.declareProperty(name='PropertyName', defaultValue='TITLE', direction=Direction.Input,
                             validator=StringMandatoryValidator(),
                             doc='Name of value to find.')

        self.declareProperty(name='Value', defaultValue='', direction=Direction.Output,
                             doc='The live value from the instrument, or an empty string if not found')

    def PyExec(self):
        self._instrument = self.getProperty('Instrument').value
        self._propertyType = self.getProperty('PropertyType').value
        self._propertyName = self.getProperty('PropertyName').value
        value = self._get_live_value()
        self._set_output_value(value)

    def _prefix(self):
        """Prefix to use at the start of the EPICS string"""
        return 'IN:'

    def _name_prefix(self):
        """Prefix to use in the EPICS string before the property name"""
        if self._propertyType == 'Run':
            return ':DAE:'
        else:
            return ':CS:SB:'

    def _get_live_value(self):
        try:
            from epics import caget
        except ImportError:
            raise RuntimeError("PyEpics must be installed to use this algorithm. "
                               "For details, see https://www.mantidproject.org/PyEpics_In_Mantid")
        epicsName = self._prefix() + self._instrument + self._name_prefix() + self._propertyName
        return caget(epicsName, as_string=True)

    def _set_output_value(self, value):
        if value is not None:
            self.log().notice(self._propertyName + ' = ' + value)
            self.setProperty('Value', str(value))
        else:
            self.log().notice(self._propertyName + ' not found')

AlgorithmFactory.subscribe(GetLiveInstrumentValue)
