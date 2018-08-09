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
        self.copyProperties('ReflectometryReductionOneAuto',[
            'InputWorkspace', 'SummationType', 'ReductionType','AnalysisMode',
            'ProcessingInstructions','ThetaIn', 'ThetaLogName','CorrectDetectors',
            'DetectorCorrectionType','WavelengthMin','WavelengthMax','I0MonitorIndex',
            'MonitorBackgroundWavelengthMin','MonitorBackgroundWavelengthMax',
            'MonitorIntegrationWavelengthMin','MonitorIntegrationWavelengthMax',
            'NormalizeByIntegratedMonitors','FirstTransmissionRun',
            'SecondTransmissionRun','Params','StartOverlap','EndOverlap',
            'StrictSpectrumChecking','CorrectionAlgorithm','Polynomial','C0','C1',
            'MomentumTransferMin','MomentumTransferStep','MomentumTransferMax',
            'PolarizationAnalysis','Pp','Ap','Rho','Alpha','OutputWorkspace','Debug',
            'IncludePartialBins'])

    def validateInputs(self):
        issues = {}
        return issues

    def copyPropertyValuesTo(self, alg):
        props = self.getProperties()
        for prop in props:
            value = self.getPropertyValue(prop.name)
            alg.setPropertyValue(prop.name, value)


    def setupSlits(self, ws, liveValues):
        s1 = liveValues['s1vg'].value
        s2 = liveValues['s2vg'].value
        SetInstrumentParameter(Workspace=ws,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit1',
                               Value=str(s1))
        SetInstrumentParameter(Workspace=ws,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit2',
                               Value=str(s2))

    def validateLiveValues(self, liveValues):
        for key in liveValues:
            if liveValues[key].value is None:
                raise RuntimeError('Required value ' + key + ' was not found for instrument')

        if float(liveValues['Theta'].value) <= 1e-06:
            raise RuntimeError('Theta must be greater than zero')

    def getLiveValueFromInstrument(self, logName, instrument):
        from epics import caget
        return caget('IN:' + instrument + ':CS:SB:' + logName, as_string=True)

    def getLiveValuesFromInstrument(self, instrument):
        # set up required values
        liveValues = {'Theta' : LiveValue(None, 'deg'),
                      's1vg' : LiveValue(None, 'm'),
                      's2vg' : LiveValue(None, 'm')}
        # get values from instrument
        for key in liveValues:
            if liveValues[key].value is None:
                liveValues[key].value = self.getLiveValueFromInstrument(key, instrument)
        # check we have all we need
        self.validateLiveValues(liveValues)
        return liveValues

    def setupInstrument(self):
        instrument = config.getInstrument().shortName()
        LoadInstrument(Workspace=self.out_ws_name,RewriteSpectraMap=True,InstrumentName=instrument)
        return instrument

    def setupSampleLogs(self, liveValues):
        logNames = [key for key in liveValues]
        logValues = [liveValues[key].value for key in liveValues]
        logUnits = [liveValues[key].unit for key in liveValues]
        AddSampleLogMultiple(Workspace=self.out_ws_name,LogNames=logNames,LogValues=logValues,
                             LogUnits=logUnits)

    def setupWorkspaceForReduction(self):
        in_ws_name = self.getProperty("InputWorkspace").value.getName()
        self.out_ws_name = self.getPropertyValue("OutputWorkspace")
        CloneWorkspace(InputWorkspace=in_ws_name,OutputWorkspace=self.out_ws_name)
        # The workspace must have an instrument.
        instrument = self.setupInstrument()
        # Set up the sample logs based on live values from the instrument.
        liveValues = self.getLiveValuesFromInstrument(instrument)
        self.setupSampleLogs(liveValues)
        # Set up instrument parameters for the slits
        self.setupSlits(self.out_ws_name, liveValues)

    def setupReductionAlgorithm(self):
        alg = AlgorithmManager.create("ReflectometryReductionOneAuto")
        alg.initialize()
        alg.setChild(False)
        self.copyPropertyValuesTo(alg)
        alg.setProperty("InputWorkspace", self.out_ws_name)
        alg.setProperty("ThetaLogName", "Theta")
        alg.setProperty("OutputWorkspaceBinned", self.out_ws_name)
        return alg

    def runReductionAlgorithm(self, alg):
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspaceBinned").value
        self.setProperty("OutputWorkspace", out_ws)

    def PyExec(self):
        self.setupWorkspaceForReduction()
        alg = self.setupReductionAlgorithm()
        self.runReductionAlgorithm(alg)

AlgorithmFactory.subscribe(ReflectometryReductionOneLiveData)
