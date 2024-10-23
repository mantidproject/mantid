# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
This algorithm is a refactored version of the RefLReduction algorithm.
It was written in an attempt to:
  - Not rely on external code but only on algorithms.
  - Do work using existing algorithms as opposed to doing everything in arrays.
  - Keep the same parameters and work as a drop-in replacement for the old algorithm.
  - Reproduce the output of the old algorithm.
"""

import time
import math
import os
from mantid.api import (
    mtd,
    AlgorithmFactory,
    AnalysisDataService,
    FileFinder,
    MatrixWorkspaceProperty,
    PropertyMode,
    PythonAlgorithm,
    WorkspaceProperty,
)
from mantid.kernel import (
    logger,
    Direction,
    FloatArrayLengthValidator,
    FloatArrayProperty,
    IntArrayLengthValidator,
    IntArrayProperty,
    StringArrayProperty,
)
from mantid.simpleapi import (
    AddSampleLog,
    ConvertToPointData,
    CreateSingleValuedWorkspace,
    CropWorkspace,
    Divide,
    ExtractSingleSpectrum,
    Load,
    LoadEventNexus,
    LRPrimaryFraction,
    LRSubtractAverageBackground,
    Multiply,
    NormaliseByCurrent,
    Rebin,
    RebinToWorkspace,
    RefRoi,
    ReplaceSpecialValues,
    SortXAxis,
    SumSpectra,
)
from functools import reduce  # pylint: disable=redefined-builtin


class LiquidsReflectometryReduction(PythonAlgorithm):
    number_of_pixels_x = 0
    number_of_pixels_y = 0
    TOLERANCE = 0.0

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LiquidsReflectometryReduction"

    def version(self):
        return 1

    def summary(self):
        return "Liquids Reflectometer (REFL) reduction"

    def PyInit(self):
        # TODO: Revisit the choice of names when we are entirely rid of the old code.
        self.declareProperty(StringArrayProperty("RunNumbers"), "List of run numbers to process")
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Optionally, we can provide a workspace directly",
        )
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(
            IntArrayProperty("SignalPeakPixelRange", [123, 137], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the data peak",
        )
        self.declareProperty("SubtractSignalBackground", True, doc="If true, the background will be subtracted from the data peak")
        self.declareProperty(
            IntArrayProperty("SignalBackgroundPixelRange", [123, 137], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range defining the background. Default:(123,137)",
        )
        self.declareProperty(
            "ErrorWeighting",
            False,
            doc="If True, a weighted average is used to to estimate the subtracted background." "Otherwise, a simple average is used.",
        )
        self.declareProperty("NormFlag", True, doc="If true, the data will be normalized")
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
            "LowResDataAxisPixelRangeFlag",
            True,
            doc="If true, the low resolution direction of the data will be cropped according " + "to the lowResDataAxisPixelRange property",
        )
        self.declareProperty(
            IntArrayProperty("LowResDataAxisPixelRange", [115, 210], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range to use in the low resolution direction of the data",
        )
        self.declareProperty(
            "LowResNormAxisPixelRangeFlag",
            True,
            doc="If true, the low resolution direction of the normalization run will be cropped "
            + "according to the LowResNormAxisPixelRange property",
        )
        self.declareProperty(
            IntArrayProperty("LowResNormAxisPixelRange", [115, 210], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range to use in the low resolution direction of the normalizaion run",
        )
        self.declareProperty(
            FloatArrayProperty("TOFRange", [0.0, 340000.0], FloatArrayLengthValidator(2), direction=Direction.Input), "TOF range to use"
        )
        self.declareProperty("TOFRangeFlag", True, doc="If true, the TOF will be cropped according to the TOF range property")
        self.declareProperty("QMin", 0.05, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, doc="Angle offset error (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("ApplyScalingFactor", True, doc="If true, the scaling from Scaling Factor file will be applied")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")
        self.declareProperty("SlitTolerance", 0.02, doc="Tolerance for matching slit positions")
        self.declareProperty("SlitsWidthFlag", True, doc="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
        self.declareProperty("GeometryCorrectionFlag", False, doc="Use or not the geometry correction")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "Si", doc="Name of the back slit")
        self.declareProperty("TOFSteps", 40.0, doc="TOF step size")
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")
        self.declareProperty("ApplyPrimaryFraction", False, doc="If true, the primary fraction correction will be applied")
        self.declareProperty(
            IntArrayProperty("PrimaryFractionRange", [117, 197], IntArrayLengthValidator(2), direction=Direction.Input),
            "Pixel range to use for calculating the primary fraction correction.",
        )

    # pylint: disable=too-many-locals,too-many-branches
    def PyExec(self):  # noqa
        # The old reduction code had a tolerance value for matching the
        # slit parameters to get the scaling factors
        self.TOLERANCE = self.getProperty("SlitTolerance").value

        # DATA
        dataPeakRange = self.getProperty("SignalPeakPixelRange").value
        dataBackRange = self.getProperty("SignalBackgroundPixelRange").value

        # NORM
        normalizationRunNumber = self.getProperty("NormalizationRunNumber").value
        normBackRange = self.getProperty("NormBackgroundPixelRange").value
        normPeakRange = self.getProperty("NormPeakPixelRange").value

        # Get Q range
        qMin = self.getProperty("QMin").value
        qStep = self.getProperty("QStep").value
        if qStep > 0:  # force logarithmic binning
            qStep = -qStep

        # Load the data
        ws_event_data = self.load_data()

        # Compute the primary fraction using the unprocessed workspace
        apply_primary_fraction = self.getProperty("ApplyPrimaryFraction").value
        primary_fraction = [1.0, 0.0]
        if apply_primary_fraction:
            signal_range = self.getProperty("PrimaryFractionRange").value
            primary_fraction = LRPrimaryFraction(InputWorkspace=ws_event_data, SignalRange=signal_range)

        # Get the TOF range
        crop_TOF = self.getProperty("TOFRangeFlag").value
        tof_step = self.getProperty("TOFSteps").value
        if crop_TOF:
            TOFrange = self.getProperty("TOFRange").value  # microS
            if TOFrange[0] <= 0:
                TOFrange[0] = tof_step
                logger.error("Lower bound of TOF range cannot be zero: using %s" % tof_step)
        else:
            # If the TOF range option is turned off, use the full range
            # Protect against TOF=0, which will crash when going to Q.
            tof_min = ws_event_data.getTofMin()
            if tof_min <= 0:
                tof_min = tof_step
            tof_max = ws_event_data.getTofMax()
            TOFrange = [tof_min, tof_max]
            logger.information("Using TOF range: %g %g" % (tof_min, tof_max))

        # Number of pixels in each direction
        # TODO: revisit this when we update the IDF
        self.number_of_pixels_x = int(ws_event_data.getInstrument().getNumberParameter("number-of-x-pixels")[0])
        self.number_of_pixels_y = int(ws_event_data.getInstrument().getNumberParameter("number-of-y-pixels")[0])

        # Get scattering angle theta
        theta = self.calculate_scattering_angle(ws_event_data)
        two_theta_degrees = 2.0 * theta * 180.0 / math.pi
        AddSampleLog(Workspace=ws_event_data, LogName="two_theta", LogText=str(two_theta_degrees), LogType="Number")

        # ----- Process Sample Data -------------------------------------------
        crop_request = self.getProperty("LowResDataAxisPixelRangeFlag").value
        low_res_range = self.getProperty("LowResDataAxisPixelRange").value
        bck_request = self.getProperty("SubtractSignalBackground").value
        data_cropped = self.process_data(ws_event_data, TOFrange, crop_request, low_res_range, dataPeakRange, bck_request, dataBackRange)

        # ----- Normalization -------------------------------------------------
        perform_normalization = self.getProperty("NormFlag").value
        if perform_normalization:
            # Load normalization
            ws_event_norm = LoadEventNexus("REF_L_%s" % normalizationRunNumber, OutputWorkspace="REF_L_%s" % normalizationRunNumber)
            crop_request = self.getProperty("LowResNormAxisPixelRangeFlag").value
            low_res_range = self.getProperty("LowResNormAxisPixelRange").value
            bck_request = self.getProperty("SubtractNormBackground").value
            norm_cropped = self.process_data(
                ws_event_norm, TOFrange, crop_request, low_res_range, normPeakRange, bck_request, normBackRange
            )
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(ws_event_norm))

            # Sum up the normalization peak
            norm_summed = SumSpectra(InputWorkspace=norm_cropped)
            norm_summed = RebinToWorkspace(WorkspaceToRebin=norm_summed, WorkspaceToMatch=data_cropped, OutputWorkspace=str(norm_summed))

            # Sum up the normalization peak
            norm_summed = SumSpectra(InputWorkspace=norm_cropped)

            # Normalize the data
            normalized_data = data_cropped / norm_summed
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(data_cropped))
            AnalysisDataService.remove(str(norm_cropped))
            AnalysisDataService.remove(str(norm_summed))
            AddSampleLog(Workspace=normalized_data, LogName="normalization_run", LogText=str(normalizationRunNumber))
        else:
            normalized_data = data_cropped
            AddSampleLog(Workspace=normalized_data, LogName="normalization_run", LogText="None")

        # At this point, the workspace should be considered a distribution of points
        normalized_data = ConvertToPointData(InputWorkspace=normalized_data, OutputWorkspace=str(normalized_data))
        normalized_data.setDistribution(True)

        # Apply scaling factors
        apply_scaling_factor = self.getProperty("ApplyScalingFactor").value
        if apply_scaling_factor:
            normalized_data = self.apply_scaling_factor(normalized_data)

        q_workspace = SumSpectra(InputWorkspace=normalized_data)
        q_workspace.getAxis(0).setUnit("MomentumTransfer")

        # Geometry correction to convert To Q with correction
        geometry_correction_flag = self.getProperty("GeometryCorrectionFlag").value
        if geometry_correction_flag:
            logger.error("The geometry correction for the Q conversion has not been implemented.")

        # Get the distance fromthe moderator to the detector
        sample = ws_event_data.getInstrument().getSample()
        source = ws_event_data.getInstrument().getSource()
        source_sample_distance = sample.getDistance(source)
        detector = ws_event_data.getDetector(0)
        sample_detector_distance = detector.getPos().getZ()
        source_detector_distance = source_sample_distance + sample_detector_distance

        # Convert to Q
        # Use the TOF range to pick the maximum Q, and give it a little extra room.
        h = 6.626e-34  # m^2 kg s^-1
        m = 1.675e-27  # kg
        constant = 4e-4 * math.pi * m * source_detector_distance / h * math.sin(theta)
        q_range = [qMin, qStep, constant / TOFrange[0] * 1.2]

        q_min_from_data = constant / TOFrange[1]
        q_max_from_data = constant / TOFrange[0]
        AddSampleLog(Workspace=q_workspace, LogName="q_min", LogText=str(q_min_from_data), LogType="Number")
        AddSampleLog(Workspace=q_workspace, LogName="q_max", LogText=str(q_max_from_data), LogType="Number")

        tof_to_lambda = 1.0e4 * h / (m * source_detector_distance)
        lambda_min = tof_to_lambda * TOFrange[0]
        lambda_max = tof_to_lambda * TOFrange[1]
        AddSampleLog(Workspace=q_workspace, LogName="lambda_min", LogText=str(lambda_min), LogType="Number")
        AddSampleLog(Workspace=q_workspace, LogName="lambda_max", LogText=str(lambda_max), LogType="Number")

        data_x = q_workspace.dataX(0)
        for i in range(len(data_x)):
            data_x[i] = constant / data_x[i]
        q_workspace = SortXAxis(InputWorkspace=q_workspace, OutputWorkspace=str(q_workspace))

        # Cook up a name compatible with the UI for backward compatibility
        _time = int(time.time())
        name_output_ws = self.getPropertyValue("OutputWorkspace")
        name_output_ws = name_output_ws + "_#" + str(_time) + "ts"

        q_rebin = Rebin(InputWorkspace=q_workspace, Params=q_range, OutputWorkspace=name_output_ws)

        # Apply the primary fraction
        if apply_primary_fraction:
            ws_fraction = CreateSingleValuedWorkspace(DataValue=primary_fraction[0], ErrorValue=primary_fraction[1])
            q_rebin = Multiply(LHSWorkspace=q_rebin, RHSWorkspace=ws_fraction, OutputWorkspace=name_output_ws)
        AddSampleLog(Workspace=q_rebin, LogName="primary_fraction", LogText=str(primary_fraction[0]), LogType="Number")
        AddSampleLog(Workspace=q_rebin, LogName="primary_fraction_error", LogText=str(primary_fraction[1]), LogType="Number")

        # Replace NaNs by zeros
        q_rebin = ReplaceSpecialValues(InputWorkspace=q_rebin, OutputWorkspace=name_output_ws, NaNValue=0.0, NaNError=0.0)
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

        crop = self.getProperty("CropFirstAndLastPoints").value
        if low_q is not None and high_q is not None:
            # Get rid of first and last Q points to avoid edge effects
            if crop:
                low_q += 1
                high_q -= 1
            data_x = q_rebin.readX(0)
            q_rebin = CropWorkspace(InputWorkspace=q_rebin, OutputWorkspace=str(q_rebin), XMin=data_x[low_q], XMax=data_x[high_q])
        else:
            logger.error("Data is all zeros. Check your TOF ranges.")

        # Clean up the workspace for backward compatibility
        data_y = q_rebin.dataY(0)
        data_e = q_rebin.dataE(0)
        # Again for backward compatibility, the first and last points of the
        # raw output when not cropping was simply set to 0 += 1.
        if crop is False:
            data_y[0] = 0
            data_e[0] = 1
            data_y[len(data_y) - 1] = 0
            data_e[len(data_y) - 1] = 1
        # Values < 1e-12 and values where the error is greater than the value are replaced by 0+-1
        for i in range(len(data_y)):
            if data_y[i] < 1e-12 or data_e[i] > data_y[i]:
                data_y[i] = 0.0
                data_e[i] = 1.0

        # Sanity check
        if sum(data_y) == 0:
            raise RuntimeError("The reflectivity is all zeros: check your peak selection")

        # Avoid leaving trash behind
        for ws in ["ws_event_data", "normalized_data", "q_workspace"]:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)

        self.setProperty("OutputWorkspace", mtd[name_output_ws])

    def load_data(self):
        """
        Load the data. We can either load it from the specified
        run numbers, or use the input workspace if no runs are specified.
        """
        dataRunNumbers = self.getProperty("RunNumbers").value
        ws_event_data = self.getProperty("InputWorkspace").value

        if len(dataRunNumbers) > 0:
            # If we have multiple files, add them
            file_list = []
            for item in dataRunNumbers:
                # The standard mode of operation is to give a run number as input
                try:
                    data_file = FileFinder.findRuns("REF_L%s" % item)[0]
                except RuntimeError:
                    # Allow for a file name or file path as input
                    data_file = FileFinder.findRuns(item)[0]
                file_list.append(data_file)
            runs = reduce((lambda x, y: "%s+%s" % (x, y)), file_list)
            ws_event_data = Load(Filename=runs, OutputWorkspace="REF_L_%s" % dataRunNumbers[0])
        elif ws_event_data is None:
            raise RuntimeError("No input data was specified")
        return ws_event_data

    def calculate_scattering_angle(self, ws_event_data):
        """
        Compute the scattering angle
        @param ws_event_data: data workspace
        """
        run_object = ws_event_data.getRun()
        thi_value = run_object.getProperty("thi").value[0]
        thi_units = run_object.getProperty("thi").units
        tthd_value = run_object.getProperty("tthd").value[0]
        tthd_units = run_object.getProperty("tthd").units

        # Make sure we have radians
        if thi_units.lower().startswith("deg"):
            thi_value *= math.pi / 180.0
        if tthd_units.lower().startswith("deg"):
            tthd_value *= math.pi / 180.0

        theta = math.fabs(tthd_value - thi_value) / 2.0
        if theta < 0.001:
            logger.warning("thi and tthd are equal: is this a direct beam?")

        # Add the offset
        angle_offset_deg = self.getProperty("AngleOffset").value
        return theta + angle_offset_deg * math.pi / 180.0

    # pylint: disable=too-many-arguments
    def process_data(self, workspace, tof_range, crop_low_res, low_res_range, peak_range, subtract_background, background_range):
        """
        Common processing for both sample data and normalization.
        """
        # TODO: The rebin and crop approach is used to be consistent with the old code.
        #      This should be replaced in the future.

        # Rebin TOF axis
        tof_max = workspace.getTofMax()
        tof_min = workspace.getTofMin()
        if tof_min > tof_range[1] or tof_max < tof_range[0]:
            error_msg = "Requested TOF range does not match data for %s: " % str(workspace)
            error_msg += "[%g, %g] found [%g, %g]" % (tof_range[0], tof_range[1], tof_min, tof_max)
            raise RuntimeError(error_msg)

        tof_step = self.getProperty("TOFSteps").value
        workspace = Rebin(
            InputWorkspace=workspace, Params=[0, tof_step, tof_max], PreserveEvents=True, OutputWorkspace="%s_histo" % str(workspace)
        )

        # Crop TOF range
        workspace = CropWorkspace(InputWorkspace=workspace, XMin=tof_range[0], XMax=tof_range[1], OutputWorkspace=str(workspace))

        # Integrate over low resolution range
        x_min = 0
        x_max = self.number_of_pixels_x
        if crop_low_res:
            x_min = int(low_res_range[0])
            x_max = int(low_res_range[1])

        # Subtract background
        if subtract_background:
            err_weight = self.getProperty("ErrorWeighting").value
            workspace = LRSubtractAverageBackground(
                InputWorkspace=workspace,
                PeakRange=peak_range,
                BackgroundRange=background_range,
                LowResolutionRange=[x_min, x_max],
                OutputWorkspace=str(workspace),
                ErrorWeighting=err_weight,
            )
        else:
            # If we don't subtract the background, we still have to integrate
            # over the low resolution axis
            workspace = RefRoi(
                InputWorkspace=workspace,
                IntegrateY=False,
                NXPixel=self.number_of_pixels_x,
                NYPixel=self.number_of_pixels_y,
                ConvertToQ=False,
                XPixelMin=x_min,
                XPixelMax=x_max,
                OutputWorkspace=str(workspace),
            )

        # Normalize by current proton charge
        # Note that the background subtraction will use an error weighted mean
        # and use 1 as the error on counts of zero. We normalize by the integrated
        # current _after_ the background subtraction so that the 1 doesn't have
        # to be changed to a 1/Charge.
        workspace = NormaliseByCurrent(InputWorkspace=workspace, OutputWorkspace=str(workspace))

        # Crop to only the selected peak region
        cropped = CropWorkspace(
            InputWorkspace=workspace,
            StartWorkspaceIndex=int(peak_range[0]),
            EndWorkspaceIndex=int(peak_range[1]),
            OutputWorkspace="%s_cropped" % str(workspace),
        )

        # Avoid leaving trash behind
        AnalysisDataService.remove(str(workspace))

        return cropped

    # pylint: disable=too-many-locals,too-many-branches
    def apply_scaling_factor(self, workspace):  # noqa
        """
        Apply scaling factor from reference scaling data
        @param workspace: Mantid workspace
        """
        scaling_factor_file = self.getProperty("ScalingFactorFile").value
        if not os.path.isfile(scaling_factor_file):
            scaling_factor_files = FileFinder.findRuns(scaling_factor_file)
            if len(scaling_factor_files) > 0:
                scaling_factor_file = scaling_factor_files[0]
                if not os.path.isfile(scaling_factor_file):
                    logger.error("Could not find scaling factor file %s" % scaling_factor_file)
                    return workspace
            else:
                logger.error("Could not find scaling factor file %s" % scaling_factor_file)
                return workspace

        # Get the incident medium
        incident_medium = self.getProperty("IncidentMediumSelected").value

        # Get the wavelength
        lr = workspace.getRun().getProperty("LambdaRequest").value[0]
        lr_value = float("{0:.2f}".format(lr))

        # Get the slit information
        front_slit = self.getProperty("FrontSlitName").value
        back_slit = self.getProperty("BackSlitName").value

        # Option to match slit widths or not
        match_slit_width = self.getProperty("SlitsWidthFlag").value

        s1h = abs(workspace.getRun().getProperty("%sVHeight" % front_slit).value[0])
        s1w = abs(workspace.getRun().getProperty("%sHWidth" % front_slit).value[0])
        try:
            s2h = abs(workspace.getRun().getProperty("%sVHeight" % back_slit).value[0])
            s2w = abs(workspace.getRun().getProperty("%sHWidth" % back_slit).value[0])
        except RuntimeError:
            # For backward compatibility with old code
            logger.error("Specified slit could not be found: %s  Trying S2" % back_slit)
            s2h = abs(workspace.getRun().getProperty("S2VHeight").value[0])
            s2w = abs(workspace.getRun().getProperty("S2HWidth").value[0])

        scaling_info = "Scaling settings: %s wl=%s S1H=%s S2H=%s" % (incident_medium, lr_value, s1h, s2h)
        if match_slit_width:
            scaling_info += " S1W=%s S2W=%s" % (s1w, s2w)
        logger.information(scaling_info)

        def _reduce(accumulation, item):
            """
            Reduce function that accumulates values in a dictionary
            """
            toks_item = item.split("=")
            if len(toks_item) != 2:
                return accumulation
            if isinstance(accumulation, dict):
                accumulation[toks_item[0].strip()] = toks_item[1].strip()
            else:
                toks_accum = accumulation.split("=")
                accumulation = {toks_item[0].strip(): toks_item[1].strip(), toks_accum[0].strip(): toks_accum[1].strip()}
            return accumulation

        def _value_check(key, data, reference):
            """
            Check an entry against a reference value
            """
            if key in data:
                return abs(abs(float(data[key])) - abs(float(reference))) <= self.TOLERANCE
            return False

        scaling_data = open(scaling_factor_file, "r")
        file_content = scaling_data.read()
        scaling_data.close()

        data_found = None
        for line in file_content.split("\n"):
            if line.startswith("#"):
                continue

            # Parse the line of data and produce a dict
            toks = line.split()
            data_dict = reduce(_reduce, toks, {})

            # Get ordered list of keys
            keys = []
            for token in toks:
                key_value = token.split("=")
                if len(key_value) == 2:
                    keys.append(key_value[0].strip())

            # Skip empty lines
            if len(keys) == 0:
                continue
            # Complain if the format is non-standard
            elif len(keys) < 10:
                logger.error("Bad scaling factor entry\n  %s" % line)
                continue

            # Sanity check
            if keys[0] != "IncidentMedium" and keys[1] != "LambdaRequested" and keys[2] != "S1H":
                logger.error("The scaling factor file isn't standard: bad keywords")
            # The S2H key has been changing in the earlier version of REFL reduction.
            # Get the key from the data to make sure we are backward compatible.
            s2h_key = keys[3]
            s2w_key = keys[5]
            if (
                "IncidentMedium" in data_dict
                and data_dict["IncidentMedium"].lower() == incident_medium.strip().lower()
                and _value_check("LambdaRequested", data_dict, lr_value)
                and _value_check("S1H", data_dict, s1h)
                and _value_check(s2h_key, data_dict, s2h)
            ):
                if not match_slit_width or (_value_check("S1W", data_dict, s1w) and _value_check(s2w_key, data_dict, s2w)):
                    data_found = data_dict
                    break

        AddSampleLog(Workspace=workspace, LogName="isSFfound", LogText=str(data_found is not None))
        if data_found is not None:
            a = float(data_found["a"])
            b = float(data_found["b"])
            a_error = float(data_found["error_a"])
            b_error = float(data_found["error_b"])
            AddSampleLog(Workspace=workspace, LogName="scaling_factor_a", LogText=str(a), LogType="Number")
            AddSampleLog(Workspace=workspace, LogName="scaling_factor_b", LogText=str(b), LogType="Number")
            AddSampleLog(Workspace=workspace, LogName="scaling_factor_a_error", LogText=str(a_error), LogType="Number")
            AddSampleLog(Workspace=workspace, LogName="scaling_factor_b_error", LogText=str(b_error), LogType="Number")

            # Extract a single spectrum, just so we have the TOF axis
            # to create a normalization workspace
            normalization = ExtractSingleSpectrum(InputWorkspace=workspace, OutputWorkspace="normalization", WorkspaceIndex=0)
            norm_tof = normalization.dataX(0)
            norm_value = normalization.dataY(0)
            norm_error = normalization.dataE(0)
            for i in range(len(norm_value)):
                norm_value[i] = norm_tof[i] * b + a
                norm_error[i] = math.sqrt(a_error * a_error + norm_tof[i] * norm_tof[i] * b_error * b_error)

            workspace = Divide(LHSWorkspace=workspace, RHSWorkspace=normalization, OutputWorkspace=str(workspace))
            # Avoid leaving trash behind
            AnalysisDataService.remove(str(normalization))
        else:
            logger.error("Could not find scaling factor for %s" % str(incident_medium))
        return workspace


AlgorithmFactory.subscribe(LiquidsReflectometryReduction)
