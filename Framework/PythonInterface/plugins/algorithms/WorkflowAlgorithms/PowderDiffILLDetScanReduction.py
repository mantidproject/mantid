from __future__ import (absolute_import, division, print_function)

from mantid.kernel import StringListValidator, Direction, FloatArrayProperty, FloatArrayOrderedPairsValidator
from mantid.api import PythonAlgorithm, MultipleFileProperty, MatrixWorkspaceProperty
from mantid.simpleapi import *


class PowderDiffILLDetScanReduction(PythonAlgorithm):
    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def _hide_run(selfs, runnumber):
        return '__' + runnumber

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return 'Performs powder diffraction data reduction for D2B and D20 (when doing a detector scan).'

    def name(self):
        return "PowderDiffILLDetScanReduction"

    def validateInputs(self):
        issues = dict()

        height_range = self.getProperty('HeightRange').value
        if len(height_range) > 2:
            issues['HeightRange'] = 'HeightRange must contain a minimum and maximum only'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Monitor']),
                             doc='Normalise to monitor, or do not perform normalisation.')

        self.declareProperty(name='UsePreCalibratedData',
                             defaultValue=True,
                             doc='Whether or not to use the calibrated data in the NeXus files')

        self.declareProperty(name='Output2D',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against tube scattering angle.')

        self.declareProperty(name='Output2DStraight',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against the real scattering angle.')

        self.declareProperty(name='Output1D',
                             defaultValue=True,
                             doc='Whether or not to use the calibrated data in the NeXus files')

        self.declareProperty(FloatArrayProperty(name='HeightRange', values=[], validator=FloatArrayOrderedPairsValidator()),
                             doc='A comma separated list of minimum and maximum height range (in m)')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):

        inputWS = self._load()
        instrument_name = inputWS[0].getInstrument().getName()
        supported_instruments = ['D2B', 'D20']
        if instrument_name not in supported_instruments:
            self.log.warning('Running for unspported instrument, use with caution. Supported instruments are: ' + str(supported_instruments))

        if self.getPropertyValue('NormaliseTo') == 'Monitor':
            inputWS = NormaliseToMonitor(InputWorkspace=inputWS, MonitorID=0)

        height_range = ''
        height_range_prop = self.getProperty('HeightRange').value
        if (len(height_range_prop) == 2):
            height_range = str(height_range_prop[0]) + ', ' + str(height_range_prop[1])

        output_workspaces = []
        if self.getPropertyValue('Output2D'):
            output2D = SumOverlappingTubes(InputWorkspaces=inputWS,
                                           OutputType='2D',
                                           HeightAxis=height_range)
            output_workspaces.append(output2D)
        if self.getPropertyValue('Output2DStraight'):
            output2Dstraight = SumOverlappingTubes(InputWorkspaces=inputWS,
                                                   OutputType='2DStraight',
                                                   HeightAxis=height_range)
            output_workspaces.append(output2Dstraight)
        if self.getPropertyValue('Output1D'):
            output1D = SumOverlappingTubes(InputWorkspaces=inputWS,
                                           OutputType='1DStraight',
                                           HeightAxis=height_range)
            output_workspaces.append(output1D)

        grouped_output_workspace = GroupWorkspaces(output_workspaces)
        self.setProperty('OutputWorkspace', grouped_output_workspace)

    def _load(self):
        """
            Loads the list of runs
            If sum is requested, MergeRuns is called
            @return : the list of the loaded ws names
        """
        runs = self.getPropertyValue('Run')
        data_type = 'Raw'
        if self.getPropertyValue('UsePreCalibratedData'):
            data_type = 'Calibrated'

        ws = LoadILLDiffraction(Filename=runs, DataType=data_type)

        return GroupWorkspaces(ws)

# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetScanReduction)
