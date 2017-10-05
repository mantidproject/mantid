from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
from mantid.kernel import StringListValidator, Direction, IntArrayBoundedValidator, IntArrayProperty, \
    CompositeValidator, IntArrayLengthValidator, IntArrayOrderedPairsValidator, FloatArrayOrderedPairsValidator, \
    FloatArrayProperty, VisibleWhenProperty, PropertyCriterion
from mantid.api import PythonAlgorithm, FileProperty, FileAction, Progress, MatrixWorkspaceProperty, PropertyMode
from mantid.simpleapi import *


class PowderDiffILLCalibration(PythonAlgorithm):

    _out_name = None
    _input_file = None
    _calib_file = None
    _progress = None
    _method = None
    _scan_points = None
    _out_response = None
    _bin_offset = None
    _n_det = None
    _normalise_to = None
    _first_pixel = None
    _last_pixel = None
    _excluded_ranges = []

    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction;Diffraction\\Calibration"

    def summary(self):
        return "Performs detector efficiency correction calculation for powder diffraction instrument D20 at ILL."

    def name(self):
        return "PowderILLCalibration"

    def PyInit(self):
        self.declareProperty(FileProperty('CalibrationRun', '', action=FileAction.Load, extensions=['nxs']),
                             doc='File path of calibration run. Must be a detector scan.')

        self.declareProperty(FileProperty('CalibrationFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='Optional file containing previous calibration constants.')

        self.declareProperty(name='CalibrationMethod',
                             defaultValue='Median',
                             validator=StringListValidator(['Median', 'Mean', 'MostLikelyMean']),
                             doc='The method of how the calibration constant of a '
                                 'pixel relative to the neighbouring one is derived.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts before deriving the calibration.')

        thetaRangeValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(FloatArrayProperty(name='ROI', values=[0,153.6], validator=thetaRangeValidator),
                             doc='Regions of interest for normalisation [in scattering angle in degrees].')

        normaliseToROI = VisibleWhenProperty('NormaliseTo', PropertyCriterion.IsEqualTo, 'ROI')
        self.setPropertySettings('ROI', normaliseToROI)

        self.declareProperty(FloatArrayProperty(name='ExcludedRange', values=[], validator=thetaRangeValidator),
                             doc='2theta regions to exclude from the computation of relative calibration constants '
                                 '[in scattering angle in degrees]. ')

        pixelRangeValidator = CompositeValidator()
        greaterThanOne = IntArrayBoundedValidator()
        greaterThanOne.setLower(1)
        lengthTwo = IntArrayLengthValidator()
        lengthTwo.setLength(2)
        orderedPairsValidator = IntArrayOrderedPairsValidator()
        pixelRangeValidator.add(greaterThanOne)
        pixelRangeValidator.add(lengthTwo)
        pixelRangeValidator.add(orderedPairsValidator)

        self.declareProperty(IntArrayProperty(name='PixelRange', values=[1,3072], validator=pixelRangeValidator),
                             doc='Range of the pixel numbers to compute the calibration factors for. '
                                 'For the other pixels outside the range, the factor will be set to 1.')

        self.declareProperty(MatrixWorkspaceProperty('OutputResponseWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='Output workspace containing the summed diffraction patterns of all the pixels.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Output workspace containing the detector efficiencies for each pixel.')

    def validateInputs(self):
        issues = dict()
        return issues

    def _update_reference(self, ws, cropped_ws, ref_ws, factor):
        """
            Merges the response of the current pixel with the current combined reference in the
            overlapping region, taking into account the relative scale factor.
            Updates the reference workspace to contain the weighted sum of the two, or the clone
            of one or the other if the scale factor is pathological.
            @param ws: workspace containing data from the current pixel
            @param cropped_ws: same as ws, but last bins cropped to match the size of the reference
            @param ref_ws: current reference workspace
            @param factor: relative efficiency factor for the current pixel
        """

        ws_y = mtd[ws].extractY().flatten()[-self._bin_offset:]
        ws_e = mtd[ws].extractE().flatten()[-self._bin_offset:]
        ws_t = mtd[ws].getAxis(1).extractValues()[-self._bin_offset:]
        ws_x = np.zeros(self._bin_offset)
        ws_out = ref_ws + '_temp'
        ws_last = ref_ws + '_last'

        if factor == 0.:
            CloneWorkspace(InputWorkspace=cropped_ws, OutputWorkspace=ws_out)
        elif str(factor) == 'inf' or str(factor) == 'nan':
            CloneWorkspace(InputWorkspace=ref_ws, OutputWorkspace=ws_out)
        else:
            Scale(InputWorkspace=cropped_ws, OutputWorkspace=cropped_ws, Factor=factor)
            WeightedMean(InputWorkspace1=ref_ws, InputWorkspace2=cropped_ws, OutputWorkspace=ws_out)

        CreateWorkspace(DataX=ws_x, DataY=ws_y, DataE=ws_e, NSpec=self._bin_offset, OutputWorkspace=ws_last,
                        VerticalAxisValues=ws_t, VerticalAxisUnit='Degrees')
        AppendSpectra(InputWorkspace1=ws_out, InputWorkspace2=ws_last, OutputWorkspace=ws_out, ValidateInputs=False)
        DeleteWorkspace(ws_last)
        CropWorkspace(InputWorkspace=ws_out, OutputWorkspace=ws_out, StartWorkspaceIndex=self._bin_offset)
        RenameWorkspace(InputWorkspace=ws_out, OutputWorkspace=ref_ws)
        DeleteWorkspace(cropped_ws)

    def _exclude_ranges(self, ratios, errors, angles):
        """
            Exctract the subarrays from ratios and errors, where angles are
            outside the given list of ranges
            @param ratios: input array of response ratios
            @param errors: errors on the ratios
            @param angles: 2theta angles corresponding to the ratios
            @returns: a tuple of extracted ratios, errors, angles
        """

        for range in np.split(self._excluded_ranges, len(self._excluded_ranges) / 2):
            cond = (angles < range[0]) | (angles > range[1])
            ratios = ratios[cond]
            errors = errors[cond]
            angles = angles[cond]
        return [ratios, errors, angles]

    def _compute_relative_factor(self, ratio):
        """
            Calculates the relative detector efficiency from the workspace containing response ratios.
            Implements mean, median and most likely mean methods.
            @param ratio: input workspace containing response ratios
            @returns: relative calibration factor
        """

        ratios = mtd[ratio].extractY()
        errors = mtd[ratio].extractE()
        angles = mtd[ratio].getAxis(1).extractValues()

        if self._excluded_ranges is not None:
            if len(self._excluded_ranges) is not 0:
                [ratios, errors, angles] = self._exclude_ranges(ratios, errors, angles)

        factor = 1.
        if self._method == 'Median':
            factor = np.median(ratios)
        elif self._method == 'Mean':
            factor = np.mean(ratios)
        elif self._method == 'MostLikelyMean':
            factor = MostLikelyMean(ratios)
        return factor

    def _validate_scan(self, scan_ws):
        """
            Ensures that the input workspace corresponds to a detector scan
            @param scan_ws: input detector scan workspace
            @throws: RuntimeError if the workspace is not a detector scan
        """

        is_scanned = False
        try:
            mtd[scan_ws].detectorInfo().isMasked(0)
        except RuntimeError:
            is_scanned = True
        if not is_scanned:
            raise RuntimeError('The input run does not correspond to a detector scan.')

    def _calibrate(self, raw_ws):
        """
            Applies previously derived calibration to the vanadium detector scan.
            @param raw_ws: the raw loaded workspace
        """

        calib_ws = self._hide('calib')
        LoadNexusProcessed(Filename=self._calib_file, OutputWorkspace=calib_ws)

        constants = mtd[calib_ws].extractY()
        errors = mtd[calib_ws].extractE()
        xaxis = np.zeros(self._n_det)

        constants = np.repeat(constants, self._scan_points)
        errors = np.repeat(errors, self._scan_points)
        xaxis = np.repeat(xaxis, self._scan_points)

        CreateWorkspace(DataX=xaxis, DataY=constants, DataE=errors,
                        NSpec=self._scan_points * self._n_det, OutputWorkspace=calib_ws)
        Multiply(LHSWorkspace=raw_ws, RHSWorkspace=calib_ws, OutputWorkspace=raw_ws)

    def _normalise_to_monitor(self, raw_ws, mon_ws):
        """
            Normalises the input data to monitor
            @param raw_ws: raw input workspace
            @param mon_ws: monitor workspace
        """

        mon_counts = mtd[mon_ws].extractY()
        mon_errors = mtd[mon_ws].extractE()
        xaxis = np.zeros(self._scan_points)

        mon_counts = np.tile(mon_counts, [self._n_det])
        mon_errors = np.tile(mon_errors, [self._n_det])
        xaxis = np.tile(xaxis, [self._n_det])

        CreateWorkspace(DataX=xaxis, DataY=mon_counts, DataE=mon_errors,
                        NSpec=self._scan_points * self._n_det, OutputWorkspace=mon_ws)

        Divide(LHSWorkspace=raw_ws, RHSWorkspace=mon_ws, OutputWorkspace=raw_ws)
        ReplaceSpecialValues(InputWorkspace=raw_ws, OutputWorkspace=raw_ws,
                             NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)

    def PyExec(self):
        self._input_file = self.getPropertyValue('CalibrationRun')
        self._calib_file = self.getPropertyValue('CalibrationFile')
        self._method = self.getPropertyValue('CalibrationMethod')
        self._normalise_to = self.getPropertyValue('NormaliseTo')
        self._excluded_ranges = self.getProperty('ExcludedRange').value
        self._first_pixel = self.getProperty('PixelRange').value[0]
        self._last_pixel = self.getProperty('PixelRange').value[1]
        self._out_response = self.getPropertyValue('OutputResponseWorkspace')
        self._out_name = self.getPropertyValue('OutputWorkspace')

        raw_ws = self._hide('raw')
        ref_ws = self._hide('ref')
        mon_ws = self._hide('mon')

        LoadILLDiffraction(Filename=self._input_file, OutputWorkspace=raw_ws)
        self._validate_scan(raw_ws)

        self._scan_points = mtd[raw_ws].getRun().getLogData('ScanSteps').value
        self.log().information('Number of scan steps is: ' + str(self._scan_points))
        self._n_det = mtd[raw_ws].detectorInfo().size() - 1
        self.log().information('Number of detector pixels is: ' + str(self._n_det))

        self._progress = Progress(self, start=0.0, end=1.0, nreports=self._n_det)

        ExtractMonitors(InputWorkspace=raw_ws, DetectorWorkspace=raw_ws, MonitorWorkspace=mon_ws)
        ConvertSpectrumAxis(InputWorkspace=raw_ws, OutputWorkspace=raw_ws, Target='SignedTheta', OrderAxis=False)

        if self._last_pixel > self._n_det:
            self.log().warning('Last pixel number provided is larger than total number of pixels. '
                               'Taking the last existing pixel.')
            self._last_pixel = self._n_det

        pixel_size = mtd[raw_ws].getRun().getLogData('PixelSize').value
        theta_zeros = mtd[raw_ws].getRun().getLogData('2theta')
        self._bin_offset = int(math.ceil((theta_zeros.nthValue(1) - theta_zeros.nthValue(0)) / pixel_size))
        self.log().information('Bin offset is: ' + str(self._bin_offset))

        if self._normalise_to == 'Monitor':
            self._normalise_to_monitor(raw_ws, mon_ws)
        DeleteWorkspace(mon_ws)

        if self._calib_file:
            self._calibrate(raw_ws)

        raw_y = mtd[raw_ws].extractY().flatten()
        raw_e = mtd[raw_ws].extractE().flatten()
        raw_x = mtd[raw_ws].extractX().flatten()
        raw_t = mtd[raw_ws].getAxis(1).extractValues()

        zeros = np.zeros(self._n_det)
        constants = np.ones(self._n_det)
        live_pixels = np.zeros(self._n_det, dtype=bool)

        comb_response_size = (self._n_det - 1) * self._bin_offset + self._scan_points
        comb_response = np.zeros(comb_response_size)
        comb_response_err = np.zeros(comb_response_size)
        comb_response_theta = np.zeros(comb_response_size)
        comb_response_zeros = np.zeros(comb_response_size)

        if self._out_response:
            # prepare the combined response if requested
            for pixel in range(1, self._first_pixel):
                for scan_point in range(0, self._bin_offset):
                    comb_response_theta[(pixel - 1) * self._bin_offset + scan_point] = \
                        raw_t[(pixel - 1) * self._scan_points + scan_point]

            for pixel in range(self._last_pixel + 1, self._n_det + 1):
                end = self._bin_offset
                if pixel == self._n_det:
                    end = self._scan_points
                for scan_point in range(0, end):
                    comb_response_theta[(pixel - 1) * self._bin_offset + scan_point] = \
                        raw_t[(pixel - 1) * self._scan_points + scan_point]

        # loop over all the requested pixels to derive the calibration
        for det in range(self._first_pixel, self._last_pixel + 1):
            ws = '__det_' + str(det)
            start = (det - 1) * self._scan_points
            end = det * self._scan_points
            det_y = raw_y[start:end]
            det_e = raw_e[start:end]
            det_x = raw_x[start:end]
            det_t = raw_t[start:end]

            # keep track of dead pixels
            if np.any(det_y):
                live_pixels[det - 1] = True

            CreateWorkspace(DataX=det_x, DataY=det_y, DataE=det_e, NSpec=self._scan_points,
                            VerticalAxisValues=det_t, VerticalAxisUnit='Degrees', OutputWorkspace=ws)

            if det == self._first_pixel:
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=ref_ws, StartWorkspaceIndex=self._bin_offset)
            else:
                ratio_ws = ws + '_ratio'
                cropped_ws = ws + '_cropped'
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=cropped_ws,
                              EndWorkspaceIndex=self._scan_points - self._bin_offset - 1)
                self._progress.report('Computing the relative calibration factor for pixel #' + str(det))
                Divide(LHSWorkspace=ref_ws, RHSWorkspace=cropped_ws, OutputWorkspace=ratio_ws, EnableLogging=False)
                factor = self._compute_relative_factor(ratio_ws)
                DeleteWorkspace(ratio_ws)
                if str(factor) == 'nan' or str(factor) == 'inf' or factor == 0.:
                    self.log().warning('Factor is ' + str(factor) + ' for pixel #' + str(det))
                else:
                    self.log().debug('Factor derived for detector pixel #' + str(det) + ' is ' + str(factor))
                    constants[det - 1] = factor

                self._update_reference(ws, cropped_ws, ref_ws, factor)

            if self._out_response:
                # take care of combined response
                end = self._bin_offset
                if det == self._last_pixel:
                    end = self._scan_points - self._bin_offset
                    for scan_point in range(0, self._bin_offset):
                        index = comb_response_size - self._bin_offset + scan_point
                        ws_index = self._scan_points - self._bin_offset + scan_point
                        comb_response[index] = mtd[ws].readY(ws_index)[0]
                        comb_response_err[index] = mtd[ws].readE(ws_index)[0]
                        comb_response_theta[index] = det_t[ws_index]

                for scan_point in range(0, end):
                    index = (det - 1) * self._bin_offset + scan_point
                    comb_response[index] = mtd[ref_ws].readY(scan_point)[0]
                    comb_response_err[index] = mtd[ref_ws].readE(scan_point)[0]
                    comb_response_theta[index] = det_t[scan_point]

            DeleteWorkspace(ws)

        DeleteWorkspace(ref_ws)
        DeleteWorkspace(raw_ws)

        # perform absolute normalisation
        absolute_norm = np.median(constants[live_pixels])
        self.log().information('Absolute normalisation constant is: ' + str(absolute_norm))
        constants = np.divide(constants, absolute_norm)
        constants[np.logical_not(live_pixels)] = 1.

        # create and set output workspace[s]
        CreateWorkspace(DataX=zeros, DataY=constants, DataE=zeros, NSpec=self._n_det, OutputWorkspace=self._out_name)
        self.setProperty('OutputWorkspace', self._out_name)

        if self._out_response:
            CreateWorkspace(DataX=comb_response_zeros, DataY=comb_response, DataE=comb_response_err,
                            VerticalAxisUnit='Degrees', VerticalAxisValues=comb_response_theta,
                            NSpec=comb_response_size, OutputWorkspace=self._out_response)
            self.setProperty('OutputResponseWorkspace', self._out_response)

#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLCalibration)
