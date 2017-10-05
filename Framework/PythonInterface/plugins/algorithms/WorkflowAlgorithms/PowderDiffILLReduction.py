from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from mantid.kernel import StringListValidator, Direction, FloatArrayProperty, FloatArrayOrderedPairsValidator, \
    VisibleWhenProperty, PropertyCriterion
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, \
    FileAction, Progress, MatrixWorkspaceProperty
from mantid.simpleapi import *


class PowderDiffILLReduction(PythonAlgorithm):

    _calibration_file = None
    _normalise_option = None
    _region_of_interest = []
    _observable = None
    _sort_x_axis = None
    _unit = None
    _out_name = None
    _progress = None
    _format = None

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
        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(FileProperty('CalibrationFile', '',
                                          action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the detector efficiencies.')

        self.declareProperty(FileProperty('ROCFile', '',
                                          action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the radial oscillating collimator (ROC) corrections.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Time', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts.')

        thetaRangeValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(FloatArrayProperty(name='ROI', values=[0, 153.6], validator=thetaRangeValidator),
                             doc='Regions of interest for normalisation [in scattering angle in degrees].')

        normaliseToROI = VisibleWhenProperty('NormaliseTo', PropertyCriterion.IsEqualTo, 'ROI')
        self.setPropertySettings('ROI', normaliseToROI)

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

        self.declareProperty(name='PrepareToSaveAs', defaultValue='NexusProcessed',
                             validator=StringListValidator(['NexusProcessed', 'FullProf', 'GSAS']),
                             doc='Performs some more steps depending on the format intended to save later in. '
                                 'Does nothing by default (nexus processed). '
                                 'This algorithm does not save itself, but prepares for the corresponding format.')

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
        self._format = self.getPropertyValue('PrepareToSaveAs')
        if self._normalise_option == 'ROI':
            self._region_of_interest = self.getProperty('ROI').value

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
                # normalise to time here, before joining, since the duration is in sample logs
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

        # TODO: ROC normalisation goes here

        if self._sort_x_axis:
            SortXAxis(InputWorkspace=joined_ws, OutputWorkspace=joined_ws)

        if self._format == 'FullProf':
            self._crop_negative_2theta(joined_ws)
            self._crop_dead_pixels(joined_ws)

        target = 'SignedTheta'
        if self._unit == 'MomentumTransfer':
            target = 'ElasticQ'
        elif self._unit == 'dSpacing':
            target = 'ElasticDSpacing'

        ConvertSpectrumAxis(InputWorkspace=joined_ws, OutputWorkspace=joined_ws, Target=target)
        Transpose(InputWorkspace=joined_ws, OutputWorkspace=joined_ws)
        RenameWorkspace(InputWorkspace=joined_ws, OutputWorkspace=self._out_name)
        self.setProperty('OutputWorkspace', self._out_name)

    def _crop_negative_2theta(self, ws):
        """
            Crops out the part of the workspace corresponding to negative signed 2theta
            @param ws: the input workspace
        """
        theta_ws = self._hide('2theta')
        ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=theta_ws, Target='SignedTheta', OrderAxis=False)
        positive_index = np.where(mtd[theta_ws].getAxis(1).extractValues() > 0)[0][0]
        self.log().information('First positive 2theta at workspace index: ' + str(positive_index))
        CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, StartWorkspaceIndex=positive_index)
        DeleteWorkspace(theta_ws)

    def _crop_dead_pixels(self, ws):
        """
            Crops out the spectra corresponding to zero counting pixels
            @param ws: the input workspace
        """
        dead_pixels = []
        for spectrum in range(mtd[ws].getNumberHistograms()):
            counts = mtd[ws].readY(spectrum)
            if not np.any(counts):
                dead_pixels.append(spectrum)
        self.log().information('Found zero counting cells at indices: ' + str(dead_pixels))
        MaskDetectors(Workspace=ws, WorkspaceIndexList=dead_pixels)
        ExtractUnmaskedSpectra(InputWorkspace=ws, OutputWorkspace=ws)

    def _normalise_to_roi(self, ws):
        """
            Normalises counts to the sum of counts in the region-of-interest
            @param ws : input workspace with raw spectrum axis
        """
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
        """
            Parses the regions of interest string from 2theta ranges to workspace indices
            @param ws : input workspace with 2theta as spectrum axis
            Returns: roi as workspace indices, e.g. 7-20,100-123
        """
        result = ''
        axis = mtd[ws].getAxis(1).extractValues()
        index = 0
        while index < len(self._region_of_interest):
            start = self._region_of_interest[index]
            end = self._region_of_interest[index+1]
            start_index = np.argwhere(axis > start)
            end_index = np.argwhere(axis < end)
            result += str(start_index[0][0])+'-'+str(end_index[-1][0])
            result += ','
            index += 2
        self.log().information('ROI summing pattern is '+result[:-1])
        return result[:-1]

# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLReduction)
