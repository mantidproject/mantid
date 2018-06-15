#pylint: disable=bare-except,no-init,invalid-name,dangerous-default-value
from __future__ import (absolute_import, division, print_function)
import sys
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi
import math
import copy
import numpy as np
from scipy.optimize import curve_fit


class MRInspectData(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "MRInspectData"

    def summary(self):
        return "This algorithm inspects Magnetism Reflectometer data and populates meta-data."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("Workspace", "", Direction.Input),
                             "Input workspace")

        # Peak finding options
        self.declareProperty("UseROI", True,
                             doc="If true, use the meta-data ROI rather than finding the ranges")
        self.declareProperty("UpdatePeakRange", False,
                             doc="If true, a fit will be performed and the peak ranges will be updated")
        self.declareProperty("UseROIBck", False,
                             doc="If true, use the 2nd ROI in the meta-data for the background")
        self.declareProperty("UseTightBck", False,
                             doc="If true, use the area on each side of the peak to compute the background")
        self.declareProperty("BckWidth", 3,
                             doc="If UseTightBck is true, width of the background on each side of the peak")
        self.declareProperty("HuberXCut", 0.0,
                             doc="Provide a Huber X value above which a run will be considered a direct beam")

        self.declareProperty("ForcePeakROI", False,
                             doc="If true, use the PeakROI property as the ROI")
        self.declareProperty(IntArrayProperty("PeakROI", [0, 0],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the reflectivity peak")
        self.declareProperty("ForceLowResPeakROI", False,
                             doc="If true, use the LowResPeakROI property as the ROI")
        self.declareProperty(IntArrayProperty("LowResPeakROI", [0, 0],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the low-resolution peak")
        self.declareProperty("ForceBckROI", False,
                             doc="If true, use the BckROI property as the ROI")
        self.declareProperty(IntArrayProperty("BckROI", [0, 0],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background")
        self.declareProperty("EventThreshold", 10000,
                             "Minimum number of events needed to call a data set a valid direct beam")

    def PyExec(self):
        nxs_data = self.getProperty("Workspace").value
        nxs_data_name = self.getPropertyValue("Workspace")
        data_info = DataInfo(nxs_data, cross_section=nxs_data_name,
                             use_roi=self.getProperty("UseROI").value,
                             update_peak_range=self.getProperty("UpdatePeakRange").value,
                             use_roi_bck=self.getProperty("UseROIBck").value,
                             use_tight_bck=self.getProperty("UseTightBck").value,
                             bck_offset=self.getProperty("BckWidth").value,
                             huber_x_cut=self.getProperty("HuberXCut").value,
                             force_peak_roi=self.getProperty("ForcePeakROI").value,
                             peak_roi=self.getProperty("PeakROI").value,
                             force_low_res_roi=self.getProperty("ForceLowResPeakROI").value,
                             low_res_roi=self.getProperty("LowResPeakROI").value,
                             force_bck_roi=self.getProperty("ForceBckROI").value,
                             bck_roi=self.getProperty("BckROI").value,
                             event_threshold=self.getProperty("EventThreshold").value)

        # Store information in logs
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='calculated_scatt_angle',
                                      LogText=str(data_info.calculated_scattering_angle),
                                      LogType='Number', LogUnit='degree')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='cross_section',
                                      LogText=nxs_data_name)
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='use_roi_actual',
                                      LogText=str(data_info.use_roi_actual))
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='is_direct_beam',
                                      LogText=str(data_info.is_direct_beam))
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='tof_range_min',
                                      LogText=str(data_info.tof_range[0]),
                                      LogType='Number', LogUnit='usec')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='tof_range_max',
                                      LogText=str(data_info.tof_range[1]),
                                      LogType='Number', LogUnit='usec')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='peak_min',
                                      LogText=str(data_info.peak_range[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='peak_max',
                                      LogText=str(data_info.peak_range[1]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='background_min',
                                      LogText=str(data_info.background[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='background_max',
                                      LogText=str(data_info.background[1]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='low_res_min',
                                      LogText=str(data_info.low_res_range[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='low_res_max',
                                      LogText=str(data_info.low_res_range[1]),
                                      LogType='Number', LogUnit='pixel')
        # Add process ROI information
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_peak_min',
                                      LogText=str(data_info.roi_peak[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_peak_max',
                                      LogText=str(data_info.roi_peak[1]),
                                      LogType='Number', LogUnit='pixel')

        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_low_res_min',
                                      LogText=str(data_info.roi_low_res[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_low_res_max',
                                      LogText=str(data_info.roi_low_res[1]),
                                      LogType='Number', LogUnit='pixel')

        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_background_min',
                                      LogText=str(data_info.roi_background[0]),
                                      LogType='Number', LogUnit='pixel')
        mantid.simpleapi.AddSampleLog(Workspace=nxs_data, LogName='roi_background_max',
                                      LogText=str(data_info.roi_background[1]),
                                      LogType='Number', LogUnit='pixel')


def _as_ints(a): return [int(a[0]), int(a[1])]


class DataInfo(object):
    """
        Class to hold the relevant information from a run (scattering or direct beam).
    """
    n_x_pixel = 304
    n_y_pixel = 256
    peak_range_offset = 0
    tolerance = 0.02
    huber_x_cut = 4.95

    def __init__(self, ws, cross_section='', use_roi=True, update_peak_range=False, use_roi_bck=False,
                 use_tight_bck=False, bck_offset=3, huber_x_cut=4.95,
                 force_peak_roi=False, peak_roi=[0,0],
                 force_low_res_roi=False, low_res_roi=[0,0],
                 force_bck_roi=False, bck_roi=[0,0], event_threshold=10000):
        self.cross_section = cross_section
        self.run_number = ws.getRunNumber()
        self.is_direct_beam = False
        self.data_type = 1
        self.peak_position = 0
        self.peak_range = [0,0]
        self.low_res_range = [0,0]
        self.background = [0,0]
        self.huber_x_cut = huber_x_cut
        self.n_events_cutoff = event_threshold

        # ROI information
        self.roi_peak = [0,0]
        self.roi_low_res = [0,0]
        self.roi_background = [0,0]

        # Options to override the ROI
        self.force_peak_roi = force_peak_roi
        self.forced_peak_roi = _as_ints(peak_roi)
        self.force_low_res_roi = force_low_res_roi
        self.forced_low_res_roi = _as_ints(low_res_roi)
        self.force_bck_roi = force_bck_roi
        self.forced_bck_roi = _as_ints(bck_roi)

        # Peak found before fitting for the central position
        self.found_peak = [0,0]
        self.found_low_res = [0,0]

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
        sample_detector_distance = run_object['SampleDetDis'].getStatistics().mean
        source_sample_distance = run_object['ModeratorSamDis'].getStatistics().mean
        # Check units
        if not run_object['SampleDetDis'].units in ['m', 'meter']:
            sample_detector_distance /= 1000.0
        if not run_object['ModeratorSamDis'].units in ['m', 'meter']:
            source_sample_distance /= 1000.0

        source_detector_distance = source_sample_distance + sample_detector_distance

        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        wl = run_object.getProperty('LambdaRequest').value[0]
        chopper_speed = run_object.getProperty('SpeedRequest1').value[0]
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
            :param workspace ws: workspace to work with
        """
        roi_peak = [0,0]
        roi_low_res = [0,0]
        roi_background = [0,0]

        # Read ROI 1
        roi1_valid = True
        if 'ROI1StartX' in ws.getRun():
            roi1_x0 = ws.getRun()['ROI1StartX'].getStatistics().mean
            roi1_y0 = ws.getRun()['ROI1StartY'].getStatistics().mean
            roi1_x1 = ws.getRun()['ROI1EndX'].getStatistics().mean
            roi1_y1 = ws.getRun()['ROI1EndY'].getStatistics().mean
            if roi1_x1 > roi1_x0:
                peak1 = [int(roi1_x0), int(roi1_x1)]
            else:
                peak1 = [int(roi1_x1), int(roi1_x0)]
            if roi1_y1 > roi1_y0:
                low_res1 = [int(roi1_y0), int(roi1_y1)]
            else:
                low_res1 = [int(roi1_y1), int(roi1_y0)]
            if peak1 == [0,0] and low_res1 == [0,0]:
                roi1_valid = False

            # Read ROI 2
            roi2_valid = True
            roi2_x0 = ws.getRun()['ROI2StartX'].getStatistics().mean
            roi2_y0 = ws.getRun()['ROI2StartY'].getStatistics().mean
            roi2_x1 = ws.getRun()['ROI2EndX'].getStatistics().mean
            roi2_y1 = ws.getRun()['ROI2EndY'].getStatistics().mean
            if roi2_x1 > roi2_x0:
                peak2 = [int(roi2_x0), int(roi2_x1)]
            else:
                peak2 = [int(roi2_x1), int(roi2_x0)]
            if roi2_y1 > roi2_y0:
                low_res2 = [int(roi2_y0), int(roi2_y1)]
            else:
                low_res2 = [int(roi2_y1), int(roi2_y0)]
            if peak2 == [0,0] and low_res2 == [0,0]:
                roi2_valid = False
        else:
            roi1_valid = False
            roi2_valid = False

        # Pick the ROI that describes the reflectivity peak
        if roi1_valid and not roi2_valid:
            roi_peak = peak1
            roi_low_res = low_res1
            roi_background = [0,0]
        elif roi2_valid and not roi1_valid:
            roi_peak = peak2
            roi_low_res = low_res2
            roi_background = [0,0]
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
                roi_background = [0,0]

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

    def determine_peak_range(self, ws, specular=True, max_pixel=304):
        """
            Find the reflectivity peak
            :param workspace ws: workspace to work with
            :param bool specular: if True, we are looking for a specular peak
            :param int max_pixel: max pixel above which to exclude peaks
        """
        ws_summed = mantid.simpleapi.RefRoi(InputWorkspace=ws, IntegrateY=specular,
                                            NXPixel=self.n_x_pixel, NYPixel=self.n_y_pixel,
                                            ConvertToQ=False,
                                            OutputWorkspace="ws_summed")

        integrated = mantid.simpleapi.Integration(ws_summed)
        integrated = mantid.simpleapi.Transpose(integrated)

        x_values = integrated.readX(0)
        y_values = integrated.readY(0)
        e_values = integrated.readE(0)
        ws_short = mantid.simpleapi.CreateWorkspace(DataX=x_values[self.peak_range_offset:max_pixel],
                                                    DataY=y_values[self.peak_range_offset:max_pixel],
                                                    DataE=e_values[self.peak_range_offset:max_pixel])
        try:
            specular_peak, low_res, _ = mantid.simpleapi.LRPeakSelection(InputWorkspace=ws_short)
        except:
            logger.notice("Peak finding error [specular=%s]: %s" % (specular, sys.exc_info()[1]))
            return integrated, [0,0], [0,0]
        if specular:
            peak = [specular_peak[0]+self.peak_range_offset, specular_peak[1]+self.peak_range_offset]
        else:
            # The low-resolution range finder tends to be a bit tight.
            # Broaden it by a third.
            #TODO: Fix the range finder algorithm
            broadening = (low_res[1]-low_res[0])/3.0
            peak = [low_res[0]+self.peak_range_offset-broadening,
                    low_res[1]+self.peak_range_offset+broadening]

        mantid.simpleapi.DeleteWorkspace(ws_short)
        mantid.simpleapi.DeleteWorkspace(ws_summed)

        return integrated, peak, [low_res[0]+self.peak_range_offset, low_res[1]+self.peak_range_offset]

    @classmethod
    def fit_peak(cls, signal_x, signal_y, peak):
        """
            Fit a Gaussian peak to a curve
            :param array signal_x: list of x values
            :param array signal_y: list of y values
            :param array peak: initial guess for the peak (one-sigma min and max)
        """
        def gauss(x, *p):
            A, mu, sigma, bck = p
            if A < 0 or sigma < 5:
                return -np.inf
            return A*np.exp(-(x-mu)**2/(2.*sigma**2)) + bck

        p0 = [np.max(signal_y), (peak[1]+peak[0])/2.0, (peak[1]-peak[0])/2.0, 0.0]
        err_y = np.sqrt(np.fabs(signal_y))
        # Using bounds would be great but only available with scipy>=0.17. bounds=(0, np.inf)
        coeff, _ = curve_fit(gauss, signal_x, signal_y, sigma=err_y, p0=p0)
        peak_position = coeff[1]
        peak_width = math.fabs(3.0*coeff[2])
        return peak_position, peak_width

    def check_direct_beam(self, ws, peak_position=None):
        """
            Determine whether this data is a direct beam
            :param workspace ws: Workspace to inspect
            :param float peak_position: reflectivity peak position
        """
        huber_x = ws.getRun().getProperty("HuberX").getStatistics().mean
        sangle = ws.getRun().getProperty("SANGLE").getStatistics().mean
        self.theta_d = 180.0 / math.pi * mantid.simpleapi.MRGetTheta(ws, SpecularPixel=peak_position)
        return not ((self.theta_d > self.tolerance or sangle > self.tolerance) and huber_x < self.huber_x_cut)

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
        # Those will be our defaults
        integrated, peak, broad_range = self.determine_peak_range(ws, specular=True)
        self.found_peak = copy.copy(peak)
        logger.notice("Run %s [%s]: Peak found %s" % (self.run_number, self.cross_section, peak))
        signal_y = integrated.readY(0)
        mantid.simpleapi.DeleteWorkspace(integrated)
        signal_x = range(len(signal_y))

        _, low_res, _ = self.determine_peak_range(ws, specular=False)
        logger.notice("Run %s [%s]: Low-res found %s" %(self.run_number, self.cross_section, str(low_res)))
        self.found_low_res = low_res
        bck_range = None

        # Process the ROI information
        self.process_roi(ws)

        # Keep track of whether we actually used the ROI
        self.use_roi_actual = False

        if self.use_roi and not self.roi_peak == [0,0]:
            peak = copy.copy(self.roi_peak)
            if not self.roi_low_res == [0,0]:
                low_res = copy.copy(self.roi_low_res)
            if not self.roi_background == [0,0]:
                bck_range = copy.copy(self.roi_background)
            logger.notice("Using ROI peak range: [%s %s]" % (peak[0], peak[1]))
            self.use_roi_actual = True

        # Determine reflectivity peak position (center)
        signal_y_crop = signal_y[peak[0]:peak[1]+1]
        signal_x_crop = signal_x[peak[0]:peak[1]+1]

        # Calculate a reasonable peak position
        #peak_mean = np.average(signal_x_crop, weights=signal_y_crop)

        peak_position = (peak[1]+peak[0])/2.0
        peak_width = (peak[1]-peak[0])/2.0
        try:
            # Try to find the peak position within the peak range we found
            peak_position, peak_width = self.fit_peak(signal_x_crop, signal_y_crop, peak)
            # If we are more than two sigmas away from the middle of the range,
            # there's clearly a problem.
            if np.abs(peak_position - (peak[1]+peak[0])/2.0)  > np.abs(peak[1]-peak[0]):
                logger.notice("Found peak position outside of given range [x=%s], switching to full detector" % peak_position)
                peak_position = (peak[1]+peak[0])/2.0
                peak_width = (peak[1]-peak[0])/2.0
                raise RuntimeError("Bad peak position")
        except:
            # If we can't find a peak, try fitting over the full detector.
            # If we do find a peak, then update the ranges rather than using
            # what we currently have (which is probably given by the ROI).
            logger.notice("Run %s [%s]: Could not fit a peak in the supplied peak range" %
                          (self.run_number, self.cross_section))
            logger.notice(str(sys.exc_info()[1]))
            try:
                # Define a good default that is wide enough for the fit to work
                default_width = (self.found_peak[1]-self.found_peak[0])/2.0
                default_width = max(default_width, 10.0)
                default_center = (self.found_peak[1]+self.found_peak[0])/2.0
                default_peak = [default_center-default_width, default_center+default_width]
                logger.notice("Run %s [%s]: Broad data region %s" % (self.run_number, self.cross_section, broad_range))
                x_min = broad_range[0]+10
                x_max = broad_range[1]-10
                peak_position, peak_width = self.fit_peak(signal_x[x_min:x_max], signal_y[x_min:x_max], default_peak)
                peak = [math.floor(peak_position-peak_width), math.floor(peak_position+peak_width)]
                #low_res = [5, self.n_x_pixel-5]
                low_res = self.found_low_res
                self.use_roi_actual = False
                logger.notice("Run %s [%s]: Peak not in supplied range! Found peak: %s low: %s" %
                              (self.run_number, self.cross_section, peak, low_res))
                logger.notice("Run %s [%s]: Peak position: %s  Peak width: %s" %
                              (self.run_number, self.cross_section, peak_position, peak_width))
            except:
                logger.notice(str(sys.exc_info()[1]))
                logger.notice("Run %s [%s]: Gaussian fit failed to determine peak position" %
                              (self.run_number, self.cross_section))

        # Update the specular peak range if needed
        if self.update_peak_range:
            peak[0] = math.floor(peak_position-peak_width)
            peak[1] = math.ceil(peak_position+peak_width)
            logger.notice("Updating peak range to: [%s %s]" % (peak[0], peak[1]))
            self.use_roi_actual = False

        # Store the information we found
        self.peak_position = peak_position
        self.peak_range = [int(peak[0]), int(peak[1])]
        self.low_res_range = [int(low_res[0]), int(low_res[1])]

        if not self.use_roi_bck or bck_range is None:
            if self.use_tight_bck:
                self.background = [self.peak_range[0]-self.bck_offset, self.peak_range[1]+self.bck_offset]
            else:
                self.background = [4, self.peak_range[0]-30]
        else:
            self.background = [int(bck_range[0]), int(bck_range[1])]

        # Computed scattering angle
        self.calculated_scattering_angle = 180.0 / math.pi * mantid.simpleapi.MRGetTheta(ws, SpecularPixel=peak_position)

        # Determine whether we have a direct beam
        self.is_direct_beam = self.check_direct_beam(ws, peak_position)

        # Convenient data type
        self.data_type = 0 if self.is_direct_beam else 1

        # Write to logs
        self.log()


# Register
AlgorithmFactory.subscribe(MRInspectData)
