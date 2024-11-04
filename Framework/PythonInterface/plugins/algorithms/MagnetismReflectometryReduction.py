# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init, invalid-name, bare-except
"""
Magnetism reflectometry reduction
"""

import sys
import math
import numpy as np
from mantid.api import (
    mtd,
    AlgorithmFactory,
    AnalysisDataService,
    PropertyMode,
    PythonAlgorithm,
    WorkspaceGroup,
    WorkspaceProperty,
)
from mantid.kernel import (
    logger,
    Direction,
    FloatArrayLengthValidator,
    FloatArrayProperty,
    IntArrayLengthValidator,
    IntArrayProperty,
    Property,
)
from mantid.simpleapi import (
    AddSampleLog,
    ConvertToPointData,
    ConvertUnits,
    CreateWorkspace,
    CropWorkspace,
    Divide,
    GroupWorkspaces,
    LoadEventNexus,
    Minus,
    MRGetTheta,
    NormaliseByCurrent,
    Rebin,
    RebinToWorkspace,
    RefRoi,
    RenameWorkspace,
    ReplaceSpecialValues,
    SortXAxis,
    SumSpectra,
)

INSTRUMENT_NAME = "REF_M"


class MagnetismReflectometryReduction(PythonAlgorithm):
    """
    Workflow algorithm to reduce REF_M data
    """

    number_of_pixels_x = 0
    number_of_pixels_y = 0
    _tof_range = None

    def category(self):
        """Algorithm category"""
        return "Reflectometry\\SNS"

    def name(self):
        """Algorithm name"""
        return "MagnetismReflectometryReduction"

    def version(self):
        """Version number"""
        return 1

    def summary(self):
        """Friendly description"""
        return "Magnetism Reflectometer (REFM) reduction"

    def checkGroups(self):
        """Allow us to deal with a workspace group"""
        return False

    def PyInit(self):
        """Initialization"""
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Optionally, we can provide a scattering workspace directly",
        )
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(
            WorkspaceProperty("NormalizationWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Optionally, we can provide a normalization workspace directly",
        )
        self.declareProperty(
            IntArrayProperty("SignalPeakPixelRange", [123, 137], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the data peak",
        )
        self.declareProperty("SubtractSignalBackground", True, doc="If true, the background will be subtracted from the data peak")
        self.declareProperty(
            IntArrayProperty("SignalBackgroundPixelRange", [123, 137], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the background. Default:(123,137)",
        )
        self.declareProperty("ApplyNormalization", True, doc="If true, the data will be normalized")
        self.declareProperty(
            IntArrayProperty("NormPeakPixelRange", [127, 133], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the normalization peak",
        )
        self.declareProperty("SubtractNormBackground", True, doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(
            IntArrayProperty("NormBackgroundPixelRange", [127, 137], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the background for the normalization",
        )
        self.declareProperty(
            "CutLowResDataAxis",
            True,
            doc="If true, the low resolution direction of the data will be cropped according " + "to the lowResDataAxisPixelRange property",
        )
        self.declareProperty(
            IntArrayProperty("LowResDataAxisPixelRange", [115, 210], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range to use in the low resolution direction of the data",
        )
        self.declareProperty(
            "CutLowResNormAxis",
            True,
            doc="If true, the low resolution direction of the normalization run will be cropped "
            + "according to the LowResNormAxisPixelRange property",
        )
        self.declareProperty(
            IntArrayProperty("LowResNormAxisPixelRange", [115, 210], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range to use in the low resolution direction of the normalizaion run",
        )
        self.declareProperty(
            FloatArrayProperty("TimeAxisRange", [0.0, 340000.0], FloatArrayLengthValidator(2), direction=Direction.Input),
            "TOF/wavelength range to use with detector binning",
        )
        self.declareProperty(
            "UseWLTimeAxis", False, doc="For const-Q, if true, wavelength will be used as the time axis, otherwise TOF is used"
        )
        self.declareProperty("ErrorWeightedBackground", True, doc="If true, use an error weighted average for the background estimate")
        self.declareProperty(
            "CutTimeAxis", True, doc="If true, the TOF/wavelength dimension will be cropped according to the TimeAxisRange property"
        )
        self.declareProperty("RoundUpPixel", True, doc="If True, round up pixel position of the specular reflectivity")
        self.declareProperty("UseSANGLE", False, doc="If True, use SANGLE as the scattering angle")
        self.declareProperty("SpecularPixel", 180.0, doc="Pixel position of the specular reflectivity")
        self.declareProperty("FinalRebin", True, doc="If True, the final reflectivity will be rebinned")
        self.declareProperty("QMin", 0.005, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (rad)")
        self.declareProperty(
            "TimeAxisStep", 40.0, doc="Binning step size for the time axis. TOF for detector binning, wavelength for constant Q"
        )
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")
        self.declareProperty("CleanupBadData", True, doc="If true, we crop the points consistent with R=0")
        self.declareProperty(
            "AcceptNullReflectivity", False, doc="If true, reflectivity curves consisting of all zeros are accepted as valid"
        )
        self.declareProperty("ConstQTrim", 0.5, doc="With const-Q binning, cut Q bins with contributions fewer than ConstQTrim of WL bins")
        self.declareProperty("SampleLength", 10.0, doc="Length of the sample in mm")
        self.declareProperty("ConstantQBinning", False, doc="If true, we convert to Q before summing")
        self.declareProperty("DirectPixelOverwrite", Property.EMPTY_DBL, doc="DIRPIX overwrite value")
        self.declareProperty("DAngle0Overwrite", Property.EMPTY_DBL, doc="DANGLE0 overwrite value (degrees)")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")

    # pylint: disable=too-many-locals
    def PyExec(self):
        """Main execution"""
        # Reduction parameters
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # ----- Process Sample Data -------------------------------------------
        crop_request = self.getProperty("CutLowResDataAxis").value
        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        bck_request = self.getProperty("SubtractSignalBackground").value
        perform_normalization = self.getProperty("ApplyNormalization").value

        # Processed normalization workspace
        norm_summed = None
        output_list = []

        for workspace in self.load_data():
            try:
                logger.notice("Processing %s" % str(workspace))
                data_cropped = self.process_data(workspace, crop_request, low_res_range, dataPeakRange, bck_request, dataBackRange)

                # Normalization
                if perform_normalization:
                    if norm_summed is None:
                        norm_summed = self.process_direct_beam(data_cropped)

                    # Normalize the data
                    normalized_data = Divide(
                        LHSWorkspace=data_cropped, RHSWorkspace=norm_summed, OutputWorkspace=str(data_cropped) + "_normalized"
                    )

                    AddSampleLog(Workspace=normalized_data, LogName="normalization_run", LogText=str(norm_summed.getRunNumber()))
                    AddSampleLog(
                        Workspace=normalized_data,
                        LogName="normalization_file_path",
                        LogText=norm_summed.getRun().getProperty("Filename").value,
                    )
                    norm_dirpix = norm_summed.getRun().getProperty("DIRPIX").getStatistics().mean
                    AddSampleLog(
                        Workspace=normalized_data,
                        LogName="normalization_dirpix",
                        LogText=str(norm_dirpix),
                        LogType="Number",
                        LogUnit="pixel",
                    )

                    # Avoid leaving trash behind
                    AnalysisDataService.remove(str(data_cropped))
                else:
                    normalized_data = data_cropped
                    AddSampleLog(Workspace=normalized_data, LogName="normalization_run", LogText="None")

                # At this point, the workspace should be considered a distribution of points
                point_data = ConvertToPointData(InputWorkspace=normalized_data, OutputWorkspace=str(workspace) + "_")
                # Avoid leaving trash behind
                AnalysisDataService.remove(str(normalized_data))

                # Convert to Q and clean up the distribution
                constant_q_binning = self.getProperty("ConstantQBinning").value
                if constant_q_binning:
                    q_rebin = self.constant_q(point_data, dataPeakRange)
                else:
                    q_rebin = self.convert_to_q(point_data)
                q_rebin = self.cleanup_reflectivity(q_rebin)

                # Avoid leaving trash behind
                AnalysisDataService.remove(str(point_data))

                # Add dQ to each Q point
                q_rebin = self.compute_resolution(q_rebin)
                output_list.append(q_rebin)
            except:
                logger.error("Could not process %s" % str(workspace))
                logger.error(str(sys.exc_info()[1]))

        # Prepare output workspace group
        if len(output_list) > 1:
            output_wsg = self.getPropertyValue("OutputWorkspace")
            GroupWorkspaces(InputWorkspaces=output_list, OutputWorkspace=output_wsg)
            self.setProperty("OutputWorkspace", output_wsg)
        elif len(output_list) == 1:
            self.setProperty("OutputWorkspace", output_list[0])
        else:
            raise ValueError("No valida workspace found.")

        # Clean up leftover workspace
        if norm_summed is not None:
            AnalysisDataService.remove(str(norm_summed))

    def load_data(self):
        """
        Load the data. We can either load it from the specified
        run numbers, or use the input workspace.

        Supplying a workspace takes precedence over supplying a list of runs
        """
        input_workspaces = self.getProperty("InputWorkspace").value

        if isinstance(input_workspaces, WorkspaceGroup):
            ws_list = input_workspaces
        else:
            ws_list = [input_workspaces]

        # Sanity check, and retrieve some info while we're at it.
        if ws_list:
            self.number_of_pixels_x = int(ws_list[0].getInstrument().getNumberParameter("number-of-x-pixels")[0])
            self.number_of_pixels_y = int(ws_list[0].getInstrument().getNumberParameter("number-of-y-pixels")[0])
        else:
            raise RuntimeError("No input data was specified")
        return ws_list

    def load_direct_beam(self):
        """
        Load a direct beam file. We can either load it from the specified
        run number, or use the input workspace.

        Supplying a workspace takes precedence over supplying a run number.
        """
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        ws_event_norm = self.getProperty("NormalizationWorkspace").value

        if ws_event_norm is not None:
            return ws_event_norm

        for entry in ["entry", "entry-Off_Off", "entry-On_Off", "entry-Off_On", "entry-On_On"]:
            try:
                ws_event_norm = LoadEventNexus(
                    "%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber),
                    NXentryName=entry,
                    OutputWorkspace="%s_%s" % (INSTRUMENT_NAME, normalizationRunNumber),
                )
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

    def process_direct_beam(self, data_cropped):
        """
        Process the direct beam and rebin it to match our
        scattering data.
        :param Workspace data_cropped: scattering data workspace
        """
        # Load normalization
        ws_event_norm = self.load_direct_beam()

        # Retrieve reduction parameters
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value
        crop_request = self.getProperty("CutLowResNormAxis").value
        low_res_range = self.getProperty("LowResNormAxisPixelRange").value
        bck_request = self.getProperty("SubtractNormBackground").value

        norm_cropped = self.process_data(
            ws_event_norm, crop_request, low_res_range, normPeakRange, bck_request, normBackRange, rebin_to_ws=data_cropped
        )
        # Avoid leaving trash behind (remove only if we loaded the data)
        if self.getProperty("NormalizationWorkspace").value is None:
            AnalysisDataService.remove(str(ws_event_norm))

        return norm_cropped

    def constant_q(self, workspace, peak):
        """
        Compute reflectivity using constant-Q binning
        """
        # Extract the x-pixel vs TOF data
        signal = workspace.extractY()
        signal_err = workspace.extractE()
        signal = np.flipud(signal)
        signal_err = np.flipud(signal_err)

        theta = self.calculate_scattering_angle(workspace)
        two_theta_degrees = 2.0 * theta * 180.0 / math.pi
        AddSampleLog(Workspace=workspace, LogName="two_theta", LogText=str(two_theta_degrees), LogType="Number", LogUnit="degree")

        # Get an array with the center wavelength value for each bin
        wl_values = workspace.readX(0)

        AddSampleLog(Workspace=workspace, LogName="lambda_min", LogText=str(wl_values[0]), LogType="Number", LogUnit="Angstrom")
        AddSampleLog(Workspace=workspace, LogName="lambda_max", LogText=str(wl_values[-1]), LogType="Number", LogUnit="Angstrom")

        x_pixel_map = np.mgrid[peak[0] : peak[1] + 1, 0 : len(wl_values)]
        x_pixel_map = x_pixel_map[0, :, :]

        pixel_width = float(workspace.getInstrument().getNumberParameter("pixel-width")[0]) / 1000.0
        det_distance = workspace.getRun()["SampleDetDis"].getStatistics().mean
        # Check units
        if workspace.getRun()["SampleDetDis"].units not in ["m", "meter"]:
            det_distance /= 1000.0

        round_up = self.getProperty("RoundUpPixel").value
        ref_pix = self.getProperty("SpecularPixel").value
        if round_up:
            x_distance = (x_pixel_map - np.round(ref_pix)) * pixel_width
        else:
            # We offset by 0.5 pixels because the reference pixel is given as
            # a fractional position relative to the start of a pixel, whereas the pixel map
            # assumes the center of each pixel.
            x_distance = (x_pixel_map - ref_pix - 0.5) * pixel_width

        theta_f = np.arcsin(x_distance / det_distance) / 2.0

        # Calculate Qx, Qz for each pixel
        LL, TT = np.meshgrid(wl_values, theta_f[:, 0])

        qz = 4 * math.pi / LL * np.sin(theta + TT) * np.cos(TT)
        qz = qz.T

        AddSampleLog(Workspace=workspace, LogName="q_min", LogText=str(np.min(qz)), LogType="Number", LogUnit="1/Angstrom")
        AddSampleLog(Workspace=workspace, LogName="q_max", LogText=str(np.max(qz)), LogType="Number", LogUnit="1/Angstrom")

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
            n_q_values = int((np.log10(np.max(qz)) - np.log10(q_min_input)) / np.log10(1.0 - q_step))
            axis_z = np.array([q_min_input * (1.0 - q_step) ** i for i in range(n_q_values)])

        refl = np.zeros(len(axis_z) - 1)
        refl_err = np.zeros(len(axis_z) - 1)
        signal_n = np.zeros(len(axis_z) - 1)

        err_count = 0
        for tof in range(qz.shape[0]):
            z_inds = np.digitize(qz[tof], axis_z)

            # Move the indices so the valid bin numbers start at zero,
            # since this is how we are going to address the output array
            z_inds -= 1

            for ix in range(signal.shape[0]):
                if z_inds[ix] < len(axis_z) - 1 and z_inds[ix] >= 0 and not np.isnan(signal[ix][tof]):
                    refl[z_inds[ix]] += signal[ix][tof]
                    refl_err[z_inds[ix]] += signal_err[ix][tof] * signal_err[ix][tof]
                    signal_n[z_inds[ix]] += 1.0
                elif signal[ix][tof] > 0:
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

        name_output_ws = str(workspace) + "_reflectivity"  # self.getPropertyValue("OutputWorkspace")
        q_rebin = CreateWorkspace(DataX=axis_z, DataY=refl, DataE=refl_err, ParentWorkspace=workspace, OutputWorkspace=name_output_ws)

        # At this point we still have a histogram, and we need to convert to point data
        q_rebin = ConvertToPointData(InputWorkspace=q_rebin, OutputWorkspace=name_output_ws)
        return q_rebin

    def convert_to_q(self, workspace):
        """
        Convert a reflectivity workspace to Q space
        @param workspace: workspace to convert
        """
        # Because of the way we bin and convert to Q, consider
        # this a distribution.
        workspace.setDistribution(True)

        # Get Q range
        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(workspace)
        two_theta_degrees = 2.0 * theta * 180.0 / math.pi
        AddSampleLog(Workspace=workspace, LogName="two_theta", LogText=str(two_theta_degrees), LogType="Number", LogUnit="degree")

        q_workspace = SumSpectra(InputWorkspace=workspace)
        q_workspace.getAxis(0).setUnit("MomentumTransfer")

        # Get the distance fromthe moderator to the detector
        run_object = workspace.getRun()
        sample_detector_distance = run_object["SampleDetDis"].getStatistics().mean
        source_sample_distance = run_object["ModeratorSamDis"].getStatistics().mean
        # Check units
        if run_object["SampleDetDis"].units not in ["m", "meter"]:
            sample_detector_distance /= 1000.0
        if run_object["ModeratorSamDis"].units not in ["m", "meter"]:
            source_sample_distance /= 1000.0
        source_detector_distance = source_sample_distance + sample_detector_distance

        # Convert to Q
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * source_detector_distance / h * math.sin(theta)
        q_range = [qMin, qStep, constant / self._tof_range[0] * 1.2]

        q_min_from_data = constant / self._tof_range[1]
        q_max_from_data = constant / self._tof_range[0]
        AddSampleLog(Workspace=q_workspace, LogName="q_min", LogText=str(q_min_from_data), LogType="Number", LogUnit="1/Angstrom")
        AddSampleLog(Workspace=q_workspace, LogName="q_max", LogText=str(q_max_from_data), LogType="Number", LogUnit="1/Angstrom")

        tof_to_lambda = 1.0e4 * h / (m * source_detector_distance)
        lambda_min = tof_to_lambda * self._tof_range[0]
        lambda_max = tof_to_lambda * self._tof_range[1]
        AddSampleLog(Workspace=q_workspace, LogName="lambda_min", LogText=str(lambda_min), LogType="Number", LogUnit="Angstrom")
        AddSampleLog(Workspace=q_workspace, LogName="lambda_max", LogText=str(lambda_max), LogType="Number", LogUnit="Angstrom")

        data_x = q_workspace.dataX(0)
        for i in range(len(data_x)):
            data_x[i] = constant / data_x[i]
        q_workspace = SortXAxis(InputWorkspace=q_workspace, OutputWorkspace=str(q_workspace))

        name_output_ws = str(workspace) + "_reflectivity"
        do_q_rebin = self.getProperty("FinalRebin").value

        if do_q_rebin:
            try:
                q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range, OutputWorkspace=name_output_ws)
                AnalysisDataService.remove(str(q_workspace))
            except:
                logger.error("Could not rebin with %s" % str(q_range))
                do_q_rebin = False

        # If we either didn't want to rebin or we failed to rebin,
        # rename the reflectivity workspace and proceed with it.
        if not do_q_rebin:
            q_rebin = RenameWorkspace(InputWorkspace=q_workspace, OutputWorkspace=name_output_ws)

        return q_rebin

    def cleanup_reflectivity(self, q_rebin):
        """
        Clean up the reflectivity workspace, removing zeros and cropping.
        @param q_rebin: reflectivity workspace
        """
        # Replace NaNs by zeros
        q_rebin = ReplaceSpecialValues(InputWorkspace=q_rebin, OutputWorkspace=str(q_rebin), NaNValue=0.0, NaNError=0.0)
        # Crop to non-zero values
        data_y = q_rebin.readY(0)
        low_q = None
        high_q = None
        for i in range(len(data_y)):
            if low_q is None and abs(data_y[i]) > 0:
                low_q = i
            if high_q is None and abs(data_y[len(data_y) - 1 - i]) > 0:
                high_q = len(data_y) - 1 - i
            if low_q is not None and high_q is not None:
                break

        cleanup = self.getProperty("CleanupBadData").value
        crop = self.getProperty("CropFirstAndLastPoints").value
        if cleanup and low_q is not None and high_q is not None:
            # Get rid of first and last Q points to avoid edge effects
            if crop:
                low_q += 1
                high_q -= 1
            data_x = q_rebin.readX(0)
            q_rebin = CropWorkspace(InputWorkspace=q_rebin, OutputWorkspace=str(q_rebin), XMin=data_x[low_q], XMax=data_x[high_q])
        elif cleanup:
            logger.error("Data is all zeros. Check your TOF ranges.")

        # Clean up the workspace for backward compatibility
        data_y = q_rebin.dataY(0)

        if self.getProperty("AcceptNullReflectivity").value is False and sum(data_y) == 0:
            raise RuntimeError("The reflectivity is all zeros: check your peak selection")

        self.write_meta_data(q_rebin)
        return q_rebin

    def compute_resolution(self, ws):
        """
        Calculate dQ/Q using the slit information.
        :param workspace ws: reflectivity workspace
        """
        sample_length = self.getProperty("SampleLength").value

        # In newer data files, the slit distances are part of the logs
        if ws.getRun().hasProperty("S1Distance"):
            s1_dist = ws.getRun().getProperty("S1Distance").value
            s2_dist = ws.getRun().getProperty("S2Distance").value
            s3_dist = ws.getRun().getProperty("S3Distance").value
            # Check the units of the distances
            if ws.getRun().getProperty("S1Distance").units in ["m", "meter"]:
                s1_dist *= 1000.0
                s2_dist *= 1000.0
                s3_dist *= 1000.0
        else:
            s1_dist = -2600.0
            s2_dist = -2019.0
            s3_dist = -714.0
        slits = [
            [ws.getRun().getProperty("S1HWidth").getStatistics().mean, s1_dist],
            [ws.getRun().getProperty("S2HWidth").getStatistics().mean, s2_dist],
            [ws.getRun().getProperty("S3HWidth").getStatistics().mean, s3_dist],
        ]
        theta = ws.getRun().getProperty("two_theta").value / 2.0 * np.pi / 180.0
        res = []
        s_width = sample_length * np.sin(theta)
        for width, dist in slits:
            # Calculate the maximum opening angle dTheta
            if s_width > 0.0:
                d_theta = np.arctan((s_width / 2.0 * (1.0 + width / s_width)) / dist) * 2.0
            else:
                d_theta = np.arctan(width / 2.0 / dist) * 2.0
            # The standard deviation for a uniform angle distribution is delta/sqrt(12)
            res.append(d_theta * 0.28867513)

        # Wavelength uncertainty
        lambda_min = ws.getRun().getProperty("lambda_min").value
        lambda_max = ws.getRun().getProperty("lambda_max").value
        dq_over_q = np.min(res) / np.tan(theta)

        data_x = ws.dataX(0)
        data_dx = ws.dataDx(0)
        dwl = (lambda_max - lambda_min) / len(data_x) / np.sqrt(12.0)
        for i in range(len(data_x)):
            dq_theta = data_x[i] * dq_over_q
            dq_wl = data_x[i] ** 2 * dwl / (4.0 * np.pi * np.sin(theta))
            data_dx[i] = np.sqrt(dq_theta**2 + dq_wl**2)

        return ws

    def calculate_scattering_angle(self, ws_event_data):
        """
        Compute the scattering angle
        @param ws_event_data: data workspace
        """
        angle_offset = self.getProperty("AngleOffset").value
        use_sangle = self.getProperty("UseSANGLE").value
        ref_pix = self.getProperty("SpecularPixel").value
        dirpix_overwrite = self.getProperty("DirectPixelOverwrite").value
        dangle0_overwrite = self.getProperty("DAngle0Overwrite").value

        _theta = MRGetTheta(
            Workspace=ws_event_data,
            AngleOffset=angle_offset,
            UseSANGLE=use_sangle,
            SpecularPixel=ref_pix,
            DirectPixelOverwrite=dirpix_overwrite,
            DAngle0Overwrite=dangle0_overwrite,
        )

        return _theta

    def get_tof_range(self, ws_event_data):
        """
        Determine the TOF range. To ensure consistency, determine it once and
        return the same range for subsequent calls.

        If a TOF range has been set using TimeAxisRange, it will be used to
        determine the range.

        @param ws_event_data: data workspace
        """
        if self._tof_range is not None:
            self.validate_tof_range(ws_event_data)
            return

        tof_step = self.getProperty("TimeAxisStep").value
        crop_TOF = self.getProperty("CutTimeAxis").value
        use_wl_cut = self.getProperty("UseWLTimeAxis").value
        crop_TOF = crop_TOF and not use_wl_cut
        if crop_TOF:
            self._tof_range = self.getProperty("TimeAxisRange").value  # microS
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

    def validate_tof_range(self, ws_event_data):
        """
        Validate that the TOF range we previously stored is consistent with
        the given workspace.
        @param ws_event_data: data workspace
        """
        if self._tof_range is not None:
            # If we have determined the range
            tof_max = ws_event_data.getTofMax()
            tof_min = ws_event_data.getTofMin()
            if tof_min > self._tof_range[1] or tof_max < self._tof_range[0]:
                error_msg = "Requested range does not match data for %s: " % str(ws_event_data)
                error_msg += "[%g, %g] found [%g, %g]" % (self._tof_range[0], self._tof_range[1], tof_min, tof_max)
                raise RuntimeError(error_msg)

    def write_meta_data(self, workspace):
        """
        Write meta data documenting the regions of interest
        @param workspace: data workspace
        """
        constant_q_binning = self.getProperty("ConstantQBinning").value
        AddSampleLog(Workspace=workspace, LogName="constant_q_binning", LogText=str(constant_q_binning))

        # Data
        data_peak_range = self.getProperty("SignalPeakPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="scatt_peak_min", LogText=str(data_peak_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="scatt_peak_max", LogText=str(data_peak_range[1]), LogType="Number", LogUnit="pixel")

        data_bg_range = self.getProperty("SignalBackgroundPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="scatt_bg_min", LogText=str(data_bg_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="scatt_bg_max", LogText=str(data_bg_range[1]), LogType="Number", LogUnit="pixel")

        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="scatt_low_res_min", LogText=str(low_res_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="scatt_low_res_max", LogText=str(low_res_range[1]), LogType="Number", LogUnit="pixel")

        specular_pixel = self.getProperty("SpecularPixel").value
        AddSampleLog(Workspace=workspace, LogName="specular_pixel", LogText=str(specular_pixel), LogType="Number", LogUnit="pixel")

        # Direct beam runs
        data_peak_range = self.getProperty("NormPeakPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="norm_peak_min", LogText=str(data_peak_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="norm_peak_max", LogText=str(data_peak_range[1]), LogType="Number", LogUnit="pixel")

        data_bg_range = self.getProperty("NormBackgroundPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="norm_bg_min", LogText=str(data_bg_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="norm_bg_max", LogText=str(data_bg_range[1]), LogType="Number", LogUnit="pixel")

        low_res_range = self.getProperty("LowResNormAxisPixelRange").value
        AddSampleLog(Workspace=workspace, LogName="norm_low_res_min", LogText=str(low_res_range[0]), LogType="Number", LogUnit="pixel")
        AddSampleLog(Workspace=workspace, LogName="norm_low_res_max", LogText=str(low_res_range[1]), LogType="Number", LogUnit="pixel")

    # pylint: disable=too-many-arguments
    def process_data(self, workspace, crop_low_res, low_res_range, peak_range, subtract_background, background_range, rebin_to_ws=None):
        """
        Common processing for both sample data and normalization.
        :param workspace: event workspace to process
        :param crop_low_res: if True, the low-resolution direction will be cropped
        :param low_res_range: low-resolution direction pixel range
        :param peak_range: pixel range of the specular reflection peak
        :param subtract_background: if True, the background will be subtracted
        :param background__range: pixel range of the background region
        :param rebin_to_ws: Workspace to rebin to instead of doing independent rebinning
        """
        logger.warning("Processing %s" % str(workspace))
        use_wl_cut = self.getProperty("UseWLTimeAxis").value
        error_weighted_bck = self.getProperty("ErrorWeightedBackground").value
        constant_q_binning = self.getProperty("ConstantQBinning").value
        tof_step = self.getProperty("TimeAxisStep").value

        # First, a sanity check to see that the TOF range matches what we expect.
        self.get_tof_range(workspace)

        # For constant-Q binning, we work in wavelength. We just need to know
        # whether we need to convert units first and bin/cut in wavelength,
        # or bin/cut in TOF and convert afterwards
        convert_first = use_wl_cut or rebin_to_ws

        if constant_q_binning and convert_first:
            # Convert to wavelength
            workspace = ConvertUnits(
                InputWorkspace=workspace,
                Target="Wavelength",
                AlignBins=True,
                ConvertFromPointData=False,
                OutputWorkspace="%s_histo" % str(workspace),
            )

        # Rebin either to the specified parameter or to the given workspace
        if rebin_to_ws is not None:
            workspace = RebinToWorkspace(
                WorkspaceToRebin=workspace, WorkspaceToMatch=rebin_to_ws, OutputWorkspace="%s_histo" % str(workspace)
            )
        else:
            # Determine rebinning parameters according to whether we work in
            # wavelength or TOF
            if use_wl_cut:
                _range = self.getProperty("TimeAxisRange").value
                if _range[0] <= 0:
                    _range[0] = tof_step
            else:
                _range = self._tof_range
            workspace = Rebin(
                InputWorkspace=workspace, Params=[_range[0], tof_step, _range[1]], OutputWorkspace="%s_histo" % str(workspace)
            )

        if constant_q_binning and not convert_first:
            # Convert to wavelength
            workspace = ConvertUnits(
                InputWorkspace=workspace,
                Target="Wavelength",
                AlignBins=True,
                ConvertFromPointData=False,
                OutputWorkspace="%s_histo" % str(workspace),
            )

        # Integrate over low resolution range
        low_res_min = 0
        low_res_max = self.number_of_pixels_y
        if crop_low_res:
            low_res_min = int(low_res_range[0])
            low_res_max = min(int(low_res_range[1]), self.number_of_pixels_y - 1)

        # Subtract background
        if subtract_background:
            average = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=True,
                NXPixel=self.number_of_pixels_x,
                NYPixel=self.number_of_pixels_y,
                ConvertToQ=False,
                XPixelMin=int(background_range[0]),
                XPixelMax=int(background_range[1]),
                YPixelMin=low_res_min,
                YPixelMax=low_res_max,
                ErrorWeighting=error_weighted_bck,
                SumPixels=True,
                NormalizeSum=True,
            )

            signal = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=True,
                NXPixel=self.number_of_pixels_x,
                NYPixel=self.number_of_pixels_y,
                ConvertToQ=False,
                YPixelMin=low_res_min,
                YPixelMax=low_res_max,
                OutputWorkspace="signal_%s" % str(workspace),
            )
            subtracted = Minus(LHSWorkspace=signal, RHSWorkspace=average, OutputWorkspace="subtracted_%s" % str(workspace))
            AnalysisDataService.remove(str(average))
            AnalysisDataService.remove(str(signal))
        else:
            # If we don't subtract the background, we still have to integrate
            # over the low resolution axis
            subtracted = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=True,
                NXPixel=self.number_of_pixels_x,
                NYPixel=self.number_of_pixels_y,
                ConvertToQ=False,
                YPixelMin=low_res_min,
                YPixelMax=low_res_max,
                OutputWorkspace="subtracted_%s" % str(workspace),
            )

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        try:
            NormaliseByCurrent(InputWorkspace=subtracted, OutputWorkspace=str(subtracted))
        except Exception as e:
            logger.error(str(e))  # allow continuation when normalization fails (e.g. no proton charge)
        finally:
            subtracted = mtd[str(subtracted)]

        # Crop to only the selected peak region
        cropped = CropWorkspace(
            InputWorkspace=subtracted,
            StartWorkspaceIndex=max(0, int(peak_range[0])),
            EndWorkspaceIndex=min(int(peak_range[1]), self.number_of_pixels_x - 1),
            OutputWorkspace="%s_cropped" % str(subtracted),
        )

        if rebin_to_ws is not None:
            cropped = SumSpectra(InputWorkspace=cropped)

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(workspace))
        AnalysisDataService.remove(str(subtracted))
        return cropped


AlgorithmFactory.subscribe(MagnetismReflectometryReduction)
