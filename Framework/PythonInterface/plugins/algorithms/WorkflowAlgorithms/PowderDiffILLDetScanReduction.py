from __future__ import (absolute_import, division, print_function)

from mantid.kernel import CompositeValidator, Direction, FloatArrayLengthValidator, FloatArrayOrderedPairsValidator, \
    FloatArrayProperty, StringListValidator
from mantid.api import DataProcessorAlgorithm, MultipleFileProperty, Progress, WorkspaceGroupProperty
from mantid.simpleapi import *


class PowderDiffILLDetScanReduction(DataProcessorAlgorithm):
    _progress = None

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return 'Performs powder diffraction data reduction for D2B and D20 (when doing a detector scan).'

    def name(self):
        return "PowderDiffILLDetScanReduction"

    def validateInputs(self):
        issues = dict()

        if not (self.getProperty("Output2DTubes").value or
                self.getProperty("Output2D").value or
                self.getProperty("Output1D").value):
            issues['Output2DTubes'] = 'No output chosen'
            issues['Output2D'] = 'No output chosen'
            issues['Output1D'] = 'No output chosen'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Monitor']),
                             doc='Normalise to monitor, or skip normalisation.')

        self.declareProperty(name='UseCalibratedData',
                             defaultValue=True,
                             doc='Whether or not to use the calibrated data in the NeXus files.')

        self.declareProperty(name='Output2DTubes',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against tube scattering angle.')

        self.declareProperty(name='Output2D',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against the real scattering angle.')

        self.declareProperty(name='Output1D',
                             defaultValue=True,
                             doc='Output a 1D workspace with counts against scattering angle.')

        self.declareProperty(FloatArrayProperty(name='HeightRange', values=[],
                                                validator=CompositeValidator([FloatArrayOrderedPairsValidator(),
                                                                              FloatArrayLengthValidator(0, 2)])),
                             doc='A pair of values, comma separated, to give the minimum and maximum height range (in m). If not specified '
                                 'the full height range is used.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):
        data_type = 'Raw'
        if self.getProperty('UseCalibratedData').value:
            data_type = 'Calibrated'

        self._progress = Progress(self, start=0.0, end=1.0, nreports=6)

        self._progress.report('Loading data')
        input_workspace = LoadAndMerge(Filename=self.getPropertyValue('Run'),
                                       LoaderName='LoadILLDiffraction',
                                       LoaderOptions={'DataType': data_type})
        # We might already have a group, but group just in case
        input_group = GroupWorkspaces(InputWorkspaces=input_workspace)

        instrument_name = input_group[0].getInstrument().getName()
        supported_instruments = ['D2B', 'D20']
        if instrument_name not in supported_instruments:
            self.log.warning('Running for unsupported instrument, use with caution. Supported instruments are: '
                             + str(supported_instruments))

        self._progress.report('Normalising to monitor')
        if self.getPropertyValue('NormaliseTo') == 'Monitor':
            input_group = NormaliseToMonitor(InputWorkspace=input_group, MonitorID=0)

        height_range = ''
        height_range_prop = self.getProperty('HeightRange').value
        if (len(height_range_prop) == 2):
            height_range = str(height_range_prop[0]) + ', ' + str(height_range_prop[1])

        output_workspaces = []
        output_workspace_name = self.getPropertyValue('OutputWorkspace')

        self._progress.report('Doing Output2DTubes Option')
        if self.getProperty('Output2DTubes').value:
            output2DTubes = SumOverlappingTubes(InputWorkspaces=input_group,
                                                OutputType='2DTubes',
                                                HeightAxis=height_range,
                                                OutputWorkspace=output_workspace_name + '_2DTubes')
            output_workspaces.append(output2DTubes)

        self._progress.report('Doing Output2D Option')
        if self.getProperty('Output2D').value:
            output2D = SumOverlappingTubes(InputWorkspaces=input_group,
                                           OutputType='2D',
                                           CropNegativeScatteringAngles=True,
                                           HeightAxis=height_range,
                                           OutputWorkspace = output_workspace_name + '_2D')
            output_workspaces.append(output2D)

        self._progress.report('Doing Output1D Option')
        if self.getProperty('Output1D').value:
            output1D = SumOverlappingTubes(InputWorkspaces=input_group,
                                           OutputType='1D',
                                           CropNegativeScatteringAngles=True,
                                           HeightAxis=height_range,
                                           OutputWorkspace=output_workspace_name + '_1D')
            output_workspaces.append(output1D)
        DeleteWorkspace('input_group')

        self._progress.report('Finishing up...')

        GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=output_workspace_name)
        self.setProperty('OutputWorkspace', output_workspace_name)

# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetScanReduction)
