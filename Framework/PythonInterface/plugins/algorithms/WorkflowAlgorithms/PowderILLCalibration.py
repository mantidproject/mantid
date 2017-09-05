from __future__ import (absolute_import, division, print_function)

import numpy as np
from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm, FileProperty, FileAction, Progress, MatrixWorkspaceProperty
from mantid.simpleapi import *


class PowderILLCalibration(PythonAlgorithm):

    _out_name = None
    _input_file = None
    _calib_file = None
    _progress = None
    _method = None
    _scan_points = None

    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction;Diffraction\\Calibration"

    def summary(self):
        return "Performs detector efficiency correction calculation for powder diffraction data for ILL instrument D20."

    def name(self):
        return "PowderILLCalibration"

    def validateInputs(self):
        return dict()

    def PyInit(self):
        self.declareProperty(FileProperty('CalibrationRun', '', action=FileAction.Load, extensions=['nxs']),
                             doc='File path of calibration run. Must be a detector scan.')

        self.declareProperty(FileProperty('CalibrationFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Optional file containing previous calibration constants.')

        self.declareProperty(name='CalibrationMethod',
                             defaultValue='Median',
                             validator=StringListValidator(['Median', 'WeightedAverage', 'MaximumLikelihood']),
                             doc='The method of how the calibration constant of a '
                                 'pixel relative to the neighbouring one is derived.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the detector efficiencies for each pixel.')

    def _sum_neighbours(self, ref_ws, ws, ws_cropped, factor):
        #ws_axis = mtd[ws].getAxis(1).extractValues()[-1]
        ws_y = mtd[ws].extractY().flatten()[-1]
        ws_e = mtd[ws].extractE().flatten()[-1]
        ws_out = ref_ws + '_temp'
        ws_last = ref_ws + '_last'
        if str(factor) != 'inf':
            Mean(Workspaces=ref_ws + ',' + ws_cropped, OutputWorkspace=ws_out)
        else:
            CloneWorkspace(InputWorkspace=ref_ws, OutputWorkspace=ws_out)
        CreateWorkspace(DataX=[0], DataY=[ws_y], DataE=[ws_e], NSpec=1, OutputWorkspace=ws_last)
        AppendSpectra(InputWorkspace1=ws_out, InputWorkspace2=ws_last, OutputWorkspace=ws_out, ValidateInputs=False)
        DeleteWorkspace(ws_last)
        CropWorkspace(InputWorkspace=ws_out, OutputWorkspace=ws_out, StartWorkspaceIndex=1)
        RenameWorkspace(InputWorkspace=ws_out, OutputWorkspace=ref_ws)
        DeleteWorkspace(ws)
        DeleteWorkspace(ws_cropped)

    def _compute_relative_factor(self, ratio):
        Transpose(InputWorkspace=ratio, OutputWorkspace=ratio)
        factor = 1.
        if self._method == 'Median':
            factor = np.median(mtd[ratio].readY(0))
        DeleteWorkspace(ratio)
        return factor

    def PyExec(self):
        self._input_file = self.getPropertyValue('CalibrationRun')
        self._calib_file = self.getPropertyValue('CalibrationFile')
        self._out_name = self.getPropertyValue('OutputWorkspace')
        self._method = self.getPropertyValue('CalibrationMethod')

        raw_ws = self._hide('raw')
        LoadILLDiffraction(Filename=self._input_file, OutputWorkspace=raw_ws)

        is_scanned = False
        try:
            mtd[raw_ws].detectorInfo().isMasked(0)
        except RuntimeError:
            is_scanned = True
        if not is_scanned:
            raise RuntimeError('The input run does not correspond to a detector scan.')

        self._scan_points = mtd[raw_ws].getRun().getLogData('2theta').size()

        calib_ws = self._hide('calib')
        if self._calib_file:
            LoadNexusProcessed(Filename=self._calib_file, OutputWorkspace=calib_ws)

        ref_ws = self._hide('ref')
        CropWorkspace(InputWorkspace=raw_ws, OutputWorkspace=ref_ws,
                      StartWorkspaceIndex=self._scan_points, EndWorkspaceIndex=2 * self._scan_points - 1)
        #ConvertSpectrumAxis(InputWorkspace=ref_ws, OutputWorkspace=ref_ws, Target='SignedTheta')
        if self._calib_file:
            Scale(InputWorkspace=ref_ws, Factor=mtd[calib_ws].readY(0)[0], OutputWorkspace=ref_ws)
        CropWorkspace(InputWorkspace=ref_ws, OutputWorkspace=ref_ws, StartWorkspaceIndex=1)

        n_det = mtd[raw_ws].detectorInfo().size()
        zeros = np.zeros(n_det - 1)
        ones = np.ones(n_det - 1)

        out_temp = '__'+self._out_name
        CreateWorkspace(DataX=zeros, DataY=ones, DataE=zeros, NSpec=n_det - 1, OutputWorkspace=out_temp)

        self._progress = Progress(self, start=0.0, end=1.0, nreports=n_det)

        raw_y = mtd[raw_ws].extractY().flatten()
        raw_e = mtd[raw_ws].extractE().flatten()
        raw_x = np.zeros(self._scan_points)

        for det in range(2, n_det):
            ws = '__det_' + str(det)
            start = det * self._scan_points
            end = (det + 1) * self._scan_points #- 1
            det_y = raw_y[ start : end ]
            det_e = raw_e[ start : end ]
            #CropWorkspace(InputWorkspace=raw_ws, OutputWorkspace=ws,
            #             StartWorkspaceIndex=,
            #              EndWorkspaceIndex=)

            CreateWorkspace(DataX=raw_x, DataY=det_y, DataE=det_e, NSpec=self._scan_points, OutputWorkspace=ws, ParentWorkspace=raw_ws)

            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='SignedTheta')
            if self._calib_file:
                Scale(InputWorkspace=ws, Factor=mtd[calib_ws].readY(det - 1)[0], OutputWorkspace=ws)
            ws_cropped = ws + '_cropped'
            ratio = ws + '_ratio'
            CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws_cropped,
                          EndWorkspaceIndex=mtd[ws].getNumberHistograms() - 2)
            Divide(LHSWorkspace=ref_ws, RHSWorkspace=ws_cropped, OutputWorkspace=ratio)
            factor = self._compute_relative_factor(ratio)
            self._progress.report()

            if str(factor) != 'inf':
                self.log().debug('Factor derived for detector pixel #' + str(det) + ' is ' + str(factor))
                mtd[out_temp].dataY(det - 1)[0] = factor
                Scale(InputWorkspace=ws, Factor=factor, OutputWorkspace=ws)
                Scale(InputWorkspace=ws_cropped, Factor=factor, OutputWorkspace=ws_cropped)
            else:
                self.log().warning('Factor is inf for pixel #' + str(det))
            self._sum_neighbours(ref_ws, ws, ws_cropped, factor)

        if self._calib_file:
            DeleteWorkspace(calib_ws)

        DeleteWorkspace(ref_ws)
        DeleteWorkspace(raw_ws)

        absolute_norm = np.median(mtd[out_temp].extractY())
        Scale(InputWorkspace=out_temp, OutputWorkspace=out_temp, Factor=1./absolute_norm)
        RenameWorkspace(InputWorkspace=out_temp, OutputWorkspace=self._out_name)
        self.setProperty('OutputWorkspace', self._out_name)

#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderILLCalibration)
