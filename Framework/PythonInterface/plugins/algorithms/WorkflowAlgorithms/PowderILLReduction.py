from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from mantid import config, mtd, logger
from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, \
    WorkspaceGroupProperty, FileAction, Progress, MatrixWorkspaceProperty
from mantid.simpleapi import *  # noqa

def hide(name):
    return '__'+name


class PowderILLReduction(PythonAlgorithm):

    _calibration_file = None
    _normalise_option = None
    _region_of_interest = None
    _observable = None
    _sort_x_axis = None
    _out_name = None
    _temp_name = None
    _calib_name = None
    _roi_name = None
    _mon_name = None
    _progress = None

    def category(self):
        return "ILL\\Diffraction"

    def summary(self):
        return 'Performs powder diffraction data reduction for ILL instrument D20.'

    def name(self):
        return "PowderILLReduction"

    def validateInputs(self):
        issues = dict()

        roi_valid = True

        if self.getPropertyValue('NormaliseTo') == 'ROI':
            roi = self.getPropertyValue('ROI')
            if roi:
                for range in roi.split(','):
                    edges = range.split('-')
                    if len(edges) != 2:
                        roi_valid = False
                        break
                    else:
                        try:
                            if float(edges[0]) >= float(edges[1]):
                                roi_valid = False
                                break
                        except ValueError:
                            roi_valid = False
                            break
            else:
                roi_valid = False
        if not roi_valid:
            issues['ROI'] = 'Invalid region of interest. Specify , separated list of - separated ranges.'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(FileProperty('CalibrationFile', '',
                                          action=FileAction.OptionalLoad, extensions=['.nxs']),
                             doc='File containing the detector efficiencies.')

        self.declareProperty(FileProperty('ROCFile', '',
                                          action=FileAction.OptionalLoad, extensions=['.nxs']),
                             doc='File containing the radial oscillating collimator (ROC) corrections.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Time', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts.')

        self.declareProperty(name='ROI', defaultValue='0-153.6', doc='Regions of interest in scattering angle in degrees.'
                                                              'E.g. 1.5-20,50-110')

        self.declareProperty(name='Observable',
                             defaultValue='sample.temperature',
                             doc='Scanning observable, a Sample Log entry.')

        self.declareProperty(name='SortObservableAxis',
                             defaultValue=False,
                             doc='Whether or not to sort the scanning observable axis.')

        self.declareProperty(name='Unit',
                             defaultValue='ScatteringAngle',
                             validator=StringListValidator(['ScatteringAngle', 'MomentumTransfer', 'dSpacing']),
                             doc='The unit of the reduced diffractogram.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):

        runs = self.getPropertyValue('Run')
        self._out_name = self.getPropertyValue('OutputWorkspace')
        self._observable = self.getPropertyValue('Observable')
        self._sort_x_axis = self.getProperty('SortObservableAxis').value
        self._normalise_option = self.getPropertyValue('NormaliseTo')
        self._calibration_file = self.getPropertyValue('CalibrationFile')

        # prepare the names for the temporary workspaces
        self._temp_name = hide('temp_'+self._out_name)
        self._calib_name = hide('calib_'+self._out_name)
        self._roi_name = hide('roi_'+self._out_name)
        self._mon_name = hide('mon_'+self._out_name)

        if self._normalise_option == 'ROI':
            self._region_of_interest = self.getPropertyValue('ROI')

        to_group = []

        self._progress = Progress(self, start=0.0, end=1.0, nreports=runs.count(',')+runs.count('+')+1)

        for runs_list in runs.split(','):

            runs_sum = runs_list.split('+')

            if len(runs_sum) == 1:
                # nothing to sum
                runnumber = os.path.basename(runs_sum[0]).split('.')[0]
                LoadILLDiffraction(Filename=runs_sum[0], OutputWorkspace=hide(runnumber))
                self._progress.report('Loaded run #' + runnumber)
                to_group.append(hide(runnumber))
            else:
                for i, run in enumerate(runs_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    if i == 0:
                        first = hide(runnumber + '_multiple')
                        LoadILLDiffraction(Filename=run, OutputWorkspace=first)
                        self._progress.report('Loaded run #' + runnumber)
                        to_group.append(first)
                    else:
                        LoadILLDiffraction(Filename=run, OutputWorkspace=hide(runnumber))
                        self._progress.report('Loaded run #' + runnumber)
                        MergeRuns(InputWorkspaces=[first, hide(runnumber)], OutputWorkspace=first)
                        DeleteWorkspace(Workspace=hide(runnumber))

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=self._temp_name)

        if self._normalise_option == 'Time':
            for ws in to_group:
                #normalise to time here, before joining, since the duration is in sample logs
                duration = mtd[ws].getRun().getLogData('duration').value
                Scale(InputWorkspace=ws,OutputWorkspace=ws,Factor=1./duration)

        JoinRuns(InputWorkspaces=self._temp_name, SampleLogAsXAxis=self._observable, OutputWorkspace=self._temp_name+'_joined')

        DeleteWorkspace(self._temp_name)

        RenameWorkspace(InputWorkspace=self._temp_name+'_joined', OutputWorkspace=self._temp_name)

        ExtractMonitors(InputWorkspace=self._temp_name, DetectorWorkspace=self._temp_name,
                        MonitorWorkspace=self._mon_name)

        ConvertSpectrumAxis(InputWorkspace=self._temp_name, OutputWorkspace=self._temp_name, Target='SignedTheta')

        if self._normalise_option == 'Monitor':
            Divide(LHSWorkspace=self._temp_name, RHSWorkspace=self._mon_name, OutputWorkspace=self._temp_name)
        elif self._normalise_option == 'ROI':
            SumSpectra(InputWorkspace=self._temp_name, OutputWorkspace=self._roi_name, ListOfWorkspaceIndices=self._parse_roi())
            SumSpectra(InputWorkspace=self._roi_name, OutputWorkspace=self._roi_name)
            Divide(LHSWorkspace=self._temp_name, RHSWorkspace=self._roi_name, OutputWorkspace=self._temp_name)
            DeleteWorkspace(self._roi_name)

        DeleteWorkspace(self._mon_name)

        if self._calibration_file:
            LoadNexusProcessed(Filename=self._calibration_file, OutputWorkspace=self._calib_name)
            Multiply(LHSWorkspace=self._temp_name, RHSWorkspace=self._calib_name, OutputWorkspace=self._temp_name)
            DeleteWorkspace(self._calib_name)

        if self._sort_x_axis:
            SortXAxis(InputWorkspace=self._temp_name, OutputWorkspace=self._temp_name)

        Transpose(InputWorkspace=self._temp_name, OutputWorkspace=self._temp_name)

        RenameWorkspace(InputWorkspace=self._temp_name, OutputWorkspace=self._out_name)

        self.setProperty('OutputWorkspace', self._out_name)

    def _parse_roi(self):
        '''
        Parses the region of interest string from 2theta ranges to workspace indices
        Returns: roi as workspace indices, e.g. 7-20,100-123
        '''
        result = ''
        axis = np.array(mtd[self._temp_name].getAxis(1).extractValues())
        for range in self._region_of_interest.split(','):
            edges = range.split('-')
            start = float(edges[0])
            end = float(edges[1])
            start_index = np.argwhere(axis>start)
            end_index = np.argwhere(axis<end)
            result += str(start_index[0][0])+'-'+str(end_index[-1][0])
            result += ','
        self.log().notice('ROI summing pattern is '+result[:-1])
        return result[:-1]


#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderILLReduction)
