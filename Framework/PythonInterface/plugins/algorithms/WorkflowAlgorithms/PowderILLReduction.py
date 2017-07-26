from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, \
    FileAction, Progress, MatrixWorkspaceProperty
from mantid.simpleapi import *


class PowderILLReduction(PythonAlgorithm):

    _calibration_file = None
    _normalise_option = None
    _region_of_interest = None
    _observable = None
    _sort_x_axis = None
    _unit = None
    _out_name = None
    _progress = None

    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def _hide_run(selfs, runnumber):
        return '__' + runnumber

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return 'Performs powder diffraction data reduction for ILL instrument D20.'

    def name(self):
        return "PowderILLReduction"

    def validateInputs(self):
        issues = dict()
        if self.getPropertyValue('NormaliseTo') == 'ROI':
            roi = self.getPropertyValue('ROI')
            roi_valid = self._validate_roi(roi)
            if roi_valid:
                issues['ROI'] = roi_valid
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

        self.declareProperty(name='ROI', defaultValue='0-153.6',
                             doc='Regions of interest in scattering angle in degrees. E.g. 1.5-20,50-110')

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
        self._unit = self.getPropertyValue('Unit')
        if self._normalise_option == 'ROI':
            self._region_of_interest = self.getPropertyValue('ROI')

        to_group = []
        temp_ws = self._hide('temp')
        joined_ws = self._hide('joined')
        mon_ws = self._hide('mon')
        calib_ws = self._hide('calib')

        self._progress = Progress(self, start=0.0, end=1.0, nreports=runs.count(',')+runs.count('+')+1)

        for runs_list in runs.split(','):
            runs_sum = runs_list.split('+')
            if len(runs_sum) == 1:
                runnumber = os.path.basename(runs_sum[0]).split('.')[0]
                run = self._hide_run(runnumber)
                LoadILLDiffraction(Filename=runs_sum[0], OutputWorkspace=run)
                self._progress.report('Loaded run #' + runnumber)
                to_group.append(run)
            else:
                for i, run in enumerate(runs_sum):
                    runnumber = os.path.basename(run).split('.')[0]
                    if i == 0:
                        first = self._hide_run(runnumber + '_multiple')
                        LoadILLDiffraction(Filename=run, OutputWorkspace=first)
                        self._progress.report('Loaded run #' + runnumber)
                        to_group.append(first)
                    else:
                        run = self._hide_run(runnumber)
                        LoadILLDiffraction(Filename=run, OutputWorkspace=run)
                        self._progress.report('Loaded run #' + runnumber)
                        MergeRuns(InputWorkspaces=[first, run], OutputWorkspace=first)
                        DeleteWorkspace(Workspace=run)

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=temp_ws)

        if self._normalise_option == 'Time':
            for ws in to_group:
                #normalise to time here, before joining, since the duration is in sample logs
                duration = mtd[ws].getRun().getLogData('duration').value
                Scale(InputWorkspace=ws,OutputWorkspace=ws,Factor=1./duration)

        ConjoinXRuns(InputWorkspaces=temp_ws, SampleLogAsXAxis=self._observable, OutputWorkspace=joined_ws)

        DeleteWorkspace(temp_ws)

        ExtractMonitors(InputWorkspace=joined_ws, DetectorWorkspace=joined_ws, MonitorWorkspace=mon_ws)

        if self._normalise_option == 'Monitor':
            Divide(LHSWorkspace=joined_ws, RHSWorkspace=mon_ws, OutputWorkspace=joined_ws)
        elif self._normalise_option == 'ROI':
            self._normalise_to_roi(joined_ws)

        DeleteWorkspace(mon_ws)

        if self._calibration_file:
            LoadNexusProcessed(Filename=self._calibration_file, OutputWorkspace=calib_ws)
            Multiply(LHSWorkspace=joined_ws, RHSWorkspace=calib_ws, OutputWorkspace=joined_ws)
            DeleteWorkspace(calib_ws)

        #TODO ROC normalisation goes here

        if self._sort_x_axis:
            SortXAxis(InputWorkspace=joined_ws, OutputWorkspace=joined_ws)

        if self._unit == 'ScatteringAngle':
            ConvertSpectrumAxis(InputWorkspace=joined_ws, OutputWorkspace=joined_ws, Target='SignedTheta')
        elif self._unit == 'MomentumTransfer':
            ConvertSpectrumAxis(InputWorkspace=joined_ws, OutputWorkspace=joined_ws, Target='ElasticQ')

        Transpose(InputWorkspace=joined_ws, OutputWorkspace=joined_ws)

        RenameWorkspace(InputWorkspace=joined_ws, OutputWorkspace=self._out_name)

        self.setProperty('OutputWorkspace', self._out_name)

    def _normalise_to_roi(self, ws):
        '''
        Normalises counts to the sum of counts in the region-of-interest
        @param ws : input workspace with raw spectrum axis
        '''
        theta_ws = self._hide('theta')
        roi_ws = self._hide('roi')
        ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=theta_ws, Target='SignedTheta')
        roi_pattern = self._parse_roi(theta_ws)
        SumSpectra(InputWorkspace=ws, OutputWorkspace=roi_ws, ListOfWorkspaceIndices=roi_pattern)
        SumSpectra(InputWorkspace=roi_ws, OutputWorkspace=roi_ws)
        Divide(LHSWorkspace=ws, RHSWorkspace=roi_ws, OutputWorkspace=ws)
        DeleteWorkspace(roi_ws)
        DeleteWorkspace(theta_ws)

    def _parse_roi(self, ws):
        '''
        Parses the region of interest string from 2theta ranges to workspace indices
        @param ws : input workspace with 2theta as spectrum axis
        Returns: roi as workspace indices, e.g. 7-20,100-123
        '''
        result = ''
        axis = np.array(mtd[ws].getAxis(1).extractValues())
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

    def _validate_roi(self, roi):
        '''
        Validates the region of interest
        Returns : Empty if valid, error message otherwise
        '''
        roi_valid = True

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
            return 'Invalid region of interest. Specify , separated list of - separated ranges.'
        else:
            return ''


#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderILLReduction)
