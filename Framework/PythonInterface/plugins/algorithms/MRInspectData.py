# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=bare-except,no-init,invalid-name,dangerous-default-value
import sys
from mantid.api import AlgorithmFactory, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction, IntArrayLengthValidator, IntArrayProperty, Property
import mantid.simpleapi
import math
import copy
import numpy as np
import scipy.optimize as opt

DEAD_PIXELS = 10
NX_PIXELS = 304
NY_PIXELS = 256


class MRInspectData(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "MRInspectData"

    def summary(self):
        return "This algorithm inspects Magnetism Reflectometer data and populates meta-data."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("Workspace", "", Direction.Input), "Input workspace")

        # Peak finding options
        self.declareProperty("UseROI", True, doc="If true, use the meta-data ROI rather than finding the ranges")
        self.declareProperty("UpdatePeakRange", False, doc="If true, a fit will be performed and the peak ranges will be updated")
        self.declareProperty("UseROIBck", False, doc="If true, use the 2nd ROI in the meta-data for the background")
        self.declareProperty("UseTightBck", False, doc="If true, use the area on each side of the peak to compute the background")
        self.declareProperty("BckWidth", 10, doc="If UseTightBck is true, width of the background on each side of the peak")

        self.declareProperty("ForcePeakROI", False, doc="If true, use the PeakROI property as the ROI")
        self.declareProperty(
            IntArrayProperty("PeakROI", [0, 0], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the reflectivity peak",
        )
        self.declareProperty("ForceLowResPeakROI", False, doc="If true, use the LowResPeakROI property as the ROI")
        self.declareProperty(
            IntArrayProperty("LowResPeakROI", [0, 0], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the low-resolution peak",
        )
        self.declareProperty("ForceBckROI", False, doc="If true, use the BckROI property as the ROI")
        self.declareProperty(
            IntArrayProperty("BckROI", [0, 0], IntArrayLengthValidator(2), direction=Direction.Input), "Pixel range defining the background"
        )
        self.declareProperty("EventThreshold", 10000, "Minimum number of events needed to call a data set a valid direct beam")
        self.declareProperty("DirectPixelOverwrite", Property.EMPTY_DBL, doc="DIRPIX overwrite value")
        self.declareProperty("DAngle0Overwrite", Property.EMPTY_DBL, doc="DANGLE0 overwrite value (degrees)")

    def PyExec(self):
        nxs_data = self.getProperty("Workspace").value
        nxs_data_name = self.getPropertyValue("Workspace")
        data_info = DataInfo(
            nxs_data,
            cross_section=nxs_data_name,
            use_roi=self.getProperty("UseROI").value,
            update_peak_range=self.getProperty("UpdatePeakRange").value,
            use_roi_bck=self.getProperty("UseROIBck").value,
            use_tight_bck=self.getProperty("UseTightBck").value,
            bck_offset=self.getProperty("BckWidth").value,
            force_peak_roi=self.getProperty("ForcePeakROI").value,
            peak_roi=self.getProperty("PeakROI").value,
            force_low_res_roi=self.getProperty("ForceLowResPeakROI").value,
            low_res_roi=self.getProperty("LowResPeakROI").value,
            force_bck_roi=self.getProperty("ForceBckROI").value,
            bck_roi=self.getProperty("BckROI").value,
            event_threshold=self.getProperty("EventThreshold").value,
            dirpix_overwrite=self.getProperty("DirectPixelOverwrite").value,
            dangle0_overwrite=self.getProperty("DAngle0Overwrite").value,
        )

        # Store information in logs
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data,
            LogName="calculated_scatt_angle",
            LogText=str(data_info.calculated_scattering_angle),
            LogType="Number",
            LogUnit="degree",
        )
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName="cross_section", LogText=nxs_data_name)
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName="use_roi_actual", LogText=str(data_info.use_roi_actual))
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName="is_direct_beam", LogText=str(data_info.is_direct_beam))
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="tof_range_min", LogText=str(data_info.tof_range[0]), LogType="Number", LogUnit="usec"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="tof_range_max", LogText=str(data_info.tof_range[1]), LogType="Number", LogUnit="usec"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="peak_min", LogText=str(data_info.peak_range[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="peak_max", LogText=str(data_info.peak_range[1]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="background_min", LogText=str(data_info.background[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="background_max", LogText=str(data_info.background[1]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="low_res_min", LogText=str(data_info.low_res_range[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="low_res_max", LogText=str(data_info.low_res_range[1]), LogType="Number", LogUnit="pixel"
        )
        # Add process ROI information
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_peak_min", LogText=str(data_info.roi_peak[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_peak_max", LogText=str(data_info.roi_peak[1]), LogType="Number", LogUnit="pixel"
        )

        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_low_res_min", LogText=str(data_info.roi_low_res[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_low_res_max", LogText=str(data_info.roi_low_res[1]), LogType="Number", LogUnit="pixel"
        )

        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_background_min", LogText=str(data_info.roi_background[0]), LogType="Number", LogUnit="pixel"
        )
        mantid.simpleapi.AddSampleLog(
            Workspace=nxs_data, LogName="roi_background_max", LogText=str(data_info.roi_background[1]), LogType="Number", LogUnit="pixel"
        )


def _as_ints(a):
    return [int(a[0]), int(a[1])]


class DataInfo(object):
    """
    Class to hold the relevant information from a run (scattering or direct beam).
    """

    peak_range_offset = 0
    tolerance = 0.02

    def __init__(
        self,
        ws,
        cross_section="",
        use_roi=True,
        update_peak_range=False,
        use_roi_bck=False,
        use_tight_bck=False,
        bck_offset=3,
        force_peak_roi=False,
        peak_roi=[0, 0],
        force_low_res_roi=False,
        low_res_roi=[0, 0],
        force_bck_roi=False,
        bck_roi=[0, 0],
        event_threshold=10000,
        dirpix_overwrite=None,
        dangle0_overwrite=None,
    ):
        self.cross_section = cross_section
        self.run_number = ws.getRunNumber()
        self.is_direct_beam = False
        self.data_type = 1
        self.peak_position = 0
        self.peak_range = [0, 0]
        self.low_res_range = [0, 0]
        self.background = [0, 0]
        self.n_events_cutoff = event_threshold
        self.dangle0_overwrite = dangle0_overwrite
        self.dirpix_overwrite = dirpix_overwrite

        # ROI information
        self.roi_peak = [0, 0]
        self.roi_low_res = [0, 0]
        self.roi_background = [0, 0]

        # Options to override the ROI
        self.force_peak_roi = force_peak_roi
        self.forced_peak_roi = _as_ints(peak_roi)
        self.force_low_res_roi = force_low_res_roi
        self.forced_low_res_roi = _as_ints(low_res_roi)
        self.force_bck_roi = force_bck_roi
        self.forced_bck_roi = _as_ints(bck_roi)

        # Peak found before fitting for the central position
        self.found_peak = [0, 0]
        self.found_low_res = [0, 0]

        # Processing options
        # Use the ROI rather than finding the ranges
        self.use_roi = use_roi
        self.use_roi_actual = False

        # Use the 2nd ROI as the background, if available
        self.use_roi_bck = use_roi_bck

        # Use background as a region on each side of the peak
        self.use_tight_bck = use_tight_bck
        # Width of the background on each side of the peak
        self.bck_offset = bck_offset

        # Update the specular peak range after finding the peak
        # within the ROI
        self.update_peak_range = update_peak_range

        self.tof_range = self.get_tof_range(ws)
        self.calculated_scattering_angle = 0.0
        self.theta_d = 0.0
        self.determine_data_type(ws)

    def log(self):
        """
        Log useful diagnostics
        """
        logger.notice("| Run: %s [direct beam: %s]" % (self.run_number, self.is_direct_beam))
        logger.notice("|   Peak position: %s" % self.peak_position)
        logger.notice("|   Reflectivity peak: %s" % str(self.peak_range))
        logger.notice("|   Low-resolution pixel range: %s" % str(self.low_res_range))

    def get_tof_range(self, ws):
        """
        Determine TOF range from the data
        :param workspace ws: workspace to work with
        """
        run_object = ws.getRun()
        sample_detector_distance = run_object["SampleDetDis"].getStatistics().mean
        source_sample_distance = run_object["ModeratorSamDis"].getStatistics().mean
        # Check units
        if run_object["SampleDetDis"].units not in ["m", "meter"]:
            sample_detector_distance /= 1000.0
        if run_object["ModeratorSamDis"].units not in ["m", "meter"]:
            source_sample_distance /= 1000.0

        source_detector_distance = source_sample_distance + sample_detector_distance

        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        wl = run_object.getProperty("LambdaRequest").value[0]
        chopper_speed = run_object.getProperty("SpeedRequest1").value[0]
        wl_offset = 0
        cst = source_detector_distance / h * m
        tof_min = cst * (wl + wl_offset * 60.0 / chopper_speed - 1.4 * 60.0 / chopper_speed) * 1e-4
        tof_max = cst * (wl + wl_offset * 60.0 / chopper_speed + 1.4 * 60.0 / chopper_speed) * 1e-4

        self.tof_range = [tof_min, tof_max]
        return [tof_min, tof_max]

    def process_roi(self, ws):
        """
        Process the ROI information and determine the peak
        range, the low-resolution range, and the background range.

        Starting in June 2018, with the DAS upgrade, the ROIs are
        specified with a start/width rather than start/stop.

        :param workspace ws: workspace to work with
        """
        roi_peak = [0, 0]
        roi_low_res = [0, 0]
        roi_background = [0, 0]

        # Read ROI 1
        roi1_valid = True
        if "ROI1StartX" in ws.getRun():
            roi1_x0 = ws.getRun()["ROI1StartX"].getStatistics().mean
            roi1_y0 = ws.getRun()["ROI1StartY"].getStatistics().mean
            if "ROI1SizeX" in ws.getRun():
                size_x = ws.getRun()["ROI1SizeX"].getStatistics().mean
                size_y = ws.getRun()["ROI1SizeY"].getStatistics().mean
                roi1_x1 = roi1_x0 + size_x
                roi1_y1 = roi1_y0 + size_y
            else:
                roi1_x1 = ws.getRun()["ROI1EndX"].getStatistics().mean
                roi1_y1 = ws.getRun()["ROI1EndY"].getStatistics().mean
            if roi1_x1 > roi1_x0:
                peak1 = [int(roi1_x0), int(roi1_x1)]
            else:
                peak1 = [int(roi1_x1), int(roi1_x0)]
            if roi1_y1 > roi1_y0:
                low_res1 = [int(roi1_y0), int(roi1_y1)]
            else:
                low_res1 = [int(roi1_y1), int(roi1_y0)]
            if peak1 == [0, 0] and low_res1 == [0, 0]:
                roi1_valid = False

            # Read ROI 2
            if "ROI2StartX" in ws.getRun():
                roi2_valid = True
                roi2_x0 = ws.getRun()["ROI2StartX"].getStatistics().mean
                roi2_y0 = ws.getRun()["ROI2StartY"].getStatistics().mean
                if "ROI2SizeX" in ws.getRun():
                    size_x = ws.getRun()["ROI2SizeX"].getStatistics().mean
                    size_y = ws.getRun()["ROI2SizeY"].getStatistics().mean
                    roi2_x1 = roi2_x0 + size_x
                    roi2_y1 = roi2_y0 + size_y
                else:
                    roi2_x1 = ws.getRun()["ROI2EndX"].getStatistics().mean
                    roi2_y1 = ws.getRun()["ROI2EndY"].getStatistics().mean
                if roi2_x1 > roi2_x0:
                    peak2 = [int(roi2_x0), int(roi2_x1)]
                else:
                    peak2 = [int(roi2_x1), int(roi2_x0)]
                if roi2_y1 > roi2_y0:
                    low_res2 = [int(roi2_y0), int(roi2_y1)]
                else:
                    low_res2 = [int(roi2_y1), int(roi2_y0)]
                if peak2 == [0, 0] and low_res2 == [0, 0]:
                    roi2_valid = False
            else:
                roi2_valid = False
        else:
            roi1_valid = False
            roi2_valid = False

        # Pick the ROI that describes the reflectivity peak
        if roi1_valid and not roi2_valid:
            roi_peak = peak1
            roi_low_res = low_res1
            roi_background = [0, 0]
        elif roi2_valid and not roi1_valid:
            roi_peak = peak2
            roi_low_res = low_res2
            roi_background = [0, 0]
        elif roi1_valid and roi2_valid:
            # If ROI 2 is within ROI 1, treat it as the peak,
            # otherwise, use ROI 1
            if peak1[0] >= peak2[0] and peak1[1] <= peak2[1]:
                roi_peak = peak1
                roi_low_res = low_res1
                roi_background = peak2
            elif peak2[0] >= peak1[0] and peak2[1] <= peak1[1]:
                roi_peak = peak2
                roi_low_res = low_res2
                roi_background = peak1
            else:
                roi_peak = peak1
                roi_low_res = low_res1
                roi_background = [0, 0]

        # After all this, update the ROI according to reduction options
        self.roi_peak = roi_peak
        self.roi_low_res = roi_low_res
        self.roi_background = roi_background

        if self.force_peak_roi:
            logger.notice("Forcing peak ROI: %s" % self.forced_peak_roi)
            self.roi_peak = self.forced_peak_roi
        if self.force_low_res_roi:
            logger.notice("Forcing low-res ROI: %s" % self.forced_low_res_roi)
            self.roi_low_res = self.forced_low_res_roi
        if self.force_bck_roi:
            logger.notice("Forcing background ROI: %s" % self.forced_bck_roi)
            self.roi_background = self.forced_bck_roi

    def check_direct_beam(self, ws, peak_position=None):
        """
        Determine whether this data is a direct beam
        :param workspace ws: Workspace to inspect
        :param float peak_position: reflectivity peak position
        """
        self.theta_d = 180.0 / math.pi * mantid.simpleapi.MRGetTheta(ws, SpecularPixel=peak_position, UseSANGLE=False)
        return not self.theta_d > self.tolerance

    def determine_data_type(self, ws):
        """
        Inspect the data and determine peak locations
        and data type.
        :param workspace ws: Workspace to inspect
        """
        # Skip empty data entries
        if ws.getNumberEvents() < self.n_events_cutoff:
            self.data_type = -1
            logger.notice("No data for %s %s" % (self.run_number, self.cross_section))
            return

        # Find reflectivity peak and low resolution ranges
        peak, low_res = fit_2d_peak(ws)
        if self.use_tight_bck:
            bck_range = [int(max(0.0, peak[0] - self.bck_offset)), int(min(NX_PIXELS, peak[1] + self.bck_offset))]
        else:
            bck_range = [int(max(0.0, peak[0] - 2 * self.bck_offset)), int(max(0.0, peak[0] - self.bck_offset))]
        self.found_peak = copy.copy(peak)
        self.found_low_res = copy.copy(low_res)
        logger.notice("Run %s [%s]: Peak found %s" % (self.run_number, self.cross_section, peak))
        logger.notice("Run %s [%s]: Low-res found %s" % (self.run_number, self.cross_section, str(low_res)))

        # Process the ROI information
        try:
            self.process_roi(ws)
        except:
            logger.notice("Could not process ROI\n%s" % sys.exc_info()[1])

        # Keep track of whether we actually used the ROI
        self.use_roi_actual = False

        # If we were asked to use the ROI but no peak is in it, use the peak we found
        # If we were asked to use the ROI and there's a peak in it, use the ROI
        if self.use_roi and not self.update_peak_range and not self.roi_peak == [0, 0]:
            logger.notice("Using ROI peak range: [%s %s]" % (self.roi_peak[0], self.roi_peak[1]))
            self.use_roi_actual = True
            peak = copy.copy(self.roi_peak)
            if not self.roi_low_res == [0, 0]:
                low_res = copy.copy(self.roi_low_res)
            if not self.roi_background == [0, 0]:
                bck_range = copy.copy(self.roi_background)
        elif self.use_roi and self.update_peak_range and not self.roi_peak == [0, 0]:
            logger.notice("Using fit peak range: [%s %s]" % (peak[0], peak[1]))
            if not self.roi_low_res == [0, 0]:
                low_res = copy.copy(self.roi_low_res)
            if not self.roi_background == [0, 0]:
                bck_range = copy.copy(self.roi_background)

        # Store the information we found
        self.peak_position = (peak[1] + peak[0]) / 2.0
        self.peak_range = [int(max(0, peak[0])), int(min(peak[1], NX_PIXELS))]
        self.low_res_range = [int(max(0, low_res[0])), int(min(low_res[1], NY_PIXELS))]
        self.background = [int(max(0, bck_range[0])), int(min(bck_range[1], NY_PIXELS))]

        # Computed scattering angle
        self.calculated_scattering_angle = mantid.simpleapi.MRGetTheta(
            ws, SpecularPixel=self.peak_position, DirectPixelOverwrite=self.dirpix_overwrite, DAngle0Overwrite=self.dangle0_overwrite
        )
        self.calculated_scattering_angle *= 180.0 / math.pi

        # Determine whether we have a direct beam
        self.is_direct_beam = self.check_direct_beam(ws, self.peak_position)

        # Convenient data type
        self.data_type = 0 if self.is_direct_beam else 1

        # Write to logs
        self.log()


def fit_2d_peak(workspace):
    """
    Fit a 2D Gaussian peak
    :param workspace: workspace to work with
    """
    n_x = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
    n_y = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])

    # Prepare data to fit
    _integrated = mantid.simpleapi.Integration(InputWorkspace=workspace)
    signal = _integrated.extractY()
    z = np.reshape(signal, (n_x, n_y))
    x = np.arange(0, n_x)
    y = np.arange(0, n_y)
    _x, _y = np.meshgrid(x, y)
    _x = _x.T
    _y = _y.T

    code = coord_to_code(_x, _y).ravel()
    data_to_fit = z.ravel()
    err_y = np.sqrt(np.fabs(data_to_fit))
    err_y[err_y < 1] = 1

    # Use the highest data point as a starting point for a simple Gaussian fit
    x_dist = np.sum(z, 1)
    y_dist = np.sum(z, 0)
    center_x = np.argmax(x_dist)
    center_y = np.argmax(y_dist)

    # Gaussian fit
    p0 = [np.max(z), center_x, 5, center_y, 50, 0]
    try:
        gauss_coef, _ = opt.curve_fit(gauss_simple, code, data_to_fit, p0=p0, sigma=err_y)
    except:
        logger.notice("Could not fit simple Gaussian")
        gauss_coef = p0

    # Keep track of the result
    th = gauss_simple(code, *gauss_coef)
    th = np.reshape(th, (n_x, n_y))
    _chi2 = chi2(th, z)
    guess_x = gauss_coef[1]
    guess_wx = 2.0 * gauss_coef[2]
    guess_y = gauss_coef[3]
    guess_wy = 2.0 * gauss_coef[4]
    guess_chi2 = _chi2

    # Fit a polynomial background, as a starting point to fitting signal + background
    try:
        step_coef, _ = opt.curve_fit(poly_bck, code, data_to_fit, p0=[0, 0, 0, center_x, 0], sigma=err_y)
    except:
        logger.notice("Could not fit polynomial background")
        step_coef = [0, 0, 0, center_x, 0]
    th = poly_bck(code, *step_coef)
    th = np.reshape(th, (n_x, n_y))

    # Now fit a Gaussian + background
    # A, mu_x, sigma_x, mu_y, sigma_y, poly_a, poly_b, poly_c, center, background
    coef = [np.max(z), center_x, 5, center_y, 50, step_coef[0], step_coef[1], step_coef[2], step_coef[3], step_coef[4]]
    try:
        coef, _ = opt.curve_fit(poly_bck_signal, code, data_to_fit, p0=coef, sigma=err_y)
    except:
        logger.notice("Could not fit Gaussian + polynomial")
    th = poly_bck_signal(code, *coef)
    th = np.reshape(th, (n_x, n_y))
    _chi2 = chi2(th, z)
    if _chi2 < guess_chi2:
        guess_x = coef[1]
        guess_wx = 2.0 * coef[2]
        guess_y = coef[3]
        guess_wy = 2.0 * coef[4]
        guess_chi2 = _chi2

    # Package the best results
    x_min = max(0, int(guess_x - np.fabs(guess_wx)))
    x_max = min(n_x - 1, int(guess_x + np.fabs(guess_wx)))
    y_min = max(0, int(guess_y - np.fabs(guess_wy)))
    y_max = min(n_y - 1, int(guess_y + np.fabs(guess_wy)))

    return [x_min, x_max], [y_min, y_max]


def coord_to_code(x, y):
    """Utility function to encode pixel coordinates so we can unravel our distribution in a 1D array"""
    return 1000 * x + y


def code_to_coord(c):
    """Utility function to decode encoded coordinates"""
    i_x = c / 1000
    i_y = c % 1000
    return i_x, i_y


def poly_bck(value, *p):
    """
    Polynomial function for background fit

    f = a + b*(x-center) + c*(x-center)**2 + bck

    where bck is a minimum threshold that is zero when the polynomial
    has a value greater than it.
    """
    coord = code_to_coord(value)
    poly_a, poly_b, poly_c, center, background = p
    values = poly_a + poly_b * (coord[0] - center) + poly_c * (coord[0] - center) ** 2
    values[values < background] = background
    values[coord[0] < DEAD_PIXELS] = 0
    values[coord[0] >= NX_PIXELS - DEAD_PIXELS] = 0
    values[coord[1] < DEAD_PIXELS] = 0
    values[coord[1] >= NY_PIXELS - DEAD_PIXELS] = 0
    return values


def poly_bck_signal(value, *p):
    """
    Function for a polynomial + Gaussian signal

    f = a + b*(x-center) + c*(x-center)**2 + Gaussian(x) + bck

    where bck is a minimum threshold that is zero when the polynomial+Gaussian
    has a value greater than it.
    """
    coord = code_to_coord(value)
    A, mu_x, sigma_x, mu_y, sigma_y, poly_a, poly_b, poly_c, center, background = p
    if A < 0 or sigma_x > 50:
        return np.zeros(len(value))

    values = poly_a + poly_b * (coord[0] - center) + poly_c * (coord[0] - center) ** 2
    values_g = A * np.exp(-((coord[0] - mu_x) ** 2) / (2.0 * sigma_x**2) - (coord[1] - mu_y) ** 2 / (2.0 * sigma_y**2))
    values += values_g
    values[values < background] = background
    values[coord[0] < DEAD_PIXELS] = 0
    values[coord[0] >= NX_PIXELS - DEAD_PIXELS] = 0
    values[coord[1] < DEAD_PIXELS] = 0
    values[coord[1] >= NY_PIXELS - DEAD_PIXELS] = 0
    return values


def gauss_simple(value, *p):
    """
    Gaussian function with threshold background

    f = Gaussian(x) + bck

    where bck is a minimum threshold that is zero when the Gaussian
    has a value greater than it.
    """
    coord = code_to_coord(value)
    A, mu_x, sigma_x, mu_y, sigma_y, background = p
    if A < 0 or sigma_x > 50:
        return np.zeros(len(value))
    values = A * np.exp(-((coord[0] - mu_x) ** 2) / (2.0 * sigma_x**2) - (coord[1] - mu_y) ** 2 / (2.0 * sigma_y**2))
    values[values < background] = background
    values[coord[0] < DEAD_PIXELS] = 0
    values[coord[0] >= NX_PIXELS - DEAD_PIXELS] = 0
    values[coord[1] < DEAD_PIXELS] = 0
    values[coord[1] >= NY_PIXELS - DEAD_PIXELS] = 0
    return values


def chi2(data, model):
    """Returns the chi^2 for a data set and model pair"""
    err = np.fabs(data.ravel())
    err[err <= 0] = 1
    return np.sum((data.ravel() - model.ravel()) ** 2 / err) / len(data.ravel())


# Register
AlgorithmFactory.subscribe(MRInspectData)
