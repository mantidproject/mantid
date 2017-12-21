from __future__ import (absolute_import, division, print_function)

from mantid.kernel import StringListValidator, Direction, FloatArrayProperty, FloatArrayOrderedPairsValidator
from mantid.api import PythonAlgorithm, MultipleFileProperty, Progress, WorkspaceGroupProperty
from mantid.simpleapi import *


class PowderDiffILLDetScanReduction(PythonAlgorithm):
    _progress = None

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

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):
        input_workspaces = self._load()
        instrument_name = input_workspaces[0].getInstrument().getName()
        supported_instruments = ['D2B', 'D20']
        if instrument_name not in supported_instruments:
            self.log.warning('Running for unsupported instrument, use with caution. Supported instruments are: ' + str(supported_instruments))

        self._progress.report('Normalising to monitor')
        if self.getPropertyValue('NormaliseTo') == 'Monitor':
            input_workspaces = NormaliseToMonitor(InputWorkspace=input_workspaces, MonitorID=0)

        height_range = ''
        height_range_prop = self.getProperty('HeightRange').value
        if (len(height_range_prop) == 2):
            height_range = str(height_range_prop[0]) + ', ' + str(height_range_prop[1])

        output_workspaces = []
        self._progress.report('Doing Output2D Option')
        if self.getProperty('Output2D').value:
            output2D = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                           OutputType='2D',
                                           HeightAxis=height_range)
            output_workspaces.append(output2D)

        self._progress.report('Doing Output2DStraight Option')
        if self.getProperty('Output2DStraight').value:
            output2Dstraight = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                                   OutputType='2DStraight',
                                                   HeightAxis=height_range)
            output_workspaces.append(output2Dstraight)

        self._progress.report('Doing Output1D Option')
        if self.getProperty('Output1D').value:
            output1D = SumOverlappingTubes(InputWorkspaces=input_workspaces,
                                           OutputType='1DStraight',
                                           HeightAxis=height_range)
            output_workspaces.append(output1D)
        DeleteWorkspace('input_workspaces')

        self._progress.report('Finishing up...')

        grouped_output_workspace = GroupWorkspaces(output_workspaces)
        output_workspace_name = self.getPropertyValue('OutputWorkspace')
        RenameWorkspace(InputWorkspace=grouped_output_workspace, OutputWorkspace=output_workspace_name)
        self.setProperty('OutputWorkspace', output_workspace_name)


    def _load(self):
        """
            Loads the list of runs
            If sum is requested, MergeRuns is called
            @return : the list of the loaded ws names
        """
        runs = self.getPropertyValue('Run')
        to_group = []
        self._progress = Progress(self, start=0.0, end=1.0, nreports=runs.count(',') + 1 + runs.count('+') + 4)

        data_type = 'Raw'
        if self.getPropertyValue('UsePreCalibratedData'):
            data_type = 'Calibrated'

        for runs_list in runs.split(','):
            runs_sum = runs_list.split('+')
            if len(runs_sum) == 1:
                run = os.path.basename(runs_sum[0]).split('.')[0]
                self._progress.report('Loading run #' + run)
                LoadILLDiffraction(Filename=runs_sum[0], OutputWorkspace=run, DataType=data_type)
                to_group.append(run)
            else:
                for i, run in enumerate(runs_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    output_ws_name = 'merged_runs'
                    if i == 0:
                        self._progress.report('Loading run #' + run)
                        LoadILLDiffraction(Filename=run, OutputWorkspace=output_ws_name, DataType=data_type)
                        to_group.append(output_ws_name)
                    else:
                        run = runnumber
                        self._progress.report('Loading run #' + run)
                        LoadILLDiffraction(Filename=run, OutputWorkspace=run, DataType=data_type)
                        MergeRuns(InputWorkspaces=[output_ws_name, run], OutputWorkspace=output_ws_name)
                        DeleteWorkspace(Workspace=run)

        return GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace='input_workspaces')


# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetScanReduction)
