from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
from mantid.kernel import StringListValidator, Direction, IntArrayBoundedValidator, IntArrayProperty, \
    CompositeValidator, IntArrayLengthValidator, IntArrayOrderedPairsValidator, FloatArrayOrderedPairsValidator, \
    FloatArrayProperty, VisibleWhenProperty, PropertyCriterion
from mantid.api import PythonAlgorithm, FileProperty, FileAction, Progress, MatrixWorkspaceProperty, PropertyMode
from mantid.simpleapi import *


class PowderDiffILLDetEffCorr(PythonAlgorithm):

    _out_name = None            # the name of the output workspace
    _input_file = None          # input file (numor), must be a detector scan
    _calib_file = None          # file containing previously derived calibration constants
    _progress = None            # progress tracking
    _method = None              # calibration method
    _scan_points = None         # number of scan points (time indices)
    _out_response = None        # the name of the second output workspace with merged response
    _bin_offset = None          # this holds int(scan step / pixel size)
    _n_det = None               # number of detector pixels (typically 3072)
    _normalise_to = None        # normalisation option
    _pixel_range = None         # range of the pixels to derive calibration for, e.g. 65,3072
    _regions_of_interest = None # ROI to normalise to, e.g. 10,50,70,100, typically just one range
    _interpolate = None         # whether to interpolate 2thetas before taking relative ratios
    _excluded_ranges = None     # 2theta ranges to exclude when deriving the relative calibration factor
                                # e.g. -20,0,40,50
    _live_pixels = None         # holds the list of cells that are not zero counting

    def _hide(self, name):
        return '__' + self._out_name + '_' + name

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction;Diffraction\\Calibration"

    def summary(self):
        return "Performs detector efficiency correction calculation for powder diffraction instrument D20 at ILL."

    def name(self):
        return "PowderDiffILLDetEffCorr"

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

        self.declareProperty(name='InterpolateOverlappingAngles', defaultValue=False,
                             doc='Wheter to interpolate 2theta values for overlapping regions between neighbouring cells.')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Monitor', 'ROI']),
                             doc='Normalise to time, monitor or ROI counts before deriving the calibration.')

        thetaRangeValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(FloatArrayProperty(name='ROI', values=[0,100.], validator=thetaRangeValidator),
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
            @param ws: input workspace containing data from the current pixel
            @param cropped_ws: same as ws, but last bins cropped to match the size of the reference
            @param ref_ws: current reference workspace
            @param factor: relative efficiency factor for the current pixel
        """
        x = mtd[ws].readX(0)[-self._bin_offset]
        last_bins = ws + '_last_bins'
        CropWorkspace(InputWorkspace=ws, XMin=x, OutputWorkspace=last_bins)

        if factor == 0.:
            CloneWorkspace(InputWorkspace=cropped_ws, OutputWorkspace=ref_ws)
        elif str(factor) != 'inf' and str(factor) != 'nan':
            Scale(InputWorkspace=cropped_ws, OutputWorkspace=cropped_ws, Factor=factor)
            Scale(InputWorkspace=last_bins, OutputWorkspace=last_bins, Factor=factor)
            WeightedMean(InputWorkspace1=ref_ws, InputWorkspace2=cropped_ws, OutputWorkspace=ref_ws)

        ConjoinXRuns(InputWorkspaces=[ref_ws,last_bins], OutputWorkspace=ref_ws)
        x = mtd[ref_ws].readX(0)[self._bin_offset]
        CropWorkspace(InputWorkspace=ref_ws, XMin=x, OutputWorkspace=ref_ws)
        DeleteWorkspace(last_bins)

    def _exclude_ranges(self, ratio_ws):
        """
            Excludes 2theta ranges from the ratio workspace
            @param ratio_ws : the name of the ratio workspace
        """
        ConvertToHistogram(InputWorkspace=ratio_ws, OutputWorkspace=ratio_ws)
        x = mtd[ratio_ws].readX(0)
        xmin = x[0]
        xmax = x[-1]
        for excluded_range in self._excluded_ranges:
            if excluded_range[1] > xmin and excluded_range[0] < xmax:
                if excluded_range[0] > xmin:
                    xmin = excluded_range[0]
                if excluded_range[1] < xmax:
                    xmax = excluded_range[1]
                MaskBins(InputWorkspace=ratio_ws, OutputWorkspace=ratio_ws, XMin=xmin, XMax=xmax)
        ConvertToPointData(InputWorkspace=ratio_ws, OutputWorkspace=ratio_ws)

    def _compute_relative_factor(self, ratio_ws):
        """
            Calculates the relative detector efficiency from the workspace containing response ratios.
            Implements mean, median and most likely mean methods.
            @param ratio_ws: input workspace containing response ratios
            @returns: relative calibration factor
        """
        ratios = mtd[ratio_ws].extractY()
        ratios = ratios[np.nonzero(ratios)]
        factor = 1.
        if ratios.any():
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

    def _reshape(self, raw_ws, ws_2d):
        """
            Reshapes the single column detector scan workspace to a 2D workspace
            with n_det+1 rows (including the monitor) and n_scan_points columns
            Sets the signed 2theta as x-values. The output is a ragged workspace.
            Sample logs are copied over, but the instrument is lost on purpose.
            @param raw_ws : raw detector scan workspace
            @param ws_2d : the name of the returned workspace
        """
        y = mtd[raw_ws].extractY()
        e = mtd[raw_ws].extractE()
        x = mtd[raw_ws].getAxis(1).extractValues()
        shape = [self._n_det+1, self._scan_points]
        y_2d = np.reshape(y, shape)
        e_2d = np.reshape(e, shape)
        x_2d = np.reshape(x, shape)
        CreateWorkspace(DataX=x_2d, DataY=y_2d, DataE=e_2d, NSpec=self._n_det+1, OutputWorkspace=ws_2d)
        CopyLogs(InputWorkspace=raw_ws, OutputWorkspace=ws_2d)

    def _set_input_properties(self):
        """
            Sets up the input properties of the algorithm
        """
        self._input_file = self.getPropertyValue('CalibrationRun')
        self._calib_file = self.getPropertyValue('CalibrationFile')
        self._method = self.getPropertyValue('CalibrationMethod')
        self._normalise_to = self.getPropertyValue('NormaliseTo')
        self._regions_of_interest = self.getProperty('ROI').value
        self._excluded_ranges = self.getProperty('ExcludedRange').value
        self._interpolate = self.getProperty('InterpolateOverlappingAngles').value
        self._pixel_range = self.getProperty('PixelRange').value
        self._out_response = self.getPropertyValue('OutputResponseWorkspace')
        self._out_name = self.getPropertyValue('OutputWorkspace')

    def _configure(self, raw_ws):
        """
            Configures the fundamental parameters for the algorithm.
            @param : the name of the raw detector scan workspace
        """
        self._scan_points = mtd[raw_ws].getRun().getLogData('ScanSteps').value
        self.log().information('Number of scan steps is: ' + str(self._scan_points))
        self._n_det = mtd[raw_ws].detectorInfo().size() - 1
        self.log().information('Number of detector pixels is: ' + str(self._n_det))
        pixel_size = mtd[raw_ws].getRun().getLogData('PixelSize').value
        theta_zeros = mtd[raw_ws].getRun().getLogData('2theta.Position')
        scan_step_in_pixel_numbers = (theta_zeros.nthValue(1) - theta_zeros.nthValue(0)) / pixel_size
        self._bin_offset = int(math.ceil(scan_step_in_pixel_numbers))
        self.log().information('Bin offset is: ' + str(self._bin_offset))
        if (abs(self._bin_offset - scan_step_in_pixel_numbers) > 0.1 and not self._interpolate):
            self.log().warning('Scan step is not an integer multiple of the pixel size. '
                               'Consider checking the option InterpolateOverlappingAngles.')
        if self._pixel_range[1] > self._n_det:
            self.log().warning('Last pixel number provided is larger than total number of pixels. '
                               'Taking the last existing pixel.')
            self._pixel_range[1] = self._n_det
        if self._excluded_ranges.any():
            n_excluded_ranges = int(len(self._excluded_ranges) / 2)
            self._excluded_ranges = np.split(self._excluded_ranges, n_excluded_ranges)

    def _validate_roi(self, ws_2d):
        """
            ROI has to be fully within the aperture of the detector at any time index.
            Example:
            time index : detector span (degrees)
            first      : -30 -> 120
            last       :  10 -> 160
            ROI can not be wider than [10,120]
            @param ws_2d : 2D input workspace
            @throws : ValueError if ROI is not fully within the detector span at any time index
        """
        roi_min = np.min(self._regions_of_interest)
        roi_max = np.max(self._regions_of_interest)
        first_cell_last_time_theta = mtd[ws_2d].readX(1)[-1]
        last_cell_first_time_theta = mtd[ws_2d].readX(self._n_det)[0]
        if roi_min < first_cell_last_time_theta or roi_max > last_cell_first_time_theta:
            raise ValueError('Invalid ROI. The region must be fully contained within the detector at any time index. '
                             'For the given scan configuration, ROI can be within {0} and {1} degrees.'
                             .format(first_cell_last_time_theta,last_cell_first_time_theta))

    def _normalise_roi(self, ws_2d):
        """
            Normalises to the regions of interests (ROI)
            @param ws_2d : 2D input workspace
        """
        y = mtd[ws_2d].extractY()
        x = mtd[ws_2d].extractX()
        roi_counts_arr = np.ones(self._scan_points)
        # typically should be number_rois = 1
        number_rois = int(len(self._regions_of_interest)/2)
        starts = self._regions_of_interest[0::2]
        ends = self._regions_of_interest[1::2]
        first_cells = []
        last_cells = []
        for roi in range(number_rois):
            first_cell = np.argmax(x[...,0]>starts[roi])
            first_cells.append(first_cell)
            last_cell = np.argmin(x[...,0]<ends[roi])
            last_cells.append(last_cell)
        for time_index in range(self._scan_points):
            roi_counts = 0
            counts = y[...,time_index]
            for roi in range(number_rois):
                first_cell = first_cells[roi] - self._bin_offset * time_index
                last_cell = last_cells[roi] - self._bin_offset * time_index
                roi_counts += np.sum(counts[first_cell:last_cell])
            roi_counts_arr[time_index] = roi_counts
        roi_ws = self._hide('roi')
        ExtractSingleSpectrum(InputWorkspace=ws_2d, WorkspaceIndex=0, OutputWorkspace=roi_ws)
        mtd[roi_ws].setY(0, roi_counts_arr)
        mtd[roi_ws].setE(0, np.sqrt(roi_counts_arr))
        Divide(LHSWorkspace=ws_2d, RHSWorkspace=roi_ws, OutputWorkspace=ws_2d)
        DeleteWorkspace(roi_ws)

    def _prepare_response_workspace(self, ws_2d, response_ws):
        """
            Prepares the response workspace with x-axis set as 2theta values
            @param ws_2d : 2D input workspace
            @param response_ws : the name of the output response workspace
        """
        size = (self._n_det - 1) * self._bin_offset + self._scan_points
        y = np.zeros(size)
        e = np.zeros(size)
        x = np.zeros(size)
        x_2d = mtd[ws_2d].extractX()
        x[0:self._scan_points] = x_2d[0,...]
        index = self._scan_points
        for pixel in range(0,self._n_det):
            x[index:(index+self._bin_offset)] = x_2d[pixel,-self._bin_offset:]
            index += self._bin_offset
        CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX='Degrees', OutputWorkspace=response_ws)

    def _perform_absolute_normalisation(self, constants_ws):
        """
            Performs the absolute normalisation
            Find the median of already derived calibration constants, excluding the dead pixels
            Divides all the constants by the median, for zero pixels sets the constants to 1.
            @param constants_ws : the name of the constants workspace
        """
        constants = mtd[constants_ws].extractY()
        absolute_norm = np.median(constants[self._live_pixels])
        self.log().information('Absolute normalisation constant is: ' + str(absolute_norm))
        Scale(InputWorkspace=constants_ws, Factor=1./absolute_norm, OutputWorkspace=constants_ws)
        for pixel in range(mtd[constants_ws].getNumberHistograms()):
            if not self._live_pixels[pixel]:
                mtd[constants_ws].dataY(pixel)[0] = 1.
                mtd[constants_ws].dataE(pixel)[0] = 0.

    def _derive_calibration(self, ws_2d, constants_ws, response_ws):
        """
            Computes the relative calibration factors sequentailly for all the pixels.
            @param : 2D input workspace
            @param : the output workspace name containing the calibration constants
            @param : the output workspace name containing the combined response
        """
        ref_ws = self._hide('ref')
        zeros = np.zeros(self._n_det)
        constants = np.ones(self._n_det)
        self._live_pixels = np.zeros(self._n_det, dtype=bool)
        CreateWorkspace(DataX=zeros, DataY=constants, DataE=zeros, NSpec=self._n_det, OutputWorkspace=constants_ws)

        # loop over all the requested pixels to derive the calibration sequentially
        # this is a serial loop of about 3K iterations, so performance is critical
        nreports = int(self._pixel_range[1] - self._pixel_range[0] + 1)
        self._progress = Progress(self, start=0.0, end=1.0, nreports=nreports)
        for det in range(self._pixel_range[0] - 1, self._pixel_range[1]):
            self._progress.report('Computing the relative calibration factor for pixel #' + str(det))
            ws = '__det_' + str(det)
            ExtractSingleSpectrum(InputWorkspace=ws_2d, WorkspaceIndex=det, OutputWorkspace=ws)
            y = mtd[ws].readY(0)
            x = mtd[ws].readX(0)
            # keep track of dead pixels
            if np.count_nonzero(y) > self._scan_points/5:
                self._live_pixels[det] = True

            if det == self._pixel_range[0] - 1:
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=ref_ws, XMin=x[self._bin_offset])
            else:
                ratio_ws = ws + '_ratio'
                cropped_ws = ws + '_cropped'
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=cropped_ws, XMax=x[-(self._bin_offset+1)])

                if self._interpolate and self._live_pixels[det]:
                    # SplineInterpolation invalidates the errors, so we need to copy them over
                    interp_ws = ws + '_interp'
                    SplineInterpolation(WorkspaceToInterpolate=cropped_ws, WorkspaceToMatch=ref_ws,
                                        OutputWorkspace=interp_ws, OutputWorkspaceDeriv="", EnableLogging=False)
                    mtd[interp_ws].setE(0, mtd[cropped_ws].readE(0))
                    RenameWorkspace(InputWorkspace=interp_ws, OutputWorkspace=cropped_ws)
                else:
                    # here we need to effectively clone the x-axis
                    cloned_ref_ws = ws + '_cloned'
                    CloneWorkspace(InputWorkspace=ref_ws, OutputWorkspace=cloned_ref_ws)
                    mtd[cloned_ref_ws].setY(0, mtd[cropped_ws].readY(0))
                    mtd[cloned_ref_ws].setE(0, mtd[cropped_ws].readE(0))
                    RenameWorkspace(InputWorkspace=cloned_ref_ws, OutputWorkspace=cropped_ws)

                Divide(LHSWorkspace=ref_ws, RHSWorkspace=cropped_ws, OutputWorkspace=ratio_ws, EnableLogging=False)
                if len(self._excluded_ranges) != 0:
                    self._exclude_ranges(ratio_ws)
                factor = self._compute_relative_factor(ratio_ws)
                DeleteWorkspace(ratio_ws)

                if str(factor) == 'nan' or str(factor) == 'inf' or factor == 0.:
                    self.log().warning('Factor is ' + str(factor) + ' for pixel #' + str(det))
                else:
                    self.log().debug('Factor derived for detector pixel #' + str(det) + ' is ' + str(factor))
                    mtd[constants_ws].dataY(det)[0] = factor

                self._update_reference(ws, cropped_ws, ref_ws, factor)
                DeleteWorkspace(cropped_ws)

            if self._out_response:
                # take care of combined response
                end = self._bin_offset
                if det == self._pixel_range[1] - 1:
                    end = self._scan_points - self._bin_offset
                    for scan_point in range(0, self._bin_offset):
                        index = mtd[response_ws].blocksize() - self._bin_offset + scan_point
                        mtd[response_ws].dataY(0)[index] = mtd[ws].readY(0)[end + scan_point]
                        mtd[response_ws].dataE(0)[index] = mtd[ws].readE(0)[end + scan_point]

                for scan_point in range(0, end):
                    index = det * self._bin_offset + scan_point
                    mtd[response_ws].dataY(0)[index] = mtd[ref_ws].readY(0)[scan_point]
                    mtd[response_ws].dataE(0)[index] = mtd[ref_ws].readE(0)[scan_point]

            DeleteWorkspace(ws)
        # end of loop over pixels
        DeleteWorkspace(ref_ws)

    def PyExec(self):
        self._set_input_properties()

        raw_ws = self._hide('raw')
        mon_ws = self._hide('mon')
        ws_2d = self._hide('2d')
        response_ws = self._hide('resp')
        constants_ws = self._hide('constants')

        LoadILLDiffraction(Filename=self._input_file, OutputWorkspace=raw_ws)
        self._validate_scan(raw_ws)
        self._configure(raw_ws)

        ConvertSpectrumAxis(InputWorkspace=raw_ws, OutputWorkspace=raw_ws, Target='SignedTheta', OrderAxis=False)
        self._reshape(raw_ws, ws_2d)
        DeleteWorkspace(raw_ws)
        # extract the monitor spectrum
        ExtractSingleSpectrum(InputWorkspace=ws_2d, WorkspaceIndex=0, OutputWorkspace=mon_ws)

        if self._normalise_to == 'Monitor':
            Divide(LHSWorkspace=ws_2d, RHSWorkspace=mon_ws, OutputWorkspace=ws_2d)
        elif self._normalise_to == 'ROI':
            self._validate_roi(ws_2d)
            self._normalise_roi(ws_2d)

        DeleteWorkspace(mon_ws)
        # only now crop out the monitor spectrum
        CropWorkspace(InputWorkspace=ws_2d, StartWorkspaceIndex=1, OutputWorkspace=ws_2d)
        ReplaceSpecialValues(InputWorkspace=ws_2d, OutputWorkspace=ws_2d,
                             NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)

        if self._calib_file:
            calib_ws = self._hide('calib')
            LoadNexusProcessed(Filename=self._calib_file, OutputWorkspace=calib_ws)
            Multiply(LHSWorkspace=ws_2d, RHSWorkspace=calib_ws, OutputWorkspace=ws_2d)
            DeleteWorkspace(calib_ws)

        if self._out_response:
            self._prepare_response_workspace(ws_2d, response_ws)

        # this is the main calculation
        self._derive_calibration(ws_2d, constants_ws, response_ws)
        DeleteWorkspace(ws_2d)
        self._perform_absolute_normalisation(constants_ws)

        # set output workspace[s]
        RenameWorkspace(InputWorkspace=constants_ws, OutputWorkspace=self._out_name)
        self.setProperty('OutputWorkspace', self._out_name)
        if self._out_response:
            RenameWorkspace(InputWorkspace=response_ws, OutputWorkspace=self._out_response)
            self.setProperty('OutputResponseWorkspace', self._out_response)

#Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetEffCorr)
