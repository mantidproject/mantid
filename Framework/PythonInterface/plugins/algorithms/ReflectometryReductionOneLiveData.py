from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class LiveValue():
    def __init__(self, value, unit):
        self.value = value
        self.unit = unit


class ReflectometryReductionOneLiveData(DataProcessorAlgorithm):
    def category(self):
        return 'Reflectometry'

    def summary(self):
        return 'Run the reflectometry reduction algorithm on live data'

    def seeAlso(self):
        return [ "ReflectometryReductionOneAuto", "StartLiveData" ]

    def PyInit(self):
        instruments = ['CRISP', 'INTER', 'OFFSPEC', 'POLREF', 'SURF']
        instrument = defaultInstrument if config.getInstrument() in instruments else ''
        self.declareProperty(name='Instrument', defaultValue=instrument, direction=Direction.Input,
                             validator=StringListValidator(instruments),
                             doc='Instrument to find live value for.')
        self.declareProperty(name='GetLiveValueAlgorithm', defaultValue='GetLiveInstrumentValue', direction=Direction.Input,
                             doc='The algorithm to use to get live values from the instrument')

        self._childProperties = [
            'InputWorkspace', 'SummationType', 'ReductionType','IncludePartialBins', 'AnalysisMode',
            'ProcessingInstructions','CorrectDetectors',
            'DetectorCorrectionType','WavelengthMin','WavelengthMax','I0MonitorIndex',
            'MonitorBackgroundWavelengthMin','MonitorBackgroundWavelengthMax',
            'MonitorIntegrationWavelengthMin','MonitorIntegrationWavelengthMax',
            'NormalizeByIntegratedMonitors','FirstTransmissionRun',
            'SecondTransmissionRun','Params','StartOverlap','EndOverlap',
            'StrictSpectrumChecking','CorrectionAlgorithm','Polynomial','C0','C1',
            'MomentumTransferMin','MomentumTransferStep','MomentumTransferMax',
            'PolarizationAnalysis','Pp','Ap','Rho','Alpha','Debug','OutputWorkspace']
        self.copyProperties('ReflectometryReductionOneAuto', self._childProperties)

    def validateInputs(self):
        issues = {}
        return issues

    def PyExec(self):
        self.setupWorkspaceForReduction()
        alg = self.setupReductionAlgorithm()
        self.runReductionAlgorithm(alg)

    def setupWorkspaceForReduction(self):
        """Set up the workspace ready for the reduction"""
        self.createWorkspaceForReduction()
        self.setupInstrument()
        liveValues = self.getLiveValuesFromInstrument()
        self.setupSampleLogs(liveValues)
        self.setupSlits(liveValues)

    def setupReductionAlgorithm(self):
        """Set up the reduction algorithm"""
        alg = AlgorithmManager.create("ReflectometryReductionOneAuto")
        alg.initialize()
        alg.setChild(True)
        self.copyPropertyValuesTo(alg)
        alg.setProperty("InputWorkspace", self.ws_name)
        alg.setProperty("ThetaLogName", "Theta")
        alg.setProperty("OutputWorkspaceBinned", self.ws_name)
        return alg

    def runReductionAlgorithm(self, alg):
        """Run the reduction"""
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspaceBinned").value
        self.setProperty("OutputWorkspace", out_ws)

    def createWorkspaceForReduction(self):
        """Create a workspace for the input/output to the reduction algorithm"""
        in_ws_name = self.getProperty("InputWorkspace").value.getName()
        self.ws_name = self.getPropertyValue("OutputWorkspace")
        CloneWorkspace(InputWorkspace=in_ws_name,OutputWorkspace=self.ws_name)

    def setupInstrument(self):
        """Sets the instrument name and loads the instrument on the workspace"""
        self.instrument = self.getProperty('Instrument').value
        LoadInstrument(Workspace=self.ws_name,RewriteSpectraMap=True,InstrumentName=self.instrument)

    def setupSampleLogs(self, liveValues):
        """Set up the sample logs based on live values from the instrument"""
        logNames = [key for key in liveValues]
        logValues = [liveValues[key].value for key in liveValues]
        logUnits = [liveValues[key].unit for key in liveValues]
        AddSampleLogMultiple(Workspace=self.ws_name,LogNames=logNames,LogValues=logValues,
                             LogUnits=logUnits)

    def setupSlits(self, liveValues):
        """Set up instrument parameters for the slits"""
        s1 = liveValues[self.s1vgName()].value
        s2 = liveValues[self.s2vgName()].value
        SetInstrumentParameter(Workspace=self.ws_name,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit1',
                               Value=str(s1))
        SetInstrumentParameter(Workspace=self.ws_name,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit2',
                               Value=str(s2))

    def copyPropertyValuesTo(self, alg):
        for prop in self._childProperties:
            value = self.getPropertyValue(prop)
            alg.setPropertyValue(prop, value)

    def getLiveValuesFromInstrument(self):
        # get values from instrument
        liveValues = self.liveValueList()
        for key in liveValues:
            if liveValues[key].value is None:
                liveValues[key].value = self.getBlockValueFromInstrument(key)
        # check we have all we need
        self.validateLiveValues(liveValues)
        return liveValues

    def s1vgName(self):
        if self.instrument == 'INTER' or self.instrument == 'SURF':
            return 'S1VG'
        elif self.instrument == 'OFFSPEC':
            return 's1vgap'
        else:
            return 's1vg'

    def s2vgName(self):
        if self.instrument == 'INTER' or self.instrument == 'SURF':
            return 'S2VG'
        elif self.instrument == 'OFFSPEC':
            return 's2vgap'
        else:
            return 's2vg'

    def getDoubleOrNone(self, propertyName):
        value = self.getProperty(propertyName)
        if value == Property.EMPTY_DBL:
            return None
        return value.value

    def liveValueList(self):
        """Get the list of required live value names and their unit type"""
        liveValues =  {'Theta' : LiveValue(None, 'deg'),
                       self.s1vgName() : LiveValue(None, 'm'),
                       self.s2vgName() : LiveValue(None, 'm')}
        return liveValues

    def getBlockValueFromInstrument(self, logName):
        algName = self.getProperty('GetLiveValueAlgorithm').value
        alg = self.createChildAlgorithm(algName)
        alg.setProperty('Instrument', self.instrument)
        alg.setProperty('PropertyType', 'Block')
        alg.setProperty('PropertyName', logName)
        alg.execute()
        return alg.getProperty("Value").value

    def validateLiveValues(self, liveValues):
        for key in liveValues:
            if liveValues[key].value is None:
                raise RuntimeError('Required value ' + key + ' was not found for instrument')
        if float(liveValues['Theta'].value) <= 1e-06:
            raise RuntimeError('Theta must be greater than zero')

AlgorithmFactory.subscribe(ReflectometryReductionOneLiveData)
