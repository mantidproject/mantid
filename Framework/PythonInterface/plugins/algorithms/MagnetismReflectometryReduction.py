#pylint: disable=no-init,invalid-name
"""
    Magnetism reflectometry reduction
"""
from __future__ import (absolute_import, division, print_function)
import math
import functools
import numpy as np
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *

INSTRUMENT_NAME = "REF_M"


class MagnetismReflectometryReduction(PythonAlgorithm):
    """
        Workflow algorithm to reduce REF_M data
    """
    number_of_pixels_x=0
    number_of_pixels_y=0
    _tof_range = None

    def category(self):
        """ Algorithm category """
        return "Reflectometry\\SNS"

    def name(self):
        """ Algorithm name """
        return "MagnetismReflectometryReduction"

    def version(self):
        """ Version number """
        return 1

    def summary(self):
        """ Friendly description """
        return "Magnetism Reflectometer (REFM) reduction"

    def PyInit(self):
        """ Initialization """
        self.declareProperty(StringArrayProperty("RunNumbers"), "List of run numbers to process")
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the data peak")
        self.declareProperty("SubtractSignalBackground", True,
                             doc='If true, the background will be subtracted from the data peak')
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background. Default:(123,137)")
        self.declareProperty("ApplyNormalization", True, doc="If true, the data will be normalized")
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127, 133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the normalization peak")
        self.declareProperty("SubtractNormBackground", True,
                             doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background for the normalization")
        self.declareProperty("CutLowResDataAxis", True,
                             doc="If true, the low resolution direction of the data will be cropped according "+
                             "to the lowResDataAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the data")
        self.declareProperty("CutLowResNormAxis", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped "+
                             "according to the LowResNormAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the normalizaion run")
        self.declareProperty(FloatArrayProperty("TimeAxisRange", [0., 340000.],
                                                FloatArrayLengthValidator(2), direction=Direction.Input),
                             "TOF/wavelength range to use with detector binning")
        self.declareProperty("UseWLTimeAxis", False,
                             doc="For const-Q, if true, wavelength will be used as the time axis, otherwise TOF is used")
        self.declareProperty("CutTimeAxis", True,
                             doc="If true, the TOF/wavelength dimension will be cropped according to the TimeAxisRange property")
        self.declareProperty("RoundUpPixel", True, doc="If True, round up pixel position of the specular reflectivity")
        self.declareProperty("UseSANGLE", False, doc="If True, use SANGLE as the scattering angle")
        self.declareProperty("SpecularPixel", 180.0, doc="Pixel position of the specular reflectivity")
        self.declareProperty("QMin", 0.005, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("TimeAxisStep", 40.0,
                             doc="Binning step size for the time axis. TOF for detector binning, wavelength for constant Q")
        self.declareProperty("EntryName", "entry-Off_Off", doc="Name of the entry to load")
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")
        self.declareProperty("ConstQTrim", 0.5,
                             doc="With const-Q binning, cut Q bins with contributions fewer than ConstQTrim of WL bins")
        self.declareProperty("ConstantQBinning", False, doc="If true, we convert to Q before summing")

    #pylint: disable=too-many-locals
    def PyExec(self):
        """ Main execution """
        # DATA
        dataRunNumbers = self.getProperty("RunNumbers").value
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # NORMALIZATION
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value

        # If we have multiple files, add them
        file_list = []
        for item in dataRunNumbers:
            # The standard mode of operation is to give a run number as input
            try:
                data_file = FileFinder.findRuns("%s%s" % (INSTRUMENT_NAME, item))[0]
            except RuntimeError:
                # Allow for a file name or file path as input
                data_file = FileFinder.findRuns(item)[0]
            file_list.append(data_file)
        runs = functools.reduce((lambda x, y: '%s+%s' % (x, y)), file_list)

        entry_name = self.getProperty("EntryName").value
        ws_event_data = LoadEventNexus(Filename=runs, NXentryName=entry_name,
                                       OutputWorkspace="%s_%s" % (INSTRUMENT_NAME, dataRunNumbers[0]))

        # Number of pixels in each direction
        self.number_of_pixels_x = int(ws_event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        self.number_of_pixels_y = int(ws_event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])

        # ----- Process Sample Data -------------------------------------------
        crop_request = self.getProperty("CutLowResDataAxis").value
        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        bck_request = self.getProperty("SubtractSignalBackground").value
        data_cropped = self.process_data(ws_event_data,
                                         crop_request, low_res_range,
                                         dataPeakRange, bck_request, dataBackRange)

        # ----- Normalization -------------------------------------------------
        perform_normalization = self.getProperty("ApplyNormalization").value
        if perform_normalization:
            # Load normalization
            ws_event_norm = self.load_direct_beam(normalizationRunNumber)
            crop_request = self.getProperty("CutLowResNormAxis").value
            low_res_range = self.getProperty("LowResNormAxisPixelRange").value
            bck_request = self.getProperty("SubtractNormBackground").value
            norm_cropped = self.process_data(ws_event_norm,
                                             crop_request, low_res_range,
                                             normPeakRange, bck_request, normBackRange)
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(ws_event_norm))

            # Sum up the normalization peak
            norm_summed = SumSpectra(InputWorkspace = norm_cropped)
            norm_summed = RebinToWorkspace(WorkspaceToRebin=norm_summed,
                                           WorkspaceToMatch=data_cropped,
                                           OutputWorkspace=str(norm_summed))

            # Normalize the data
            normalized_data = data_cropped / norm_summed

            AddSampleLog(Workspace=normalized_data, LogName='normalization_run', LogText=str(normalizationRunNumber))
            AddSampleLog(Workspace=normalized_data, LogName='normalization_file_path',
                         LogText=norm_summed.getRun().getProperty("Filename").value)
            norm_dirpix = norm_summed.getRun().getProperty('DIRPIX').getStatistics().mean
            AddSampleLog(Workspace=normalized_data, LogName='normalization_dirpix',
                         LogText=str(norm_dirpix), LogType='Number', LogUnit='pixel')

            # Avoid leaving trash behind
            AnalysisDataService.remove(str(data_cropped))
            AnalysisDataService.remove(str(norm_cropped))
            AnalysisDataService.remove(str(norm_summed))
        else:
            normalized_data = data_cropped
            AddSampleLog(Workspace=normalized_data, LogName='normalization_run', LogText="None")

        # At this point, the workspace should be considered a distribution of points
        normalized_data = ConvertToPointData(InputWorkspace=normalized_data,
                                             OutputWorkspace=str(normalized_data))

        # Convert to Q and clean up the distribution
        constant_q_binning = self.getProperty("ConstantQBinning").value
        if constant_q_binning:
            q_rebin = self.constant_q(normalized_data, dataPeakRange)
        else:
            q_rebin = self.convert_to_q(normalized_data)
        q_rebin = self.cleanup_reflectivity(q_rebin)

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(normalized_data))

        self.setProperty('OutputWorkspace', q_rebin)

    def load_direct_beam(self, normalizationRunNumber):
        """
            Load a direct beam file. This method will disappear once we move to the new DAS.
            It is necessary at the moment only because the direct beam data is occasionally
            stored in another entry than entry_Off-Off.
        """
        for entry in ['entry', 'entry-Off_Off', 'entry-On_Off', 'entry-Off_On', 'entry-On_On']:
            try:
                ws_event_norm = LoadEventNexus("%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber),
                                               NXentryName=entry,
                                               OutputWorkspace="%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber))
            except:
                # No data in this cross-section.
                # When this happens, Mantid throws an exception.
                continue

            # There can be a handful of events in the unused entries.
            # Protect against that by requiring a minimum number of events.
            if ws_event_norm.getNumberEvents() > 1000:
                return ws_event_norm

        # If we are here, we haven't found the data we need and we need to stop execution.
        raise RuntimeError("Could not find direct beam data for run %s" % normalizationRunNumber)

    def constant_q(self, workspace, peak):
        """
            Compute reflectivity using constant-Q binning
        """
        # Extract the x-pixel vs TOF data
        signal = workspace.extractY()
        signal_err = workspace.extractE()
        signal=np.flipud(signal)
        signal_err=np.flipud(signal_err)

        theta = self.calculate_scattering_angle(workspace)
        two_theta_degrees = 2.0*theta*180.0/math.pi
        AddSampleLog(Workspace=workspace, LogName='two_theta', LogText=str(two_theta_degrees),
                     LogType='Number', LogUnit='degree')

        # Get an array with the center wavelength value for each bin
        wl_values = workspace.readX(0)

        AddSampleLog(Workspace=workspace, LogName='lambda_min', LogText=str(wl_values[0]),
                     LogType='Number', LogUnit='Angstrom')
        AddSampleLog(Workspace=workspace, LogName='lambda_max', LogText=str(wl_values[-1]),
                     LogType='Number', LogUnit='Angstrom')

        x_pixel_map = np.mgrid[peak[0]:peak[1]+1, 0:len(wl_values)]
        x_pixel_map = x_pixel_map[0,:,:]

        pixel_width = float(workspace.getInstrument().getNumberParameter("pixel-width")[0]) / 1000.0
        det_distance = workspace.getRun()['SampleDetDis'].getStatistics().mean / 1000.0
        round_up = self.getProperty("RoundUpPixel").value
        ref_pix = self.getProperty("SpecularPixel").value
        if round_up:
            x_distance=(x_pixel_map-np.round(ref_pix)) * pixel_width
        else:
            # We offset by 0.5 pixels because the reference pixel is given as
            # a fractional position relative to the start of a pixel, whereas the pixel map
            # assumes the center of each pixel.
            x_distance=(x_pixel_map-ref_pix-0.5) * pixel_width

        theta_f = np.arcsin(x_distance/det_distance)/2.0

        # Calculate Qx, Qz for each pixel
        LL,TT = np.meshgrid(wl_values, theta_f[:,0])

        qz=4*math.pi/LL * np.sin(theta + TT) * np.cos(TT)
        qz = qz.T

        AddSampleLog(Workspace=workspace, LogName='q_min', LogText=str(np.min(qz)),
                     LogType='Number', LogUnit='1/Angstrom')
        AddSampleLog(Workspace=workspace, LogName='q_max', LogText=str(np.max(qz)),
                     LogType='Number', LogUnit='1/Angstrom')

        # Transform q values to q indices
        q_min_input = self.getProperty("QMin").value
        q_step = self.getProperty("QStep").value

        # We use np.digitize to bin. The output of digitize is a bin
        # number for each entry, starting at 1. The first bin (0) is
        # for underflow entries, and the last bin is for overflow entries.
        if q_step > 0:
            n_q_values = int((np.max(qz) - q_min_input) / q_step)
            axis_z = np.linspace(q_min_input, np.max(qz), n_q_values)
        else:
            n_q_values = int((np.log10(np.max(qz))-np.log10(q_min_input)) / np.log10(1.0-q_step))
            axis_z = np.array([q_min_input * (1.0-q_step)**i for i in range(n_q_values)])

        refl = np.zeros(len(axis_z)-1)
        refl_err = np.zeros(len(axis_z)-1)
        signal_n = np.zeros(len(axis_z)-1)

        err_count = 0
        for tof in range(qz.shape[0]):
            z_inds = np.digitize(qz[tof], axis_z)

            # Move the indices so the valid bin numbers start at zero,
            # since this is how we are going to address the output array
            z_inds -= 1

            for ix in range(signal.shape[0]):
                if z_inds[ix]<len(axis_z)-1 and z_inds[ix]>=0 and not np.isnan(signal[ix][tof]):
                    refl[z_inds[ix]] += signal[ix][tof]
                    refl_err[z_inds[ix]] += signal_err[ix][tof] * signal_err[ix][tof]
                    signal_n[z_inds[ix]] += 1.0
                elif signal[ix][tof]>0:
                    err_count += 1
        logger.notice("Ignored pixels: %s" % err_count)

        signal_n = np.where(signal_n > 0, signal_n, 1)
        refl = float(signal.shape[0]) * refl / signal_n
        refl_err = float(signal.shape[0]) * np.sqrt(refl_err) / signal_n

        # Trim Qz bins with fewer than a certain number of wavelength bins
        # contributing to them,
        trim_factor = self.getProperty("ConstQTrim").value
        for i in range(len(refl)):
            if signal_n[i] < np.max(signal_n) * trim_factor:
                refl[i] = 0.0
                refl_err[i] = 0.0

        q_rebin = CreateWorkspace(DataX=axis_z, DataY=refl, DataE=refl_err, ParentWorkspace=workspace)
        # At this point we still have a histogram, and we need to convert to point data
        q_rebin = ConvertToPointData(InputWorkspace=q_rebin)
        return q_rebin

    def convert_to_q(self, workspace):
        """
            Convert a reflectivity workspace to Q space
            @param workspace: workspace to convert
        """
        # Because of the way we bin and convert to Q, consider
        # this a distribution.
        workspace.setDistribution(True)

        # TOF range
        tof_range = self.get_tof_range(workspace)

        # Get Q range
        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(workspace)
        two_theta_degrees = 2.0*theta*180.0/math.pi
        AddSampleLog(Workspace=workspace, LogName='two_theta', LogText=str(two_theta_degrees),
                     LogType='Number', LogUnit='degree')

        q_workspace = SumSpectra(InputWorkspace = workspace)
        q_workspace.getAxis(0).setUnit("MomentumTransfer")

        # Get the distance fromthe moderator to the detector
        run_object = workspace.getRun()
        sample_detector_distance = run_object['SampleDetDis'].getStatistics().mean / 1000.0
        source_sample_distance = run_object['ModeratorSamDis'].getStatistics().mean / 1000.0
        source_detector_distance = source_sample_distance + sample_detector_distance

        # Convert to Q
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * source_detector_distance / h * math.sin(theta)
        q_range = [qMin, qStep, constant / tof_range[0] * 1.2]

        q_min_from_data = constant / tof_range[1]
        q_max_from_data = constant / tof_range[0]
        AddSampleLog(Workspace=q_workspace, LogName='q_min', LogText=str(q_min_from_data),
                     LogType='Number', LogUnit='1/Angstrom')
        AddSampleLog(Workspace=q_workspace, LogName='q_max', LogText=str(q_max_from_data),
                     LogType='Number', LogUnit='1/Angstrom')

        tof_to_lambda = 1.0e4 * h / (m * source_detector_distance)
        lambda_min = tof_to_lambda * tof_range[0]
        lambda_max = tof_to_lambda * tof_range[1]
        AddSampleLog(Workspace=q_workspace, LogName='lambda_min', LogText=str(lambda_min),
                     LogType='Number', LogUnit='Angstrom')
        AddSampleLog(Workspace=q_workspace, LogName='lambda_max', LogText=str(lambda_max),
                     LogType='Number', LogUnit='Angstrom')

        data_x = q_workspace.dataX(0)
        for i in range(len(data_x)):
            data_x[i] = constant / data_x[i]
        q_workspace = SortXAxis(InputWorkspace=q_workspace, OutputWorkspace=str(q_workspace))

        name_output_ws = self.getPropertyValue("OutputWorkspace")
        try:
            q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range,
                            OutputWorkspace=name_output_ws)
        except:
            raise RuntimeError("Could not rebin with %s" % str(q_range))

        AnalysisDataService.remove(str(q_workspace))

        return q_rebin

    def cleanup_reflectivity(self, q_rebin):
        """
            Clean up the reflectivity workspace, removing zeros and cropping.
            @param q_rebin: reflectivity workspace
        """
        # Replace NaNs by zeros
        q_rebin = ReplaceSpecialValues(InputWorkspace=q_rebin,
                                       OutputWorkspace=str(q_rebin),
                                       NaNValue=0.0, NaNError=0.0)
        # Crop to non-zero values
        data_y = q_rebin.readY(0)
        low_q = None
        high_q = None
        for i in range(len(data_y)):
            if low_q is None and abs(data_y[i])>0:
                low_q = i
            if high_q is None and abs(data_y[len(data_y)-1-i])>0:
                high_q = len(data_y)-1-i
            if low_q is not None and high_q is not None:
                break

        crop = self.getProperty("CropFirstAndLastPoints").value
        if low_q is not None and high_q is not None:
            # Get rid of first and last Q points to avoid edge effects
            if crop:
                low_q += 1
                high_q -= 1
            data_x = q_rebin.readX(0)
            q_rebin = CropWorkspace(InputWorkspace=q_rebin,
                                    OutputWorkspace=str(q_rebin),
                                    XMin=data_x[low_q], XMax=data_x[high_q])
        else:
            logger.error("Data is all zeros. Check your TOF ranges.")

        # Clean up the workspace for backward compatibility
        data_y = q_rebin.dataY(0)
        data_e = q_rebin.dataE(0)

        # Values < 1e-12 and values where the error is greater than the value are replaced by 0+-1
        for i in range(len(data_y)):
            if data_y[i] < 1e-12 or data_e[i]>data_y[i]:
                data_y[i]=0.0
                data_e[i]=1.0

        # Sanity check
        if sum(data_y) == 0:
            raise RuntimeError("The reflectivity is all zeros: check your peak selection")

        self.write_meta_data(q_rebin)
        return q_rebin

    def calculate_scattering_angle(self, ws_event_data):
        """
            Compute the scattering angle
            @param ws_event_data: data workspace
        """
        run_object = ws_event_data.getRun()

        angle_offset = self.getProperty("AngleOffset").value
        use_sangle = self.getProperty("UseSANGLE").value
        sangle = run_object['SANGLE'].getStatistics().mean

        if use_sangle:
            theta = abs(sangle) * math.pi / 180.0
        else:
            dangle = run_object['DANGLE'].getStatistics().mean
            dangle0 = run_object['DANGLE0'].getStatistics().mean
            det_distance = run_object['SampleDetDis'].getStatistics().mean / 1000.0
            direct_beam_pix = run_object['DIRPIX'].getStatistics().mean
            ref_pix = self.getProperty("SpecularPixel").value

            # Get pixel size from instrument properties
            if ws_event_data.getInstrument().hasParameter("pixel_width"):
                pixel_width = float(ws_event_data.getInstrument().getNumberParameter("pixel_width")[0]) / 1000.0
            else:
                pixel_width = 0.0007

            theta = (dangle - dangle0) * math.pi / 180.0 / 2.0 + ((direct_beam_pix - ref_pix) * pixel_width) / (2.0 * det_distance)
            theta = abs(theta)

        return theta + angle_offset

    def get_tof_range(self, ws_event_data):
        """
            Determine the TOF range
            @param ws_event_data: data workspace
        """
        if self._tof_range is not None:
            return self._tof_range

        crop_TOF = self.getProperty("CutTimeAxis").value
        tof_step = self.getProperty("TimeAxisStep").value
        if crop_TOF:
            self._tof_range = self.getProperty("TimeAxisRange").value  #microS
            if self._tof_range[0] <= 0:
                self._tof_range[0] = tof_step
                logger.error("Lower bound of TOF range cannot be zero: using %s" % tof_step)
        else:
            # If the TOF range option is turned off, use the full range
            # Protect against TOF=0, which will crash when going to Q.
            tof_min = ws_event_data.getTofMin()
            if tof_min <= 0:
                tof_min = tof_step
            tof_max = ws_event_data.getTofMax()
            self._tof_range = [tof_min, tof_max]
            logger.notice("Determining range: %g %g" % (tof_min, tof_max))

        return self._tof_range

    def write_meta_data(self, workspace):
        """
            Write meta data documenting the regions of interest
            @param workspace: data workspace
        """
        constant_q_binning = self.getProperty("ConstantQBinning").value
        AddSampleLog(Workspace=workspace, LogName='constant_q_binning', LogText=str(constant_q_binning))

        # Data
        data_peak_range = self.getProperty("SignalPeakPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='scatt_peak_min', LogText=str(data_peak_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='scatt_peak_max', LogText=str(data_peak_range[1]),
                     LogType='Number', LogUnit='pixel')

        data_bg_range = self.getProperty("SignalBackgroundPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='scatt_bg_min', LogText=str(data_bg_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='scatt_bg_max', LogText=str(data_bg_range[1]),
                     LogType='Number', LogUnit='pixel')

        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='scatt_low_res_min', LogText=str(low_res_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='scatt_low_res_max', LogText=str(low_res_range[1]),
                     LogType='Number', LogUnit='pixel')

        specular_pixel = self.getProperty("SpecularPixel").value
        AddSampleLog(Workspace=workspace, LogName='specular_pixel', LogText=str(specular_pixel),
                     LogType='Number', LogUnit='pixel')

        # Direct beam runs
        data_peak_range = self.getProperty("NormPeakPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='norm_peak_min', LogText=str(data_peak_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='norm_peak_max', LogText=str(data_peak_range[1]),
                     LogType='Number', LogUnit='pixel')

        data_bg_range = self.getProperty("NormBackgroundPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='norm_bg_min', LogText=str(data_bg_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='norm_bg_max', LogText=str(data_bg_range[1]),
                     LogType='Number', LogUnit='pixel')

        low_res_range = self.getProperty("LowResNormAxisPixelRange").value
        AddSampleLog(Workspace=workspace, LogName='norm_low_res_min', LogText=str(low_res_range[0]),
                     LogType='Number', LogUnit='pixel')
        AddSampleLog(Workspace=workspace, LogName='norm_low_res_max', LogText=str(low_res_range[1]),
                     LogType='Number', LogUnit='pixel')

    #pylint: disable=too-many-arguments
    def process_data(self, workspace, crop_low_res, low_res_range,
                     peak_range, subtract_background, background_range):
        """
            Common processing for both sample data and normalization.
        """
        use_wl_cut = self.getProperty("UseWLTimeAxis").value
        constant_q_binning = self.getProperty("ConstantQBinning").value

        # With constant-Q binning, convert to wavelength before or after
        # cutting the time axis depending on how the user wanted it.
        if constant_q_binning and use_wl_cut:
            # Convert to wavelength
            workspace = ConvertUnits(InputWorkspace=workspace, Target="Wavelength",
                                     AlignBins=True, ConvertFromPointData=False,
                                     OutputWorkspace="%s_histo" % str(workspace))
        tof_range = self.get_tof_range(workspace)
        # Rebin wavelength axis
        tof_max = workspace.getTofMax()
        tof_min = workspace.getTofMin()
        if tof_min > tof_range[1] or tof_max < tof_range[0]:
            error_msg = "Requested range does not match data for %s: " % str(workspace)
            error_msg += "[%g, %g] found [%g, %g]" % (tof_range[0], tof_range[1],
                                                      tof_min, tof_max)
            raise RuntimeError(error_msg)

        tof_step = self.getProperty("TimeAxisStep").value
        logger.notice("Time axis range: %s %s %s [%s %s]" % (tof_range[0], tof_step, tof_range[1], tof_min, tof_max))
        workspace = Rebin(InputWorkspace=workspace, Params=[tof_range[0], tof_step, tof_range[1]],
                          OutputWorkspace="%s_histo" % str(workspace))

        if constant_q_binning and not use_wl_cut:
            # Convert to wavelength
            workspace = ConvertUnits(InputWorkspace=workspace, Target="Wavelength",
                                     AlignBins=True, ConvertFromPointData=False,
                                     OutputWorkspace="%s_histo" % str(workspace))

        # Integrate over low resolution range
        low_res_min = 0
        low_res_max = self.number_of_pixels_y
        if crop_low_res:
            low_res_min = int(low_res_range[0])
            low_res_max = int(low_res_range[1])

        # Subtract background
        if subtract_background:
            average = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                             NXPixel=self.number_of_pixels_x,
                             NYPixel=self.number_of_pixels_y,
                             ConvertToQ=False,
                             XPixelMin=int(background_range[0]),
                             XPixelMax=int(background_range[1]),
                             YPixelMin=low_res_min,
                             YPixelMax=low_res_max,
                             ErrorWeighting = True,
                             SumPixels=True, NormalizeSum=True)

            signal = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                            NXPixel=self.number_of_pixels_x,
                            NYPixel=self.number_of_pixels_y,
                            ConvertToQ=False, YPixelMin=low_res_min, YPixelMax=low_res_max,
                            OutputWorkspace="signal_%s" % str(workspace))
            subtracted = Minus(LHSWorkspace=signal, RHSWorkspace=average,
                               OutputWorkspace="subtracted_%s" % str(workspace))
            AnalysisDataService.remove(str(average))
            AnalysisDataService.remove(str(signal))
        else:
            # If we don't subtract the background, we still have to integrate
            # over the low resolution axis
            subtracted = RefRoi(InputWorkspace=workspace, IntegrateY=True,
                                NXPixel=self.number_of_pixels_x,
                                NYPixel=self.number_of_pixels_y,
                                ConvertToQ=False, YPixelMin=low_res_min, YPixelMax=low_res_max,
                                OutputWorkspace="subtracted_%s" % str(workspace))

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        subtracted = NormaliseByCurrent(InputWorkspace=subtracted, OutputWorkspace=str(subtracted))

        # Crop to only the selected peak region
        cropped = CropWorkspace(InputWorkspace=subtracted,
                                StartWorkspaceIndex=int(peak_range[0]),
                                EndWorkspaceIndex=int(peak_range[1]),
                                OutputWorkspace="%s_cropped" % str(subtracted))

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(workspace))
        AnalysisDataService.remove(str(subtracted))
        return cropped


AlgorithmFactory.subscribe(MagnetismReflectometryReduction)
