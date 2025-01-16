# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import (
    CompositeValidator,
    Direction,
    EnabledWhenProperty,
    IntArrayLengthValidator,
    IntArrayBoundedValidator,
    IntArrayProperty,
    IntBoundedValidator,
    StringListValidator,
    PropertyCriterion,
    PropertyManagerProperty,
)
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    PropertyMode,
    WorkspaceUnitValidator,
)
from mantid.simpleapi import (
    CalculatePolynomialBackground,
    CloneWorkspace,
    ConvertUnits,
    CreateWorkspace,
    Divide,
    ExtractMonitors,
    Fit,
    Integration,
    LoadAndMerge,
    logger,
    Minus,
    MoveInstrumentComponent,
    mtd,
    NormaliseToMonitor,
    Scale,
    SpecularReflectionPositionCorrect,
    RebinToWorkspace,
    RotateInstrumentComponent,
    Transpose,
)
import ReflectometryILL_common as common
import ILL_utilities as utils

from math import fabs, atan
import numpy as np
from scipy.constants import physical_constants
from typing import List, Tuple


class Prop:
    START_WS_INDEX = "FitStartWorkspaceIndex"
    END_WS_INDEX = "FitEndWorkspaceIndex"
    XMIN = "FitRangeLower"
    XMAX = "FitRangeUpper"
    BKG_METHOD = "FlatBackground"
    CLEANUP = "Cleanup"
    DIRECT_LINE_WORKSPACE = "DirectLineWorkspace"
    FLUX_NORM_METHOD = "FluxNormalisation"
    FOREGROUND_HALF_WIDTH = "ForegroundHalfWidth"
    HIGH_BKG_OFFSET = "HighAngleBkgOffset"
    HIGH_BKG_WIDTH = "HighAngleBkgWidth"
    LINE_POSITION = "LinePosition"
    LOW_BKG_OFFSET = "LowAngleBkgOffset"
    LOW_BKG_WIDTH = "LowAngleBkgWidth"
    OUTPUT_WS = "OutputWorkspace"
    RUN = "Run"
    SLIT_NORM = "SlitNormalisation"
    THETA = "BraggAngle"
    SUBALG_LOGGING = "SubalgorithmLogging"
    WATER_REFERENCE = "WaterWorkspace"


class BkgMethod:
    AVERAGE = "Background Average"
    CONSTANT = "Background Constant Fit"
    LINEAR = "Background Linear Fit"
    OFF = "Background OFF"


class FluxNormMethod:
    MONITOR = "Normalise To Monitor"
    TIME = "Normalise To Time"
    OFF = "Normalisation OFF"


class SlitNorm:
    AUTO = "Slit Normalisation AUTO"
    OFF = "Slit Normalisation OFF"
    ON = "Slit Normalisation ON"


class SubalgLogging:
    OFF = "Logging OFF"
    ON = "Logging ON"


def normalisation_monitor_workspace_index(ws: MatrixWorkspace) -> int:
    """Return the spectrum number of the monitor used for normalisation.

    Keyword arguments:
    ws -- workspace used to extract the monitor spectrum metadata
    """
    param_name = "default-incident-monitor-spectrum"
    instr = ws.getInstrument()
    if not instr.hasParameter(param_name):
        raise RuntimeError("Parameter " + param_name + " is missing from the IPF.")
    n = instr.getIntParameter(param_name)[0]
    return ws.getIndexFromSpectrumNumber(n)


