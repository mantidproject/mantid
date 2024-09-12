# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import numpy as np
import numpy.ma as ma
from mantid.kernel import (
    CompositeValidator,
    Direction,
    FloatArrayLengthValidator,
    FloatArrayOrderedPairsValidator,
    FloatArrayProperty,
    IntArrayBoundedValidator,
    IntArrayLengthValidator,
    IntArrayProperty,
    IntArrayOrderedPairsValidator,
    IntBoundedValidator,
    PropertyCriterion,
    StringListValidator,
    VisibleWhenProperty,
)
from mantid.api import FileAction, FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, Progress, PropertyMode, PythonAlgorithm

from mantid.simpleapi import *


def _crop_bins(ws, bin_min, bin_max, out):
    """
    Extracts workspace data in [bin_min, bin_max] for ragged workspace
    """
    y = mtd[ws].extractY()
    e = mtd[ws].extractE()
    x = mtd[ws].extractX()
    CreateWorkspace(
        DataX=x[:, bin_min:bin_max],
        DataY=y[:, bin_min:bin_max],
        DataE=e[:, bin_min:bin_max],
        NSpec=mtd[ws].getNumberHistograms(),
        OutputWorkspace=out,
    )


def _divide_friendly(ws1, ws2, out):
    """
    Divides ws1/ws2 ignoring the difference in x-axis
    """
    mtd[ws2].setX(0, mtd[ws1].readX(0))
    Divide(LHSWorkspace=ws1, RHSWorkspace=ws2, OutputWorkspace=out)


def _plus_friendly(ws1, ws2, out):
    """
    Sums ws1+ws2 ignoring the difference in x-axis
    """
    mtd[ws2].setX(0, mtd[ws1].readX(0))
    Plus(LHSWorkspace=ws1, RHSWorkspace=ws2, OutputWorkspace=out)


