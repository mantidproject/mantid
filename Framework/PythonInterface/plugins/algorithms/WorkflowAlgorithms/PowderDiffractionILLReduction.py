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


class PowderDiffractionILLReduction(PythonAlgorithm):

    _calibration_file = None
    _normalise_option = None
    _region_of_interest = None
    _observable = None
    _sort_x_axis = None
    _out_name = None

    def category(self):
        return "ILL\\Diffraction"

    def summary(self):
        return 'Performs powder diffraction data reduction for ILL instrument D20.'

    def name(self):
        return "PowderDiffractionILLReduction"

    def validateInputs(self):
        return dict()

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(FileProperty('CalibrationFile', '',
                                          action=FileAction.OptionalLoad, extensions=['.nxs']),
                             doc='File containing the detector efficiencies.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Time',
                             validator=StringListValidator(['Time', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts.')

        self.declareProperty(name='ROI', defaultValue='', doc='Region of interest.')

        self.declareProperty(name='Observable',
                             defaultValue='sample.temperature',
                             doc='Scanning observable, a Sample Log entry.')

        self.declareProperty(name='SortXAxis',
                             defaultValue=False,
                             doc='Whether or not to sort the x-axis.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

    def PyExec(self):

        runs = self.getPropertyValue('Run')

        self._out_name = self.getPropertyValue('OutputWorkspace')
        self._observable = self.getPropertyValue('Observable')
        self._sort_x_axis = self.getPropertyValue('SortXAxis')
        self._normalise_option = self.getPropertyValue('NormaliseTo')

        to_group = []

        for runs_list in runs.split(','):

            runs_sum = runs_list.split('+')

            if len(runs_sum) == 1:
                # nothing to sum
                runnumber = os.path.basename(runs_sum[0]).split('.')[0]
                LoadILLDiffraction(Filename=runs_sum[0], OutputWorkspace=hide(runnumber))
                to_group.append(hide(runnumber))
            else:
                for i, run in enumerate(runs_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    if i == 0:
                        first = hide(runnumber + '_multiple')
                        LoadILLDiffraction(Filename=run, OutputWorkspace=first)
                        to_group.append(first)
                    else:
                        LoadILLDiffraction(Filename=run, OutputWorkspace=hide(runnumber))
                        MergeRuns(InputWorkspaces=[first, hide(runnumber)], OutputWorkspace=first)
                        DeleteWorkspace(Workspace=hide(runnumber))

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace='__temp')

        if self._calibration_file:
            LoadNexusProcessed(Filename=self._calibration_file, OutputWorkspace='__calib')

        for ws in to_group:
            if self._normalise_option == 'Monitor':
                #normalise to monitor
                ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws, MonitorWorkspace=ws+'_mon')
                Divide(LHSWorkspace=ws, RHSWorkspace=ws+'_mon')
                DeleteWorkspace(ws+'_mon')
            elif self._normalise_option == 'Time':
                #normalise to time
                duration = mtd[ws].getRun().getLogData('duration')
                Scale(InputWorkspace=ws,OutputWorkspace=ws,Factor=1./duration)
            else:
                #normalise to ROI
                pass

            if self._calibration_file:
                Divide(LHSWorkspace=ws, RHSWorkspace='__calib', OutputWorkspace=ws)

        if self._calibration_file:
            DeleteWorkspace('__calib')

        JoinRuns(InputWorkspaces='__temp', SampleLogAsXAxis=self._observable, OutputWorkspace='__temp')

        if self._sort_x_axis:
            SortXAxis(InputWorkspace='__temp', OutputWorkspace=self._out_name)

        self.setProperty('OutputWorkspace', self._out_name)

#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffractionILLReduction)