class ReflectometryILLPreprocess(DataProcessorAlgorithm):
    _bragg_angle = None
    _theta_zero = None
    _instrument_name = None

    @staticmethod
    def _calculate_average_background(
        transposed_ws: MatrixWorkspace, transposed_bkg_ws_name: MatrixWorkspace, ranges: List[float]
    ) -> MatrixWorkspace:
        """Calculates mean background in the specified detector region.

        Keyword arguments:
        transposed_ws -- transposed workspace for flat background calculation
        transposed_bkg_ws_name -- name to be assigned to the transposed workspace clone
        ranges -- range on which the flat background shall be calculated
        """
        n_spec = transposed_ws.getNumberHistograms()
        block_size = transposed_ws.blocksize()
        y = transposed_ws.extractY()
        x = transposed_ws.extractX()
        transposed_bkg_ws = CloneWorkspace(InputWorkspace=transposed_ws, OutputWorkspace=transposed_bkg_ws_name)
        condition = (x >= ranges[0]) & (x <= ranges[1])
        if len(ranges) == 4:
            condition = ((x >= ranges[0]) & (x <= ranges[1])) | ((x >= ranges[2]) & (x <= ranges[3]))
        bkg_region = np.extract(condition, y)
        bkg_region = bkg_region.reshape((n_spec, int(bkg_region.size / n_spec)))
        bkg = np.mean(bkg_region, axis=1)
        for channel in range(n_spec):
            transposed_bkg_ws.setE(channel, np.zeros(block_size))
            transposed_bkg_ws.setY(channel, bkg[channel] * np.ones(block_size))
        return transposed_bkg_ws

    @staticmethod
    def _check_angles_match(input_files: str) -> None:
        """Checks if the sample and detector angles difference between the loaded data is close enough.

        Keyword parameters:
        input_files -- string containing paths to NeXus files to be checked
        """
        tolerance = 0.1  # degree
        file_list = input_files.split("+")
        san = common.sample_angle(file_list[0])
        dan = common.detector_angle(file_list[0])
        r = file_list[0][file_list[0].rfind("/") + 1 : -4]
        for file_no in range(1, len(file_list)):
            file_name = file_list[file_no]
            san_test = common.sample_angle(file_name)
            dan_test = common.detector_angle(file_name)
            r_test = file_name[file_list[0].rfind("/") + 1 : -4]
            if fabs(san - san_test) > tolerance:
                logger.warning("Different sample angles detected! {0}: {1}, {2}: {3}".format(r, san, r_test, san_test))
            if fabs(dan - dan_test) > tolerance:
                logger.warning("Different detector angles detected! {0}: {1}, {2}: {3}".format(r, dan, r_test, dan_test))

    @staticmethod
    def _recalculate_average_chopper_params(ws: MatrixWorkspace) -> None:
        """Recalculates averages of chopper 1 and 2 speed and phase using time as weight for recalculation. The new averages
        are used to update the sample logs.

        Keyword arguments:
        ws -- workspace which may require chopper parameter recalculation
        """
        run = ws.run()
        time = run.getProperty("time").value
        logs_to_average = ["chopper1.speed_average", "chopper1.phase_average", "chopper2.speed_average", "chopper2.phase_average"]
        for log in logs_to_average:
            if run.hasProperty(log):
                param = run.getProperty(log).value
                if not isinstance(param, np.ndarray):
                    continue
                param = np.sum(param * time) / np.sum(time)
                run.addProperty(log, float(param), True)

    @staticmethod
    def _workspace_index_direction(ws: MatrixWorkspace) -> int:
        """Returns 1 if workspace indices increase with Bragg angle, otherwise returns -1.

        Keyword arguments:
        ws -- workspace to be analysed for the index direction.
        """
        first_det = ws.getDetector(0)
        first_angle = ws.detectorTwoTheta(first_det)
        last_det = ws.getDetector(ws.getNumberHistograms() - 1)
        last_angle = ws.detectorTwoTheta(last_det)
        return 1 if first_angle < last_angle else -1

    def category(self):
        """Return the categories of the algrithm."""
        return "ILL\\Reflectometry;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryILLPreprocess"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Loads, merges, normalizes and subtracts background from ILL reflectometry data."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return [
            "ReflectometryILLConvertToQ",
            "ReflectometryILLPolarizationCor",
            "ReflectometryILLSumForeground",
            "ReflectometryILLAutoProcess",
        ]

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if self.getPropertyValue("Measurement") == "ReflectedBeam":
            angle_option = self.getPropertyValue("AngleOption")
            if angle_option == "UserAngle" and self.getProperty("BraggAngle").isDefault:
                issues["BraggAngle"] = "Bragg angle is mandatory if the angle option is UserAngle"
            elif angle_option == "DetectorAngle":
                if self.getProperty("DirectBeamDetectorAngle").isDefault:
                    issues["DirectBeamDetectorAngle"] = "Direct beam detector angle is mandatory if the angle option is DetectorAngle"
                if self.getProperty("DirectBeamForegroundCentre").isDefault:
                    issues["DirectBeamForegroundCentre"] = "Direct beam foreground centre is mandatory if the angle option is DetectorAngle"

        if self.getProperty(Prop.BKG_METHOD).value != BkgMethod.OFF:
            if self.getProperty(Prop.LOW_BKG_WIDTH).value == 0 and self.getProperty(Prop.HIGH_BKG_WIDTH).value == 0:
                issues[Prop.BKG_METHOD] = "Cannot calculate flat background if both upper and lower background / widths are zero."
        # Early input validation to prevent FindReflectometryLines to fail its validation
        if not self.getProperty(Prop.XMIN).isDefault and not self.getProperty(Prop.XMAX).isDefault:
            x_min = self.getProperty(Prop.XMIN).value
            x_max = self.getProperty(Prop.XMAX).value
            if x_max < x_min:
                issues[Prop.XMIN] = "Must be smaller than RangeUpper."
            if x_min < 0.0:
                issues[Prop.XMIN] = "Must be larger or equal than 0.0."
            if x_max > 255.0:
                issues[Prop.XMAX] = "Must be smaller or equal than 255.0."
        if not self.getProperty(Prop.START_WS_INDEX).isDefault and not self.getProperty(Prop.END_WS_INDEX).isDefault:
            min_index = self.getProperty(Prop.START_WS_INDEX).value
            max_index = self.getProperty(Prop.END_WS_INDEX).value
            if max_index < min_index:
                issues[Prop.START_WS_INDEX] = "Must be smaller than EndWorkspaceIndex."
        return issues

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        non_negative_int = IntBoundedValidator(lower=0)
        ws_index_range = IntBoundedValidator(lower=0, upper=255)
        non_negative_int_array = IntArrayBoundedValidator(lower=0)
        max_two_non_negative_ints = CompositeValidator()
        max_two_non_negative_ints.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        max_two_non_negative_ints.add(non_negative_int_array)

        self.declareProperty(MultipleFileProperty(Prop.RUN, extensions=["nxs"]), doc="A list of input run numbers/files.")

        self.declareProperty(
            MatrixWorkspaceProperty(Prop.OUTPUT_WS, defaultValue="", direction=Direction.Output),
            doc="The preprocessed output workspace (unit wavelength), single histogram.",
        )

        self.declareProperty(
            "Measurement",
            "DirectBeam",
            StringListValidator(["DirectBeam", "ReflectedBeam"]),
            "Whether to process as direct or reflected beam.",
        )

        self.declareProperty("AngleOption", "SampleAngle", StringListValidator(["SampleAngle", "DetectorAngle", "UserAngle"]))

        self.setPropertySettings("AngleOption", EnabledWhenProperty("Measurement", PropertyCriterion.IsEqualTo, "ReflectedBeam"))

        self.declareProperty(
            Prop.THETA, defaultValue=-1.0, doc="The bragg angle for reflected beam [Degree], used if angle option is UserAngle."
        )

        self.setPropertySettings(Prop.THETA, EnabledWhenProperty("AngleOption", PropertyCriterion.IsEqualTo, "UserAngle"))

        self.declareProperty(
            "DirectBeamDetectorAngle",
            -1.0,
            "The detector angle value [Degree] for the corresponding direct beam, used if angle option is DetectorAngle",
        )

        self.declareProperty(
            "DirectBeamForegroundCentre", -1.0, "Fractional pixel index for the direct beam, used if angle option is DetectorAngle."
        )

        self.setPropertySettings(
            "DirectBeamDetectorAngle", EnabledWhenProperty("AngleOption", PropertyCriterion.IsEqualTo, "DetectorAngle")
        )

        self.setPropertySettings(
            "DirectBeamForegroundCentre", EnabledWhenProperty("AngleOption", PropertyCriterion.IsEqualTo, "DetectorAngle")
        )

        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc="Enable or disable child algorithm logging.",
        )

        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=utils.Cleanup.ON,
            validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
            doc="Enable or disable intermediate workspace cleanup.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.WATER_REFERENCE,
                defaultValue="",
                direction=Direction.Input,
                validator=WorkspaceUnitValidator("TOF"),
                optional=PropertyMode.Optional,
            ),
            doc="A (water) calibration workspace (unit TOF).",
        )

        self.declareProperty(
            Prop.SLIT_NORM,
            defaultValue=SlitNorm.AUTO,
            validator=StringListValidator([SlitNorm.AUTO, SlitNorm.OFF, SlitNorm.ON]),
            doc="Enable or disable slit normalisation.",
        )

        self.declareProperty(
            Prop.FLUX_NORM_METHOD,
            defaultValue=FluxNormMethod.TIME,
            validator=StringListValidator([FluxNormMethod.TIME, FluxNormMethod.MONITOR, FluxNormMethod.OFF]),
            doc="Neutron flux normalisation method.",
        )

        self.declareProperty(
            IntArrayProperty(Prop.FOREGROUND_HALF_WIDTH, validator=max_two_non_negative_ints),
            doc="Number of foreground pixels at lower and higher angles from the centre pixel.",
        )

        self.declareProperty(
            Prop.BKG_METHOD,
            defaultValue=BkgMethod.AVERAGE,
            validator=StringListValidator([BkgMethod.AVERAGE, BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
            doc="Flat background calculation method for background subtraction.",
        )

        self.declareProperty(
            Prop.LOW_BKG_OFFSET,
            defaultValue=7,
            validator=non_negative_int,
            doc="Distance of flat background region towards smaller detector angles from the foreground centre, in pixels.",
        )

        self.declareProperty(
            Prop.LOW_BKG_WIDTH,
            defaultValue=5,
            validator=non_negative_int,
            doc="Width of flat background region towards smaller detector angles from the foreground centre, in pixels.",
        )

        self.declareProperty(
            Prop.HIGH_BKG_OFFSET,
            defaultValue=7,
            validator=non_negative_int,
            doc="Distance of flat background region towards larger detector angles from the foreground centre, in pixels.",
        )

        self.declareProperty(
            Prop.HIGH_BKG_WIDTH,
            defaultValue=5,
            validator=non_negative_int,
            doc="Width of flat background region towards larger detector angles from the foreground centre, in pixels.",
        )

        self.declareProperty(
            Prop.START_WS_INDEX, validator=ws_index_range, defaultValue=0, doc="Start workspace index used for peak fitting."
        )

        self.declareProperty(
            Prop.END_WS_INDEX, validator=ws_index_range, defaultValue=255, doc="Last workspace index used for peak fitting."
        )

        self.declareProperty(Prop.XMIN, defaultValue=-1.0, doc="Minimum wavelength [Angstrom] used for peak fitting.")

        self.declareProperty(Prop.XMAX, defaultValue=-1.0, doc="Maximum wavelength [Angstrom] used for peak fitting.")

        self.declareProperty(name="CorrectGravity", defaultValue=False, doc="Whether to correct for gravity effects (FIGARO only).")

        self.declareProperty(PropertyManagerProperty("LogsToReplace", dict()), doc="Sample logs to be overwritten.")

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(wsPrefix, cleanupMode)

        ws = self._input_ws()

        self._instrument_name = ws.getInstrument().getName()

        ws, mon_ws = self._extract_monitors(ws)

        if len((self.getPropertyValue(Prop.RUN)).split("+")) > 1:
            # foreground refitting is needed if there is more than one merged numor
            self._refit_foreground(ws)

        self._recalculate_average_chopper_params(ws)
        self._add_sample_log_info(ws)

        if self.getPropertyValue("AngleOption") == "DetectorAngle" and self.getPropertyValue("Measurement") == "ReflectedBeam":
            # we still have to do this after having loaded and found the foreground centre of the reflected beam
            ws = self._calibrate_detector_angle_by_direct_beam(ws)

        ws = self._water_calibration(ws)

        ws = self._normalise_to_slits(ws)

        ws = self._normalise_to_flux(ws, mon_ws)
        self._cleanup.cleanup(mon_ws)

        ws = self._subtract_flat_bkg(ws)

        ws = self._convert_to_wavelength(ws)

        if self.getProperty("CorrectGravity").value and self._instrument_name == "FIGARO":
            ws = self._gravity_correction(ws)

        self._finalize(ws)

    def _add_sample_log_info(self, ws: MatrixWorkspace) -> None:
        """Add foreground indices (start, center, end), names start with reduction.

        Keyword arguments:
        ws -- workspace to which the sample logs are added
        """
        run = ws.run()
        hws = self._foreground_widths()
        foreground_centre = run.getProperty(common.SampleLogs.LINE_POSITION).value
        sign = self._workspace_index_direction(ws)
        start_index = foreground_centre - sign * hws[0]
        end_index = foreground_centre + sign * hws[1]
        if start_index > end_index:
            end_index, start_index = start_index, end_index
        # note that those 3 are integers, but he line position is fractional
        run.addProperty(common.SampleLogs.FOREGROUND_START, int(start_index), True)
        run.addProperty(common.SampleLogs.FOREGROUND_CENTRE, int(foreground_centre), True)
        run.addProperty(common.SampleLogs.FOREGROUND_END, int(end_index), True)

    def _calibrate_detector_angle_by_direct_beam(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Perform detector position correction for reflected beams."""
        calibrated_ws_name = self._names.withSuffix("reflected_beam_calibration")
        direct_line = self.getProperty("DirectBeamForegroundCentre").value
        run = ws.getRun()
        refl_line = run.getLogData("reduction.line_position").value
        if self._instrument_name == "FIGARO":
            # Figaro requires an extra correction to its position equal to difference between direct and reflected
            # lines and their instrument centre
            pixel_width = common.pixel_size(self._instrument_name)
            sample_det_distance = run.getLogData("L2").value
            peak_centre = 127.5  # centre of the instrument
            ref_down = run.getLogData("refdown").value
            dir_angle = (180.0 / np.pi) * atan((direct_line - peak_centre) * pixel_width / sample_det_distance)
            refl_angle = (180.0 / np.pi) * atan((refl_line - peak_centre) * pixel_width / sample_det_distance)
            sign = -1.0 if ref_down else 1.0
            delta_angle = sign * (refl_angle - dir_angle)
            line_position = int(refl_line)
        else:
            line_position = direct_line
            delta_angle = 0.0
            sign = 1.0
        self._theta_zero = sign * (2.0 * sign * self._theta_from_detector_angles() + delta_angle)
        # LinePosition parameter below used to be defined as direct_line for both D17 and FIGARO,
        # but COSMOS for FIGARO assumes the foreground centre of the reflected beam to be exactly at the value given
        # by the _theta_zero variable, and not with regards to the direct beam as for D17.
        # For FIGARO, the direct beam position correction is handled by the delta_angle offset.
        calibrated_ws = SpecularReflectionPositionCorrect(
            InputWorkspace=ws,
            OutputWorkspace=calibrated_ws_name,
            DetectorComponentName="detector",
            LinePosition=line_position,  # specular line position
            TwoTheta=self._theta_zero,
            PixelSize=common.pixel_size(self._instrument_name),
            DetectorCorrectionType="RotateAroundSample",
            DetectorFacesSample=True,
            EnableLogging=self._subalgLogging,
        )

        self._cleanup.cleanup(ws)
        return calibrated_ws

    def _convert_to_wavelength(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Convert the X units of ws to wavelength.

        Keyword arguments:
        ws -- workspace to be converted to wavelength
        """
        wavelength_ws_name = self._names.withSuffix("in_wavelength")
        wavelength_ws = ConvertUnits(
            InputWorkspace=ws, OutputWorkspace=wavelength_ws_name, Target="Wavelength", EMode="Elastic", EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(ws)
        return wavelength_ws

    def _extract_monitors(self, ws: MatrixWorkspace) -> Tuple[MatrixWorkspace, MatrixWorkspace]:
        """Extract monitor spectra from ws to another workspace.

        Keyword arguments:
        ws -- workspace used for extracting detectors and monitors.
        """
        det_ws_name = self._names.withSuffix("detectors")
        mon_ws_name = self._names.withSuffix("monitors")
        ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=det_ws_name, MonitorWorkspace=mon_ws_name, EnableLogging=self._subalgLogging)
        if mtd.doesExist(det_ws_name) is None:
            raise RuntimeError("No detectors in the input data.")
        det_ws = mtd[det_ws_name]
        mon_ws = mtd[mon_ws_name] if mtd.doesExist(mon_ws_name) else None
        self._cleanup.cleanup(ws)
        return det_ws, mon_ws

    def _finalize(self, ws: MatrixWorkspace) -> None:
        """Set OutputWorkspace to ws and clean up.

        Keyword arguments:
        ws -- workspace to be set as output of this algorithm
        """
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _flat_bkg_ranges(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Return spectrum number ranges for flat background fitting.

        Keyword arguments:
        ws -- workspace used to extract flat background ranges from
        """
        sign = self._workspace_index_direction(ws)
        peak_pos = ws.run().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
        # Convert to spectrum numbers
        peak_pos = ws.getSpectrum(peak_pos).getSpectrumNo()
        peak_half_widths = self._foreground_widths()
        low_peak_half_width = peak_half_widths[0]
        low_offset = self.getProperty(Prop.LOW_BKG_OFFSET).value
        low_width = self.getProperty(Prop.LOW_BKG_WIDTH).value
        low_start_index = peak_pos - sign * (low_peak_half_width + low_offset + low_width)
        low_end_index = low_start_index + sign * low_width
        high_peak_half_width = peak_half_widths[1]
        high_offset = self.getProperty(Prop.HIGH_BKG_OFFSET).value
        high_width = self.getProperty(Prop.HIGH_BKG_WIDTH).value
        high_start_index = peak_pos + sign * (high_peak_half_width + high_offset)
        high_end_index = high_start_index + sign * high_width
        if sign > 0:
            low_range = [low_start_index - sign * 0.5, low_end_index - sign * 0.5]
            high_range = [high_start_index + sign * 0.5, high_end_index + sign * 0.5]
            return low_range + high_range
        # Indices decrease with increasing bragg angle. Swap everything.
        low_range = [low_end_index - sign * 0.5, low_start_index - sign * 0.5]
        high_range = [high_end_index + sign * 0.5, high_start_index + sign * 0.5]
        return high_range + low_range

    def _foreground_widths(self) -> List[float]:
        """Return an array of [low angle width, high angle width]."""
        half_widths = self.getProperty(Prop.FOREGROUND_HALF_WIDTH).value
        if len(half_widths) == 0:
            half_widths = [0, 0]
        elif len(half_widths) == 1:
            half_widths = [half_widths[0], half_widths[0]]
        return half_widths

    def _gravity_correction(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Performs simple gravity correction by replacing the wavelength X-axis and providing the corrected
        grazing angle, which is applied to data at a later stage. The algorithm below is reimplemented from COSMOS.

        Keyword arguments:
        ws -- reflected or direct beam workspace to be corrected for gravity
        """
        mm2m = 1e-3  # millimetre to metre conversion factor
        us2s = 1e-6  # microsecond to second conversion factor
        deg2rad = np.pi / 180.0
        rad2deg = 1.0 / deg2rad
        g = physical_constants["standard acceleration of gravity"][0]  # 9.80665 m / s^2
        h = physical_constants["Planck constant"][0]  # in J * s
        m_n = physical_constants["neutron mass"][0]  # in kg
        planck_per_kg = h / m_n  # in m^2 / s

        t_size = len(ws.readX(0))
        x_axis = np.linspace(0.5, t_size - 1, t_size, endpoint=False)

        run = ws.getRun()
        channel_width = run.getLogData("FrameOverlap.channel_width").value * us2s
        poff = run.getLogData("MainParameters.pickup_offset").value
        open_off = run.getLogData("MainParameters.open_offset").value
        chop_window = run.getLogData("ChopperWindow").value
        master_phase = run.getLogData("MainParameters.Master_chopper_phase").value
        slave_phase = run.getLogData("MainParameters.Slave_chopper_phase").value
        open_angle = chop_window - (slave_phase - master_phase) - open_off
        delay_angle = master_phase + 0.5 * (poff - open_angle)
        master_speed = run.getLogData("MainParameters.Master_chopper_speed").value
        slave_speed = run.getLogData("MainParameters.Slave_chopper_speed").value
        period = 60.0 / (0.5 * (master_speed + slave_speed))  # rpm
        e_delay = run.getLogData("PSD.time_of_flight_2").value * us2s
        delay = e_delay - (delay_angle / 360.0) * period
        ref_total_tofd = ws.spectrumInfo().l1() + ws.spectrumInfo().l2(0)  # total time of flight distance
        wavelength = 1e10 * (planck_per_kg * (x_axis * channel_width + delay) / ref_total_tofd)
        x_chopper = ws.spectrumInfo().l1()  # distance mid-chopper to sample
        offset = run.getLogData("Distance.sample_changer_horizontal_offset").value * mm2m
        x_slits2 = run.getLogData("Distance.S2_sample").value * mm2m + offset
        x_slits3 = run.getLogData("Distance.S3_sample").value * mm2m + offset
        cr = ws.getInstrument().getNumberParameter("chopper_radius")[0]  # m, Nexus is improperly filled

        if self._theta_zero is not None:
            theta0 = self._theta_zero * deg2rad / 2.0
        else:
            try:
                theta0 = ws.getRun().getLogData("VirtualAxis.SAN_actual_angle").value * deg2rad
            except RuntimeError:
                theta0 = ws.getRun().getLogData("VirtualAxis.DAN_actual_angle").value * deg2rad

        y_slits3 = x_slits3 * np.tan(theta0)  # x1 * tan(theta) in the paper
        y_slits2 = x_slits2 * np.tan(theta0)

        # origin is the centre of the sample
        v = planck_per_kg * 1e10 / wavelength  # neutron velocity in m/s, 1e10 for A to m conversion
        k = g / (2.0 * v**2)  # a characteristic inverse length

        # define parabola y = y0 - k * (x-x0) ^ 2 passing by (xslits3, yslits3) and (xslits2, yslits2)
        x0 = (k * ((x_slits2**2) - (x_slits3**2)) + y_slits2 - y_slits3) / (2 * k * (x_slits2 - x_slits3))
        y0 = y_slits3 + k * ((x_slits3 - x0) ** 2)

        delta = x0 - np.sqrt(y0 / k)  # shift in x along sample due to gravity

        grad = (2.0 * k) * (x0 - delta)  # gradient of parabola (to find true theta at y = 0)
        new_theta = np.arctan(grad)
        name = ws.name()
        shift = 2 if name[0] == "_" else 0
        new_twoTheta_ws = "{}_new_twoTheta".format(name[shift : shift + name[shift:].find("_")])

        chop_z = x_chopper * np.tan(theta0) - (y0 - k * (x_chopper - x0) ** 2)
        poffoff = (chop_z / cr) * rad2deg

        new_delay_angle = delay_angle - poffoff / 2.0
        new_delay = delay + ((delay_angle - new_delay_angle) / 360.0) * period
        new_lambda = 1e10 * (planck_per_kg * (x_axis * channel_width + new_delay) / ref_total_tofd)
        if self.getPropertyValue("Measurement") == "ReflectedBeam":
            correction = np.abs(new_theta / theta0)
            # the y axis needs to be 1 value shorter than the X axis so the workspace is a histogram,
            # which is needed for rebinning at a later stage
            correction = 0.5 * (correction[1:] + correction[:-1])
            CreateWorkspace(DataX=new_lambda, DataY=correction, OutputWorkspace=new_twoTheta_ws, UnitX="Wavelength", ParentWorkspace=ws)
        for spec_no in range(ws.getNumberHistograms()):
            ws.setX(spec_no, new_lambda)
        return ws

    def _input_ws(self) -> MatrixWorkspace:
        """Return a raw input workspace."""
        input_files = self.getPropertyValue(Prop.RUN)
        input_files = input_files.replace(",", "+")
        merged_ws_name = self._names.withSuffix("merged")
        measurement_type = self.getPropertyValue("Measurement")
        load_options = {
            "Measurement": measurement_type,
            "XUnit": "TimeOfFlight",
            "FitStartWorkspaceIndex": self.getProperty(Prop.START_WS_INDEX).value,
            "FitEndWorkspaceIndex": self.getProperty(Prop.END_WS_INDEX).value,
            "FitRangeLower": self.getProperty(Prop.XMIN).value,
            "FitRangeUpper": self.getProperty(Prop.XMAX).value,
        }
        if measurement_type == "ReflectedBeam":
            angle_option = self.getPropertyValue("AngleOption")
            first_run = self.getProperty(Prop.RUN).value[0]
            if angle_option == "SampleAngle":
                self._bragg_angle = common.sample_angle(first_run)
            elif angle_option == "DetectorAngle":
                self._bragg_angle = 0.0
                # in this case, we still need to correct for the difference of foreground
                # centres between direct and reflected beams, and this can be done most clearly
                # when the centre of the reflected beam is set at the beam axis
            elif angle_option == "UserAngle":
                self._bragg_angle = self.getProperty("BraggAngle").value
            load_options["BraggAngle"] = self._bragg_angle

        # The loop below handles translation of PropertyManagerProperty to dictionary of str: value (str, int, float...)
        # so it can be interfaced with PropertyManagerProperty via LoadOptions of LoadAndMerge.
        # Direct forwarding was not possible due to internal casting of this property to TypedValue
        logs_to_replace = dict()
        tmp_dict = dict(self.getProperty("LogsToReplace").value)
        for key in tmp_dict:
            logs_to_replace[key] = tmp_dict[key].value
        load_options["LogsToReplace"] = logs_to_replace

        # perform data consistency check
        if len(input_files.split("+")) > 1:
            self._check_angles_match(input_files)

        # MergeRunsOptions are defined by the parameter files and will not be modified here!
        ws = LoadAndMerge(
            Filename=input_files,
            LoaderName="LoadILLReflectometry",
            LoaderOptions=load_options,
            OutputWorkspace=merged_ws_name,
            EnableLogging=self._subalgLogging,
        )
        return ws

    def _normalise_to_flux(self, det_ws: MatrixWorkspace, mon_ws: MatrixWorkspace) -> MatrixWorkspace:
        """Normalise ws to monitor counts or counting time.

        Keyword arguments:
        det_ws -- workspace containing detector spectra
        mon_ws -- workspace containing monitor spectra
        """
        method = self.getProperty(Prop.FLUX_NORM_METHOD).value
        if method == FluxNormMethod.MONITOR:
            if mon_ws is None:
                raise RuntimeError("Cannot normalise to monitor data: no monitors in input data.")
            normalised_ws_name = self._names.withSuffix("normalised_to_monitor")
            mon_index = normalisation_monitor_workspace_index(mon_ws)
            mon_xs = mon_ws.readX(0)
            min_x = mon_xs[0]
            max_x = mon_xs[-1]
            normalised_ws = NormaliseToMonitor(
                InputWorkspace=det_ws,
                OutputWorkspace=normalised_ws_name,
                MonitorWorkspace=mon_ws,
                MonitorWorkspaceIndex=mon_index,
                IntegrationRangeMin=min_x,
                IntegrationRangeMax=max_x,
                EnableLogging=self._subalgLogging,
            )
            self._cleanup.cleanup(det_ws)
            return normalised_ws
        elif method == FluxNormMethod.TIME:
            t = det_ws.run().getProperty("duration").value
            normalised_ws_name = self._names.withSuffix("normalised_to_time")
            scaled_ws = Scale(InputWorkspace=det_ws, OutputWorkspace=normalised_ws_name, Factor=1.0 / t, EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(det_ws)
            return scaled_ws
        return det_ws

    def _normalise_to_slits(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Normalize workspace to slit opening and update slit widths.

        Keyword arguments:
        ws -- workspace to be normalized
        """
        # Update slit width in any case for later re-use.
        common.slit_sizes(ws)
        slit_norm = self.getProperty(Prop.SLIT_NORM).value
        if slit_norm == SlitNorm.OFF:
            return ws
        elif slit_norm == SlitNorm.AUTO and self._instrument_name != "D17":
            return ws
        run = ws.run()
        slit2_width = run.get(common.SampleLogs.SLIT2WIDTH).value
        slit3_width = run.get(common.SampleLogs.SLIT3WIDTH).value
        if slit2_width == "-" or slit3_width == "-":
            self.log().warning("Slit information not found in sample logs. Slit normalisation disabled.")
            return ws
        f = slit2_width * slit3_width
        normalised_ws_name = self._names.withSuffix("normalised_to_slits")
        normalised_ws = Scale(InputWorkspace=ws, OutputWorkspace=normalised_ws_name, Factor=1.0 / f, EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return normalised_ws

    def _refit_foreground(self, ws: MatrixWorkspace) -> None:
        """Refits the foreground position of the reflected beam measurement, and corrects position of the entire
         detector to correspond to the proper place.

        Keyword arguments:
        ws -- merged workspace that requires foreground refitting
        """
        # first, we need to  integrate the workspace indices
        int_ws = "{}_integrated".format(ws)
        kwargs = {}
        start_index = 0
        if not self.getProperty(Prop.START_WS_INDEX).isDefault:
            kwargs["StartWorkspaceIndex"] = self.getProperty(Prop.START_WS_INDEX).value
            start_index = self.getProperty(Prop.START_WS_INDEX).value
        if not self.getProperty(Prop.END_WS_INDEX).isDefault:
            kwargs["EndWorkspaceIndex"] = self.getProperty(Prop.END_WS_INDEX).value
        if not self.getProperty(Prop.XMAX).isDefault:
            kwargs["RangeUpper"] = self.getProperty(Prop.XMAX).value
        if not self.getProperty(Prop.XMIN).isDefault:
            kwargs["RangeLower"] = self.getProperty(Prop.XMIN).value
        Integration(InputWorkspace=ws, OutputWorkspace=int_ws, **kwargs)
        Transpose(InputWorkspace=int_ws, OutputWorkspace=int_ws)  # the integration output is a bin plot, cannot be fitted
        original_peak_centre = ws.getRun().getLogData("reduction.line_position").value
        # determine the initial fitting parameters
        y_axis = mtd[int_ws].readY(0)
        max_height = np.max(y_axis)
        max_index = start_index + (np.where(y_axis == max_height))[0][0]
        sigma = max_index - (np.where(y_axis > 0.667 * max_height))[0][0]
        sigma = sigma if sigma > 0 else 2
        fit_fun = "name=FlatBackground, A0={};".format(0.3 * max_height) + "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2}".format(
            max_index, 0.7 * max_height, sigma
        )
        try:
            fit_name = "fit_output"
            fit_output = Fit(
                Function=fit_fun,
                InputWorkspace=int_ws,
                StartX=float(max_index - 3 * sigma),
                EndX=float(max_index + 3 * sigma),
                CreateOutput=True,
                IgnoreInvalidData=True,
                Output="{}_{}".format(int_ws, fit_name),
            )
            # this table contains 5 rows, with fitted parameters of the magnitude flat background, then height, peakCentre,
            # and Sigma of the Gaussian, and finally the cost function
            param_table = fit_output.OutputParameters
            # grabbing the fitted peak centre, already as a function of spectrum number
            peak_centre = param_table.row(2)["Value"]
            # finally the instrument should be rotated to the correct position

            self._cleanup.cleanup("{}_{}_Parameters".format(int_ws, fit_name))
            self._cleanup.cleanup("{}_{}_Workspace".format(int_ws, fit_name))
            self._cleanup.cleanup("{}_{}_NormalisedCovarianceMatrix".format(int_ws, fit_name))
        except RuntimeError:
            self.log().error("Refitting of the foreground position of merge data failed. Using the maximum value instead.")
            peak_centre = max_index
        ws.getRun().addProperty("reduction.line_position", float(peak_centre), True)
        self._rotate_instrument(ws, peak_centre, original_peak_centre)
        self._cleanup.cleanup(int_ws)

    def _rotate_instrument(self, ws: MatrixWorkspace, peak_centre: float, original_centre: float) -> None:
        """Rotates the instrument of the provided workspace by the given angle around the sample, and then rotates
        the instrument around its own axis to maintain perpendicular orientation versus the incoming beam.

        Keyword arguments:
        ws -- workspace of which the instrument is going to be rotated
        peak_centre -- fitted peak centre in fractional tube number
        original_centre -- peak centre before the refitting
        """
        distance_entry = "det.value" if self._instrument_name == "D17" else "Distance.Sample_CenterOfDetector_distance"
        distance = 1e-3 * ws.getRun().getLogData(distance_entry).value  # in m

        # the calculation below is reimplementation of the code in the LoadILLReflectometry that performs the same operation
        sign = -1.0 if self._instrument_name == "D17" else 1.0
        pixel_width = common.pixel_size(self._instrument_name)
        offset_width = (127.5 - peak_centre) * pixel_width
        offset_angle = sign * atan(offset_width / distance)
        angle = offset_angle
        if self._bragg_angle is not None:  # case for a reflected beam
            angle += 2 * self._bragg_angle * np.pi / 180.0

        if self._instrument_name == "D17":
            x = distance * np.sin(angle)
            y = 0
            z = distance * np.cos(angle)
        else:
            x = 0
            y = distance * np.sin(angle)
            z = distance * np.cos(angle)
        MoveInstrumentComponent(Workspace=ws, ComponentName="detector", x=x, y=y, z=z, RelativePosition=False)

        # after the instrument is moved to its correct new position, it also has to be rotated to face the beam
        original_angle = sign * atan((127.5 - original_centre) * pixel_width / distance) * 180.0 / np.pi
        if self._bragg_angle is not None:  # case for a reflected beam
            original_angle += 2 * self._bragg_angle
        delta_angle = original_angle - angle * 180.0 / np.pi  # in degrees
        x_orientation = 0 if self._instrument_name == "D17" else -1
        y_orientation = 1 if self._instrument_name == "D17" else 0
        RotateInstrumentComponent(
            Workspace=ws, ComponentName="detector", X=x_orientation, Y=y_orientation, Z=0, Angle=delta_angle, RelativeRotation=True
        )

    def _subtract_flat_bkg(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Return a workspace where a flat background has been subtracted from ws.

        Keyword arguments:
        ws -- workspace from which the flat background will be subtracted
        """
        method = self.getProperty(Prop.BKG_METHOD).value
        if method == BkgMethod.OFF:
            return ws
        cloned_ws_name = self._names.withSuffix("cloned_for_flat_bkg")
        cloned_ws = CloneWorkspace(InputWorkspace=ws, OutputWorkspace=cloned_ws_name, EnableLogging=self._subalgLogging)
        transposed_ws_name = self._names.withSuffix("transposed_clone")
        transposed_ws = Transpose(InputWorkspace=cloned_ws, OutputWorkspace=transposed_ws_name, EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(cloned_ws)
        ranges = self._flat_bkg_ranges(ws)
        self.log().information("Calculating background in the range " + str(ranges))
        transposed_bkg_ws_name = self._names.withSuffix("transposed_flat_background")
        if method == BkgMethod.CONSTANT or method == BkgMethod.LINEAR:
            # fit with polynomial
            polynomial_degree = 0 if method == BkgMethod.CONSTANT else 1
            transposed_bkg_ws = CalculatePolynomialBackground(
                InputWorkspace=transposed_ws,
                OutputWorkspace=transposed_bkg_ws_name,
                Degree=polynomial_degree,
                XRanges=ranges,
                CostFunction="Unweighted least squares",
                EnableLogging=self._subalgLogging,
            )
        elif method == BkgMethod.AVERAGE:
            transposed_bkg_ws = self._calculate_average_background(transposed_ws, transposed_bkg_ws_name, ranges)
        self._cleanup.cleanup(transposed_ws)
        bkg_ws_name = self._names.withSuffix("flat_background")
        bkg_ws = Transpose(InputWorkspace=transposed_bkg_ws, OutputWorkspace=bkg_ws_name, EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(transposed_bkg_ws)
        subtracted_ws_name = self._names.withSuffix("flat_background_subtracted")
        subtracted_ws = Minus(LHSWorkspace=ws, RHSWorkspace=bkg_ws, OutputWorkspace=subtracted_ws_name, EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        self._cleanup.cleanup(bkg_ws)
        return subtracted_ws

    def _theta_from_detector_angles(self) -> float:
        """Returns the bragg angle as half of detector angle difference"""
        first_run = self.getProperty(Prop.RUN).value[0]
        db_detector_angle = self.getProperty("DirectBeamDetectorAngle").value
        return (common.detector_angle(first_run) - db_detector_angle) / 2.0

    def _water_calibration(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Divide ws by a (water) reference workspace.

        Keyword arguments:
        ws -- workspace to be calibrated by water reference
        """
        if self.getProperty(Prop.WATER_REFERENCE).isDefault:
            return ws
        water_ws = self.getProperty(Prop.WATER_REFERENCE).value
        det_ws_name = self._names.withSuffix("water_detectors")
        water_ws = ExtractMonitors(InputWorkspace=water_ws, DetectorWorkspace=det_ws_name, EnableLogging=self._subalgLogging)
        if mtd.doesExist(det_ws_name) is None:
            raise RuntimeError("No detectors in the water reference data.")
        if water_ws.getNumberHistograms() != ws.getNumberHistograms():
            self.log().error("Water workspace and run do not have the same number of histograms.")
        rebinned_water_ws_name = self._names.withSuffix("water_rebinned")
        rebinned_water_ws = RebinToWorkspace(
            WorkspaceToRebin=water_ws, WorkspaceToMatch=ws, OutputWorkspace=rebinned_water_ws_name, EnableLogging=self._subalgLogging
        )
        calibrated_ws_name = self._names.withSuffix("water_calibrated")
        calibrated_ws = Divide(
            LHSWorkspace=ws, RHSWorkspace=rebinned_water_ws, OutputWorkspace=calibrated_ws_name, EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(water_ws)
        self._cleanup.cleanup(rebinned_water_ws)
        self._cleanup.cleanup(ws)
        return calibrated_ws


AlgorithmFactory.subscribe(ReflectometryILLPreprocess)
