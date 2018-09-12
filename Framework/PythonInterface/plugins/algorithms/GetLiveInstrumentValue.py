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
        self.declareProperty(name='Instrument', defaultValue='', direction=Direction.Input,
                             validator=StringListValidator(['CRISP', 'INTER', 'OFFSPEC', 'POLREF', 'SURF']),
                             doc='Instrument to find live value for.')

        self.declareProperty(name='LogName', defaultValue='', direction=Direction.Input,
                             doc='Log name of value to find.')

        self.declareProperty(name='OutputValue', defaultValue=Property.EMPTY_DBL, direction=Direction.Output,
                             doc='The live value from the instrument, or an empty string if not found')

    def validateInputs(self):
        issues = {}
        return issues

    def PyExec(self):
        instrument = self.getProperty('Instrument').value
        logName = self.getProperty('LogName').value
        if logName == '':
            raise ValueError('LogName was not specified')
        value = self.getLiveValueFromInstrument(instrument, logName)
        if value is not None:
            self.log().notice(logName + ' = ' + value)
            self.setProperty('OutputValue', value)
        else:
            self.log().notice(logName + ' not found')

    def getLiveValueFromInstrument(self, instrument, logName):
        from epics import caget
        return caget('IN:' + instrument + ':CS:SB:' + logName, as_string=True)

AlgorithmFactory.subscribe(GetLiveInstrumentValue)
