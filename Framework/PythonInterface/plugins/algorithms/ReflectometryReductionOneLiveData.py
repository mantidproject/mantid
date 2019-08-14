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
        return ["ReflectometryISISLoadAndProcess", "StartLiveData"]

    def PyInit(self):
        instruments = ['CRISP', 'INTER', 'OFFSPEC', 'POLREF', 'SURF']
        defaultInstrument = str(config.getInstrument())
        defaultInstrument = defaultInstrument if defaultInstrument in instruments else instruments[0]
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(name='Instrument', defaultValue=defaultInstrument, direction=Direction.Input,
                             validator=StringListValidator(instruments),
                             doc='Instrument to find live value for.')
        self.declareProperty(name='GetLiveValueAlgorithm', defaultValue='GetLiveInstrumentValue',
                             direction=Direction.Input,
                             doc='The algorithm to use to get live values from the instrument')

        self._child_properties = [
            'FirstTransmissionRunList','SecondTransmissionRunList',
            'SliceWorkspace', 'NumberOfSlices',
            'SummationType', 'ReductionType', 'IncludePartialBins',
            'AnalysisMode', 'ProcessingInstructions', 'CorrectDetectors',
            'DetectorCorrectionType', 'WavelengthMin', 'WavelengthMax', 'I0MonitorIndex',
            'MonitorBackgroundWavelengthMin', 'MonitorBackgroundWavelengthMax',
            'MonitorIntegrationWavelengthMin', 'MonitorIntegrationWavelengthMax',
            'NormalizeByIntegratedMonitors', 'Params', 'StartOverlap', 'EndOverlap',
            'ScaleRHSWorkspace', 'TransmissionProcessingInstructions',
            'CorrectionAlgorithm', 'Polynomial', 'C0', 'C1',
            'MomentumTransferMin', 'MomentumTransferStep', 'MomentumTransferMax',
            'ScaleFactor', 'PolarizationAnalysis',
            'FloodCorrection', 'FloodWorkspace', 'Debug',
            'OutputWorkspace']
        self.copyProperties('ReflectometryISISLoadAndProcess', self._child_properties)

    def PyExec(self):
        self._setup_workspace_for_reduction()
        alg = self._setup_reduction_algorithm()
        self._run_reduction_algorithm(alg)

    def _setup_workspace_for_reduction(self):
        """Set up the workspace ready for the reduction"""
        self._in_ws_name = self.getProperty("InputWorkspace").value.name()
        self._out_ws_name = self.getPropertyValue("OutputWorkspace")
        self._setup_instrument()
        liveValues = self._get_live_values_from_instrument()
        self._setup_sample_logs(liveValues)
        self._setup_slits(liveValues)

    def _setup_reduction_algorithm(self):
        """Set up the reduction algorithm"""
        alg = AlgorithmManager.create("ReflectometryISISLoadAndProcess")
        alg.initialize()
        alg.setChild(True)
        self._copy_property_values_to(alg)
        alg.setProperty("InputRunList", self._in_ws_name)
        alg.setProperty("ThetaLogName", "Theta")
        alg.setProperty("GroupTOFWorkspaces", False)
        alg.setProperty("ReloadInvalidWorkspaces", False)
        alg.setProperty("OutputWorkspaceBinned", self._out_ws_name)
        return alg

    def _run_reduction_algorithm(self, alg):
        """Run the reduction"""
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspaceBinned").value
        self.setProperty("OutputWorkspace", out_ws)

    def _setup_instrument(self):
        """Sets the instrument name and loads the instrument on the workspace"""
        self._instrument = self.getProperty('Instrument').value
        LoadInstrument(Workspace=self._in_ws_name, RewriteSpectraMap=True,
                       InstrumentName=self._instrument)

    def _setup_sample_logs(self, liveValues):
        """Set up the sample logs based on live values from the instrument"""
        logNames = [key for key in liveValues]
        logValues = [liveValues[key].value for key in liveValues]
        logUnits = [liveValues[key].unit for key in liveValues]
        AddSampleLogMultiple(Workspace=self._in_ws_name, LogNames=logNames,
                             LogValues=logValues, LogUnits=logUnits)

    def _setup_slits(self, liveValues):
        """Set up instrument parameters for the slits"""
        s1 = liveValues[self._s1vg_name()].value
        s2 = liveValues[self._s2vg_name()].value
        SetInstrumentParameter(Workspace=self._in_ws_name,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit1',
                               Value=str(s1))
        SetInstrumentParameter(Workspace=self._in_ws_name,
                               ParameterName='vertical gap',
                               ParameterType='Number',
                               ComponentName='slit2',
                               Value=str(s2))

    def _copy_property_values_to(self, alg):
        for prop in self._child_properties:
            value = self.getPropertyValue(prop)
            alg.setPropertyValue(prop, value)

    def _get_live_values_from_instrument(self):
        # get values from instrument
        liveValues = self._live_value_list()
        for key in liveValues:
            if liveValues[key].value is None:
                liveValues[key].value = self._get_block_value_from_instrument(key)
        # check we have all we need
        self._validate_live_values(liveValues)
        return liveValues

    def _s1vg_name(self):
        if self._instrument == 'INTER' or self._instrument == 'SURF':
            return 'S1VG'
        elif self._instrument == 'OFFSPEC':
            return 's1vgap'
        else:
            return 's1vg'

    def _s2vg_name(self):
        if self._instrument == 'INTER' or self._instrument == 'SURF':
            return 'S2VG'
        elif self._instrument == 'OFFSPEC':
            return 's2vgap'
        else:
            return 's2vg'

    def _get_double_or_none(self, propertyName):
        value = self.getProperty(propertyName)
        if value == Property.EMPTY_DBL:
            return None
        return value.value

    def _live_value_list(self):
        """Get the list of required live value names and their unit type"""
        liveValues = {'Theta': LiveValue(None, 'deg'),
                      self._s1vg_name(): LiveValue(None, 'm'),
                      self._s2vg_name(): LiveValue(None, 'm')}
        return liveValues

    def _get_block_value_from_instrument(self, logName):
        algName = self.getProperty('GetLiveValueAlgorithm').value
        alg = self.createChildAlgorithm(algName)
        alg.setProperty('Instrument', self._instrument)
        alg.setProperty('PropertyType', 'Block')
        alg.setProperty('PropertyName', logName)
        alg.execute()
        return alg.getProperty("Value").value

    def _validate_live_values(self, liveValues):
        for key in liveValues:
            if liveValues[key].value is None:
                raise RuntimeError('Required value ' + key + ' was not found for instrument')
        if float(liveValues['Theta'].value) <= 1e-06:
            raise RuntimeError('Theta must be greater than zero')

AlgorithmFactory.subscribe(ReflectometryReductionOneLiveData)