class PowderILLEfficiency(PythonAlgorithm):
    _out_name = None  # the name of the output workspace
    _input_files = None  # input files (numor), must be detector scans (to list for D2B, to merge for D20)
    _calib_file = None  # file containing previously derived calibration constants
    _progress = None  # progress tracking
    _method = None  # calibration method
    _scan_points = None  # number of scan points (time indices)
    _out_response = None  # the name of the second output workspace with merged response
    _bin_offset = None  # this holds int(scan step / pixel size)
    _n_det = None  # number of detector pixels for D20 (=3072)
    _normalise_to = None  # normalisation option
    _pixel_range = None  # range of the pixels to derive calibration for D20, e.g. 65,3072
    _regions_of_interest = None  # ROI to normalise to, e.g. 10,50,70,100, typically just one range, used for D20
    _interpolate = None  # whether to interpolate 2thetas before taking relative ratios (D20)
    _excluded_ranges = None  # 2theta ranges to exclude when deriving the calibration factor, e.g. -20,0,40,50
    _live_pixels = None  # holds the list of cells that are not zero counting
    _derivation_method = ""  # sequential reference (D20) or global reference (D2B)
    _n_scan_files = None  # number of standard scan files for D2B (~30)
    _n_scans_per_file = None  # number of scan points in a standard scan for D2B (=25)
    _n_tubes = None  # number of tubes in D2B (=128)
    _n_pixels_per_tube = None  # number of pixels per tube in D2B (=128)
    _n_iterations = None  # number of iterations (=1); used for D2B
    _pixels_to_trim = None  # number of pixels to trim from top and bottom of tubes for chi2 calculation (D2B)
    _mask_criterion = None  # the range of efficiency constant values, outside of which they should be set to 0

    def _hide(self, name):
        return "__" + self._out_name + "_" + name

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction;Diffraction\\Calibration"

    def summary(self):
        return (
            "Performs detector efficiency correction calculation for scanning "
            "monochromatic powder diffraction instruments D20 and D2B at ILL."
        )

    def seeAlso(self):
        return ["PowderILLDetectorScan", "PowderILLParameterScan"]

    def name(self):
        return "PowderILLEfficiency"

    def PyInit(self):
        self.declareProperty(
            MultipleFileProperty("CalibrationRun", action=FileAction.Load, extensions=["nxs"]),
            doc="File path of calibration runs (numors). Must be detector scans.",
        )

        self.declareProperty(
            FileProperty("CalibrationFile", "", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Optional file containing previous calibration constants.",
        )

        self.declareProperty(
            name="CalibrationMethod",
            defaultValue="Median",
            validator=StringListValidator(["Median", "Mean", "MostLikelyMean"]),
            doc="The method of how the calibration constant of a pixel " "is derived from the distribution of ratios.",
        )

        self.declareProperty(
            name="DerivationMethod",
            defaultValue="SequentialSummedReference1D",
            validator=StringListValidator(["SequentialSummedReference1D", "GlobalSummedReference2D"]),
            doc="Choose sequential for D20 (1D detector), global for D2B (2D detector).",
        )

        self.declareProperty(
            name="InterpolateOverlappingAngles",
            defaultValue=False,
            doc="Whether to interpolate scattering angle values in overlapping regions (D20 only).",
        )

        self.declareProperty(
            name="NormaliseTo",
            defaultValue="None",
            validator=StringListValidator(["None", "Monitor", "ROI"]),
            doc="Normalise to monitor or ROI counts before deriving the calibration.",
        )

        thetaRangeValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(
            FloatArrayProperty(name="ROI", values=[0, 100.0], validator=thetaRangeValidator),
            doc="Scattering angle regions of interest for normalisation [degrees].",
        )

        normaliseToROI = VisibleWhenProperty("NormaliseTo", PropertyCriterion.IsEqualTo, "ROI")
        self.setPropertySettings("ROI", normaliseToROI)

        self.declareProperty(
            FloatArrayProperty(name="ExcludedRange", values=[], validator=thetaRangeValidator),
            doc="Scattering angle regions to exclude from the computation of "
            "relative calibration constants; for example, the beam stop [degrees]. ",
        )

        pixelRangeValidator = CompositeValidator()
        greaterThanOne = IntArrayBoundedValidator(lower=1)
        lengthTwo = IntArrayLengthValidator()
        lengthTwo.setLength(2)
        orderedPairsValidator = IntArrayOrderedPairsValidator()
        pixelRangeValidator.add(greaterThanOne)
        pixelRangeValidator.add(lengthTwo)
        pixelRangeValidator.add(orderedPairsValidator)

        self.declareProperty(
            IntArrayProperty(name="PixelRange", values=[1, 3072], validator=pixelRangeValidator),
            doc="Range of the pixel numbers to compute the calibration factors for (D20 only); "
            "for the other pixels outside the range, the factor will be set to 1.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputResponseWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="Output workspace containing the summed diffraction patterns of all the overlapping pixels.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Output workspace containing the calibration constants (inverse of efficiency) for each pixel.",
        )

        self.declareProperty(
            name="NumberOfIterations",
            defaultValue=1,
            validator=IntBoundedValidator(lower=0, upper=10),
            doc="Number of iterations to perform (D2B only): 0 means auto; that is, the "
            "iterations will terminate after reaching some Chi2/NdoF.",
        )

        maskCriterionValidator = CompositeValidator()
        arrayLengthTwo = FloatArrayLengthValidator()
        arrayLengthTwo.setLengthMax(2)
        orderedPairs = FloatArrayOrderedPairsValidator()
        maskCriterionValidator.add(arrayLengthTwo)
        maskCriterionValidator.add(orderedPairs)

        self.declareProperty(
            FloatArrayProperty(name="MaskCriterion", values=[], validator=maskCriterionValidator),
            doc="Efficiency constants outside this range will be set to zero.",
        )

    def validateInputs(self):
        issues = dict()

        if self.getPropertyValue("DerivationMethod") == "GlobalSummedReference2D":
            if self.getProperty("InterpolateOverlappingAngles").value:
                issues["InterpolateOverlappingAngles"] = "Interpolation option is not supported for global method"
            if self.getPropertyValue("NormaliseTo") == "ROI":
                issues["NormaliseTo"] = "ROI normalisation is not supported for global method"
            method = self.getPropertyValue("CalibrationMethod")
            if method == "MostLikelyMean" or method == "Mean":
                issues["CalibrationMethod"] = method + " is not supported for global reference method"

        if self.getPropertyValue("DerivationMethod") == "SequentialSummedReference1D":
            if self.getProperty("NumberOfIterations").value != 1:
                issues["NumberOfIterations"] = "NumberOfIterations is not supported for sequential method"

        return issues

    def _update_reference(self, ws, cropped_ws, ref_ws, factor):
        """
        Merges the response of the current pixel with the current combined reference in the
        overlapping region, taking into account the relative scale factor.
        Updates the reference workspace to contain the weighted sum of the two, or the clone
        of one or the other if the scale factor is pathological.

        For example:
            ws:  10-20 degrees
            cropped_ws: 10-18 degrees
            ref_ws: 8-18 degrees

        In this case in the new reference should cover 10-20 degrees as follows:
            10-18: as the weighted mean of the previous reference and the cropped_ws*factor
            18-20: containing the data only from cropped_ws*factor

        8-10 bit is cropped out to keep the reference of the same size for the next iteration.

        @param ws: input workspace containing data from the current pixel
        @param cropped_ws: same as ws, but last bins cropped to match the size of the reference
        @param ref_ws: current reference workspace
        @param factor: relative efficiency factor for the current pixel
        """
        x = mtd[ws].readX(0)[-self._bin_offset]
        last_bins = ws + "_last_bins"
        CropWorkspace(InputWorkspace=ws, XMin=x, OutputWorkspace=last_bins)

        if factor == 0.0:
            CloneWorkspace(InputWorkspace=cropped_ws, OutputWorkspace=ref_ws)
        elif str(factor) != "inf" and str(factor) != "nan":
            Scale(InputWorkspace=cropped_ws, OutputWorkspace=cropped_ws, Factor=factor)
            Scale(InputWorkspace=last_bins, OutputWorkspace=last_bins, Factor=factor)
            WeightedMean(InputWorkspace1=ref_ws, InputWorkspace2=cropped_ws, OutputWorkspace=ref_ws)
        ConjoinXRuns(InputWorkspaces=[ref_ws, last_bins], OutputWorkspace=ref_ws)
        SortXAxis(InputWorkspace=ref_ws, OutputWorkspace=ref_ws)
        x = mtd[ref_ws].readX(0)[self._bin_offset]
        CropWorkspace(InputWorkspace=ref_ws, XMin=x, OutputWorkspace=ref_ws)
        DeleteWorkspace(last_bins)

    def _exclude_ranges(self, ratio_ws):
        """
        Excludes 2theta ranges from the ratio workspace
        @param ratio_ws : the name of the ratio workspace
        """
        ConvertToHistogram(InputWorkspace=ratio_ws, OutputWorkspace=ratio_ws)
        equator = int(mtd[ratio_ws].getNumberHistograms() / 2)
        x = mtd[ratio_ws].readX(equator)
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

    def _compute_relative_factor_1D(self, ratio_ws):
        """
        Calculates the relative detector efficiency from the workspace containing response ratios.
        Implements mean, median and most likely mean methods.
        @param ratio_ws: input workspace containing response ratios
        @returns: relative calibration factor (scalar)
        """
        if len(self._excluded_ranges) != 0:
            self._exclude_ranges(ratio_ws)
        ratios = mtd[ratio_ws].extractY()
        ratios = ratios[np.nonzero(ratios)]
        factor = 1.0
        if ratios.any():
            if self._method == "Median":
                factor = np.median(ratios)
            elif self._method == "Mean":
                factor = np.mean(ratios)
            elif self._method == "MostLikelyMean":
                factor = MostLikelyMean(ratios)
        return factor

    def _compute_relative_factor_2D(self, ratio_ws, tube_index):
        """
        Calculates the relative detector efficiency from the workspace containing response ratios.
        Implements mean, median and most likely mean methods.
        @param ratio_ws: input workspace containing response ratios
        @returns: relative calibration factor (1D array, factor per pixel in the tube)
        """
        if len(self._excluded_ranges) != 0:
            self._exclude_ranges(ratio_ws)
        ratios = mtd[ratio_ws].extractY()
        if tube_index == 0:
            ratios = ratios[:, 0 : -self._n_scans_per_file]
        elif tube_index == self._n_tubes - 1:
            ratios = ratios[:, self._n_scans_per_file :]
        factors = np.ones(ratios.shape[0])
        ratios = ma.masked_array(ratios, mask=[ratios == 0])
        ratios = ma.masked_invalid(ratios)
        if self._method == "Median":
            factors = np.array(ma.median(ratios, axis=1))
        elif self._method == "Mean":
            factors = np.array(ma.mean(ratios, axis=1))
        return factors

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
            raise RuntimeError("The input run is not a detector scan.")

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
        shape = [self._n_det + 1, self._scan_points]
        y_2d = np.reshape(y, shape)
        e_2d = np.reshape(e, shape)
        x_2d = np.reshape(x, shape)
        CreateWorkspace(DataX=x_2d, DataY=y_2d, DataE=e_2d, NSpec=self._n_det + 1, OutputWorkspace=ws_2d)
        CopyLogs(InputWorkspace=raw_ws, OutputWorkspace=ws_2d)

    def _chi_squared(self, calib_current):
        """
        Calculates the termination parameter for automatic iterations for global method (D2B)
        @param calib_current : the residual calibration map at the current iteration
        @return : chi2/NdoF
        """
        start = self._pixels_to_trim
        end = self._n_pixels_per_tube - self._pixels_to_trim
        y = mtd[calib_current].extractY()[:, start:end]
        diff = (y - 1) ** 2
        chi2 = np.sum(diff)
        ndof = (self._n_pixels_per_tube - 2 * self._pixels_to_trim) * self._n_tubes
        return chi2 / ndof

    def _set_input_properties(self):
        """
        Sets up the input properties of the algorithm
        """
        self._input_files = self.getPropertyValue("CalibrationRun")
        self._calib_file = self.getPropertyValue("CalibrationFile")
        self._method = self.getPropertyValue("CalibrationMethod")
        self._derivation_method = self.getPropertyValue("DerivationMethod")
        self._normalise_to = self.getPropertyValue("NormaliseTo")
        self._regions_of_interest = self.getProperty("ROI").value
        self._excluded_ranges = self.getProperty("ExcludedRange").value
        self._interpolate = self.getProperty("InterpolateOverlappingAngles").value
        self._pixel_range = self.getProperty("PixelRange").value
        self._out_response = self.getPropertyValue("OutputResponseWorkspace")
        self._out_name = self.getPropertyValue("OutputWorkspace")
        self._n_iterations = self.getProperty("NumberOfIterations").value
        self._mask_criterion = self.getProperty("MaskCriterion").value

    def _configure_sequential(self, raw_ws):
        """
        Configures the calibration with SequentialSummedReference1D method (D20)
        @param : the name of the raw detector scan (merged) workspace
        """
        self._scan_points = int(mtd[raw_ws].getRun().getLogData("ScanSteps").value)
        self.log().information("Number of scan steps is: " + str(self._scan_points))
        self._n_det = mtd[raw_ws].detectorInfo().size() - 1
        self.log().information("Number of detector pixels is: " + str(self._n_det))
        pixel_size = mtd[raw_ws].getRun().getLogData("PixelSize").value
        theta_zeros = mtd[raw_ws].getRun().getLogData("2theta.Position")
        scan_step_in_pixel_numbers = (theta_zeros.nthValue(1) - theta_zeros.nthValue(0)) / pixel_size
        self._bin_offset = int(math.ceil(scan_step_in_pixel_numbers))
        self.log().information("Bin offset is: " + str(self._bin_offset))
        if abs(self._bin_offset - scan_step_in_pixel_numbers) > 0.1 and not self._interpolate:
            self.log().warning(
                "Scan step is not an integer multiple of the pixel size: {0:.3f}. "
                "Consider checking the option InterpolateOverlappingAngles.".format(scan_step_in_pixel_numbers)
            )
        if self._pixel_range[1] > self._n_det:
            self.log().warning("Last pixel number provided is larger than total number of pixels. " "Taking the last existing pixel.")
            self._pixel_range[1] = self._n_det
        if self._excluded_ranges.any():
            n_excluded_ranges = int(len(self._excluded_ranges) / 2)
            self._excluded_ranges = np.split(self._excluded_ranges, n_excluded_ranges)

    def _configure_global(self, raw_ws):
        """
        Configures the calibration with GlobalSummedReference2D method (D2B)
        @param : first raw ws name in the list
        """
        inst = mtd[raw_ws].getInstrument()
        self._n_tubes = inst.getComponentByName("detectors").nelements()
        self._n_pixels_per_tube = inst.getComponentByName("detectors/tube_1").nelements()
        # self._n_scans_per_file = mtd[raw_ws].getRun().getLogData('ScanSteps').value
        self._n_scans_per_file = 25  # TODO: In v2 this should be freely variable
        self._scan_points = self._n_scans_per_file * self._n_scan_files
        self.log().information("Number of scan steps is: " + str(self._scan_points))
        if self._excluded_ranges.any():
            n_excluded_ranges = int(len(self._excluded_ranges) / 2)
            self._excluded_ranges = np.split(self._excluded_ranges, n_excluded_ranges)

    def _validate_roi(self, ws_2d):
        """
        ROI has to be fully within the aperture of the detector at any time index.
        Example:
        time index : detector span (degrees)
        ------------------------------------
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
            raise ValueError(
                "Invalid ROI. The region must be fully contained within the detector at any time index. "
                "For the given scan configuration, ROI can be within {0} and {1} degrees.".format(
                    first_cell_last_time_theta, last_cell_first_time_theta
                )
            )

    def _normalise_roi(self, ws_2d):
        """
        Normalises to the regions of interests (ROI)
        @param ws_2d : 2D input workspace
        """
        y = mtd[ws_2d].extractY()
        x = mtd[ws_2d].extractX()
        roi_counts_arr = np.ones(self._scan_points)
        # typically should be number_rois = 1
        number_rois = int(len(self._regions_of_interest) / 2)
        starts = self._regions_of_interest[0::2]
        ends = self._regions_of_interest[1::2]
        first_cells = []
        last_cells = []
        for roi in range(number_rois):
            first_cell = np.argmax(x[..., 0] > starts[roi])
            first_cells.append(first_cell)
            last_cell = np.argmin(x[..., 0] < ends[roi])
            last_cells.append(last_cell)
        for time_index in range(self._scan_points):
            roi_counts = 0
            counts = y[..., time_index]
            for roi in range(number_rois):
                first_cell = first_cells[roi] - self._bin_offset * time_index
                last_cell = last_cells[roi] - self._bin_offset * time_index
                roi_counts += np.sum(counts[first_cell:last_cell])
            roi_counts_arr[time_index] = roi_counts
        roi_ws = self._hide("roi")
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
        x[0 : self._scan_points] = x_2d[0, ...]
        index = self._scan_points
        for pixel in range(0, self._n_det - 1):
            x[index : (index + self._bin_offset)] = x_2d[pixel, -self._bin_offset :]
            index += self._bin_offset
        CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="Degrees", OutputWorkspace=response_ws)

    def _perform_absolute_normalisation(self, constants_ws):
        """
        Performs the absolute normalisation
        Find the median of already derived calibration constants, excluding the dead pixels
        Divides all the constants by the median, for zero pixels sets the constants to 1.
        @param constants_ws : the name of the constants workspace
        """
        constants = mtd[constants_ws].extractY()
        absolute_norm = np.median(constants[self._live_pixels])
        self.log().information("Absolute normalisation constant is: " + str(absolute_norm))
        Scale(InputWorkspace=constants_ws, Factor=1.0 / absolute_norm, OutputWorkspace=constants_ws)
        for pixel in range(mtd[constants_ws].getNumberHistograms()):
            if not self._live_pixels[pixel]:
                mtd[constants_ws].dataY(pixel)[0] = 1.0
                mtd[constants_ws].dataE(pixel)[0] = 0.0

    def _derive_calibration_sequential(self, ws_2d, constants_ws, response_ws):
        """
        Computes the relative calibration factors sequentailly for all the pixels.
        This is the main calculation, the performance is critical
        @param : 2D input workspace
        @param : the output workspace name containing the calibration constants
        @param : the output workspace name containing the combined response
        """
        ref_ws = self._hide("ref")
        zeros = np.zeros(self._n_det)
        constants = np.ones(self._n_det)
        self._live_pixels = np.zeros(self._n_det, dtype=bool)
        CreateWorkspace(DataX=zeros, DataY=constants, DataE=zeros, NSpec=self._n_det, OutputWorkspace=constants_ws)

        # loop over all the requested pixels to derive the calibration sequentially
        # this is a serial loop of about 3K iterations (number of pixels), so performance is critical
        # this is due to the method that is sequential; that is, to calibrate pixel N,
        # you need to have the factors derived for all the pixels up to N-1 included
        nreports = int(self._pixel_range[1] - self._pixel_range[0] + 1)
        self._progress = Progress(self, start=0.0, end=1.0, nreports=nreports)
        for det in range(self._pixel_range[0] - 1, self._pixel_range[1]):
            self._progress.report("Computing the relative calibration factor for pixel #" + str(det + 1))
            ws = "__det_" + str(det)
            ExtractSingleSpectrum(InputWorkspace=ws_2d, WorkspaceIndex=det, OutputWorkspace=ws)
            SortXAxis(InputWorkspace=ws, OutputWorkspace=ws)
            y = mtd[ws].readY(0)
            x = mtd[ws].readX(0)
            # keep track of dead pixels
            if np.count_nonzero(y) > self._scan_points / 5:
                self._live_pixels[det] = True

            if det == self._pixel_range[0] - 1:
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=ref_ws, XMin=x[self._bin_offset])
            else:
                ratio_ws = ws + "_ratio"
                cropped_ws = ws + "_cropped"
                CropWorkspace(InputWorkspace=ws, OutputWorkspace=cropped_ws, XMax=x[-(self._bin_offset + 1)])
                ref_bins = mtd[ref_ws].blocksize()
                current_pixel_bins = mtd[cropped_ws].blocksize()
                log_message = "Pixel #{0}: number of bins in reference is {1}, in current cropped is {2}".format(
                    det, ref_bins, current_pixel_bins
                )
                self.log().information(log_message)
                if current_pixel_bins != ref_bins:
                    # after cropping these should always be equal
                    raise RuntimeError("Unequal number of bins in the reference and the cropped workspace. " + log_message)
                if self._interpolate and self._live_pixels[det]:
                    # SplineInterpolation invalidates the errors, so we need to copy them over
                    interp_ws = ws + "_interp"
                    SplineInterpolation(
                        WorkspaceToInterpolate=cropped_ws,
                        WorkspaceToMatch=ref_ws,
                        OutputWorkspace=interp_ws,
                        OutputWorkspaceDeriv="",
                        EnableLogging=False,
                    )
                    mtd[interp_ws].setE(0, mtd[cropped_ws].readE(0))
                    RenameWorkspace(InputWorkspace=interp_ws, OutputWorkspace=cropped_ws)
                else:
                    # here we need to effectively clone the x-axis
                    cloned_ref_ws = ws + "_cloned"
                    CloneWorkspace(InputWorkspace=ref_ws, OutputWorkspace=cloned_ref_ws)
                    mtd[cloned_ref_ws].setY(0, mtd[cropped_ws].readY(0))
                    mtd[cloned_ref_ws].setE(0, mtd[cropped_ws].readE(0))
                    RenameWorkspace(InputWorkspace=cloned_ref_ws, OutputWorkspace=cropped_ws)

                Divide(LHSWorkspace=ref_ws, RHSWorkspace=cropped_ws, OutputWorkspace=ratio_ws, EnableLogging=False)
                factor = self._compute_relative_factor_1D(ratio_ws)
                DeleteWorkspace(ratio_ws)

                if str(factor) == "nan" or str(factor) == "inf" or factor == 0.0:
                    # pixel numbers start from 1
                    self.log().warning("Factor is " + str(factor) + " for pixel #" + str(det + 1))
                else:
                    self.log().debug("Factor derived for detector pixel #" + str(det + 1) + " is " + str(factor))
                    mtd[constants_ws].dataY(det)[0] = factor

                self._update_reference(ws, cropped_ws, ref_ws, factor)
                DeleteWorkspace(cropped_ws)

            if self._out_response:
                # take care of combined response
                end = self._bin_offset
                response = mtd[response_ws]
                responseBlockSize = response.blocksize()
                if det == self._pixel_range[1] - 1:
                    end = self._scan_points - self._bin_offset
                    for scan_point in range(0, self._bin_offset):
                        index = responseBlockSize - self._bin_offset + scan_point
                        response.dataY(0)[index] = mtd[ws].readY(0)[end + scan_point]
                        response.dataE(0)[index] = mtd[ws].readE(0)[end + scan_point]

                for scan_point in range(0, end):
                    index = det * self._bin_offset + scan_point
                    response.dataY(0)[index] = mtd[ref_ws].readY(0)[scan_point]
                    response.dataE(0)[index] = mtd[ref_ws].readE(0)[scan_point]

            DeleteWorkspace(ws)
        # end of loop over pixels
        DeleteWorkspace(ref_ws)

    def _process_sequential(self):
        """
        Performs the sequential derivation for D20 with the following logic:
        1. Take first cell as the reference
        2. Compute the coefficient for the second cell wrt reference
        3. Scale the response of the second cell with the obtained factor
        4. Merge the responses of first and second cells and set it as the new reference
        5. Back to Step 2 for the third pixel and so on...
        """
        raw_ws = self._hide("raw")
        mon_ws = self._hide("mon")
        ws_2d = self._hide("2d")
        response_ws = self._hide("resp")
        constants_ws = self._hide("constants")
        calib_ws = self._hide("calib")

        ConvertSpectrumAxis(InputWorkspace=raw_ws, OutputWorkspace=raw_ws, Target="SignedTheta", OrderAxis=False)
        self._reshape(raw_ws, ws_2d)
        DeleteWorkspace(raw_ws)
        # extract the monitor spectrum
        ExtractSingleSpectrum(InputWorkspace=ws_2d, WorkspaceIndex=0, OutputWorkspace=mon_ws)
        if self._normalise_to == "Monitor":
            Divide(LHSWorkspace=ws_2d, RHSWorkspace=mon_ws, OutputWorkspace=ws_2d)
        elif self._normalise_to == "ROI":
            self._validate_roi(ws_2d)
            self._normalise_roi(ws_2d)
        DeleteWorkspace(mon_ws)
        # only now crop out the monitor spectrum
        CropWorkspace(InputWorkspace=ws_2d, StartWorkspaceIndex=1, OutputWorkspace=ws_2d)
        ReplaceSpecialValues(InputWorkspace=ws_2d, OutputWorkspace=ws_2d, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)

        if self._calib_file:
            Multiply(LHSWorkspace=ws_2d, RHSWorkspace=calib_ws, OutputWorkspace=ws_2d)
            DeleteWorkspace(calib_ws)

        if self._out_response:
            self._prepare_response_workspace(ws_2d, response_ws)

        # this is the main calculation
        self._derive_calibration_sequential(ws_2d, constants_ws, response_ws)
        DeleteWorkspace(ws_2d)
        self._perform_absolute_normalisation(constants_ws)
        mtd[constants_ws].getAxis(1).setUnit("Label").setLabel("Cell #", "")
        mtd[constants_ws].setYUnitLabel("Calibration constant")

    def _crop_last_time_index(self, ws, n_scan_points):
        ws_index_list = ""
        for pixel in range(self._n_tubes * self._n_pixels_per_tube):
            start = n_scan_points * pixel
            end = n_scan_points * (pixel + 1) - 2
            index_range = str(start) + "-" + str(end) + ","
            ws_index_list += index_range
        ws_index_list = ws_index_list[:-1]
        ExtractSpectra(InputWorkspace=ws, OutputWorkspace=ws, WorkspaceIndexList=ws_index_list)

    def _process_global(self):
        """
        Performs the global derivation for D2B following the logic:
        1. SumOverlappingTubes with 2D option to obtain the reference
        2. Loop over tubes, make ratios wrt reference, obtain constants
        3. Apply the constants, and iterate over if requested
        """
        constants_ws = self._hide("constants")
        response_ws = self._hide("resp")
        calib_ws = self._hide("calib")
        ref_ws = self._hide("ref")
        numors = []
        self._progress = Progress(self, start=0.0, end=1.0, nreports=self._n_scan_files)

        for index, numor in enumerate(self._input_files.split(",")):
            self._progress.report("Pre-processing detector scan " + numor[-10:-4])
            ws_name = "__raw_" + str(index)
            numors.append(ws_name)
            LoadILLDiffraction(Filename=numor, OutputWorkspace=ws_name)
            self._validate_scan(ws_name)
            if index == 0:
                if mtd[ws_name].getInstrument().getName() != "D2B":
                    raise RuntimeError("Global reference method is not supported for the instrument given")
                self._configure_global(ws_name)
            if self._normalise_to == "Monitor":
                NormaliseToMonitor(InputWorkspace=ws_name, OutputWorkspace=ws_name, MonitorID=0)
            ExtractMonitors(InputWorkspace=ws_name, DetectorWorkspace=ws_name)
            ConvertSpectrumAxis(InputWorkspace=ws_name, OrderAxis=False, Target="SignedTheta", OutputWorkspace=ws_name)
            if self._calib_file:
                ApplyDetectorScanEffCorr(InputWorkspace=ws_name, DetectorEfficiencyWorkspace=calib_ws, OutputWorkspace=ws_name)

            n_scan_steps = mtd[ws_name].getRun().getLogData("ScanSteps").value
            if n_scan_steps != self._n_scans_per_file:
                self.log().warning(
                    "Run {0} has {1} scan points instead of {2}.".format(numor[-10:-4], n_scan_steps, self._n_scans_per_file)
                )
                self._crop_last_time_index(ws_name, n_scan_steps)

        if self._calib_file:
            DeleteWorkspace(calib_ws)

        constants = np.ones([self._n_pixels_per_tube, self._n_tubes])
        x = np.arange(self._n_tubes)
        e = np.zeros([self._n_pixels_per_tube, self._n_tubes])
        CreateWorkspace(
            DataX=np.tile(x, self._n_pixels_per_tube), DataY=constants, DataE=e, NSpec=self._n_pixels_per_tube, OutputWorkspace=constants_ws
        )
        calib_current = self._hide("current")
        CloneWorkspace(InputWorkspace=constants_ws, OutputWorkspace=calib_current)

        iteration = 0
        chi2_ndof = np.inf  # set a large number to start with
        self._pixels_to_trim = 28
        chi2_ndof_threshold = 1.0
        inst = mtd[numors[0]].getInstrument()
        if inst.hasParameter("pixels_to_trim"):
            self._pixels_to_trim = inst.getIntParameter("pixels_to_trim")[0]
        if inst.hasParameter("chi2_ndof"):
            chi2_ndof_threshold = inst.getNumberParameter("chi2_ndof")[0]

        while iteration < self._n_iterations or (self._n_iterations == 0 and chi2_ndof > chi2_ndof_threshold):
            self._progress = Progress(self, start=0.0, end=1.0, nreports=5)
            self._progress.report("Starting iteration #" + str(iteration))
            self._derive_calibration_global(numors)
            Multiply(LHSWorkspace=constants_ws, RHSWorkspace=calib_current, OutputWorkspace=constants_ws)
            chi2_ndof = self._chi_squared(calib_current)
            if iteration != 0:
                self.log().warning(
                    "Iteration {0}: Chi2/NdoF={1} (termination criterion: < {2})".format(iteration, chi2_ndof, chi2_ndof_threshold)
                )
            iteration += 1

        if self._out_response:
            for index in range(self._n_scan_files):
                ws_name = "__raw_" + str(index)
                ApplyDetectorScanEffCorr(InputWorkspace=ws_name, DetectorEfficiencyWorkspace=calib_current, OutputWorkspace=ws_name)
            SumOverlappingTubes(
                InputWorkspaces=numors,
                OutputWorkspace=response_ws,
                MirrorScatteringAngles=False,
                CropNegativeScatteringAngles=False,
                Normalise=True,
                OutputType="2DTubes",
            )

        DeleteWorkspace(ref_ws)
        DeleteWorkspaces(numors)
        DeleteWorkspace(calib_current)
        mtd[constants_ws].getAxis(0).setUnit("Label").setLabel("Tube #", "")
        mtd[constants_ws].getAxis(1).setUnit("Label").setLabel("Pixel #", "")
        mtd[constants_ws].setYUnitLabel("Calibration constant")

    def _derive_calibration_global(self, numors):
        """
        Derives one iteration of calibration with the global reference method (D2B)
        This is the main calculation, so the performance is critical
        @param numors : list of workspace names
        """
        y_tubes = []
        x_tubes = []
        e_tubes = []
        calib_current = self._hide("current")
        ref_ws = self._hide("ref")
        tubes_group = self._hide("tubes")
        ratios_group = self._hide("ratios")
        shape = [self._n_tubes, self._n_pixels_per_tube, self._n_scans_per_file]
        for i in range(self._n_tubes):
            y_tubes.append([])
            x_tubes.append([])
            e_tubes.append([])

        for index in range(self._n_scan_files):
            ws_name = "__raw_" + str(index)
            ApplyDetectorScanEffCorr(InputWorkspace=ws_name, DetectorEfficiencyWorkspace=calib_current, OutputWorkspace=ws_name)
            y = mtd[ws_name].extractY()
            e = mtd[ws_name].extractE()
            x = mtd[ws_name].getAxis(1).extractValues()
            y_3d = np.reshape(y, shape)
            x_3d = np.reshape(x, shape)
            e_3d = np.reshape(e, shape)
            for tube in range(self._n_tubes):
                y_tubes[tube].append(y_3d[tube, :, :])
                x_tubes[tube].append(x_3d[tube, :, :])
                e_tubes[tube].append(e_3d[tube, :, :])

        self._progress.report("Constructing the global reference")
        SumOverlappingTubes(
            InputWorkspaces=numors,
            OutputWorkspace=ref_ws,
            MirrorScatteringAngles=False,
            CropNegativeScatteringAngles=False,
            Normalise=True,
            OutputType="2DTubes",
        )

        to_group = []
        self._progress.report("Preparing the tube responses")
        for tube in range(self._n_tubes):
            y_tube = np.concatenate(y_tubes[tube], axis=1)
            x_tube = np.concatenate(x_tubes[tube], axis=1)
            e_tube = np.concatenate(e_tubes[tube], axis=1)
            ws_name = "__tube" + str(tube)
            CreateWorkspace(DataX=x_tube, DataY=y_tube, DataE=e_tube, NSpec=self._n_pixels_per_tube, OutputWorkspace=ws_name)
            SortXAxis(InputWorkspace=ws_name, OutputWorkspace=ws_name)
            to_group.append(ws_name)
        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=tubes_group)

        ratios = []
        self._progress.report("Constructing response ratios")
        for tube in reversed(range(self._n_tubes)):
            itube = self._n_tubes - tube - 1
            ratio_ws = "__ratio" + str(tube)
            _crop_bins(ref_ws, itube * self._n_scans_per_file, itube * self._n_scans_per_file + self._scan_points, "__cropped_ref")
            _divide_friendly("__cropped_ref", "__tube" + str(tube), ratio_ws)
            ratios.append(ratio_ws)
            DeleteWorkspace("__cropped_ref")
        GroupWorkspaces(InputWorkspaces=ratios, OutputWorkspace=ratios_group)
        DeleteWorkspace(tubes_group)

        self._progress.report("Computing the calibration constants")
        Transpose(InputWorkspace=calib_current, OutputWorkspace=calib_current)
        for tube in range(self._n_tubes):
            coeff = self._compute_relative_factor_2D("__ratio" + str(tube), tube)
            mtd[calib_current].setY(tube, coeff)
        Transpose(InputWorkspace=calib_current, OutputWorkspace=calib_current)
        DeleteWorkspace(ratios_group)

        ReplaceSpecialValues(
            InputWorkspace=calib_current,
            OutputWorkspace=calib_current,
            NaNValue=1,
            InfinityValue=1,
            SmallNumberThreshold=0.00001,
            SmallNumberValue=1,
        )

    def PyExec(self):
        self._set_input_properties()

        raw_ws = self._hide("raw")
        response_ws = self._hide("resp")
        constants_ws = self._hide("constants")
        calib_ws = self._hide("calib")

        if self._calib_file:
            LoadNexusProcessed(Filename=self._calib_file, OutputWorkspace=calib_ws)

        if self._derivation_method == "SequentialSummedReference1D":  # D20
            self._input_files = self._input_files.replace(",", "+")
            LoadAndMerge(Filename=self._input_files, OutputWorkspace=raw_ws, LoaderName="LoadILLDiffraction")
            if not mtd[raw_ws].getInstrument().getName().startswith("D20"):
                DeleteWorkspace(raw_ws)
                raise RuntimeError("Sequential reference method is not supported for the instrument given")
            self._validate_scan(raw_ws)
            self._configure_sequential(raw_ws)
            self._process_sequential()
        elif self._derivation_method == "GlobalSummedReference2D":  # D2B
            self._input_files = self._input_files.replace("+", ",")
            self._n_scan_files = self._input_files.count(",") + 1
            if self._n_scan_files < 2:
                raise RuntimeError("At least two overlapping scan files needed for the global method")
            self._process_global()

        if self._mask_criterion.any():
            MaskBinsIf(
                InputWorkspace=constants_ws,
                OutputWorkspace=constants_ws,
                Criterion="y<" + str(self._mask_criterion[0]) + "||y>" + str(self._mask_criterion[1]),
            )

        # set output workspace[s]
        RenameWorkspace(InputWorkspace=constants_ws, OutputWorkspace=self._out_name)
        self.setProperty("OutputWorkspace", self._out_name)
        if self._out_response:
            RenameWorkspace(InputWorkspace=response_ws, OutputWorkspace=self._out_response)
            self.setProperty("OutputResponseWorkspace", self._out_response)


# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderILLEfficiency)
