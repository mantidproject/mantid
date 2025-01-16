# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    MultipleFileProperty,
    Progress,
    WorkspaceGroup,
    WorkspaceGroupProperty,
)
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatArrayBoundedValidator,
    FloatArrayProperty,
    IntArrayBoundedValidator,
    IntArrayProperty,
    PropertyCriterion,
    StringArrayLengthValidator,
    StringArrayProperty,
    StringListValidator,
)
import ILL_utilities as utils
import ReflectometryILL_common as common
from ReflectometryILLPreprocess import BkgMethod, Prop, SubalgLogging
from mantid.simpleapi import (
    config,
    CropWorkspace,
    GroupWorkspaces,
    mtd,
    RebinToWorkspace,
    ReflectometryILLConvertToQ,
    ReflectometryILLPolarizationCor,
    ReflectometryILLPreprocess,
    ReflectometryILLSumForeground,
    RenameWorkspace,
    Scale,
    Stitch,
)
import math
from typing import List, Tuple, Union
import os


class PropertyNames(object):
    RB = "Run"
    DB = "DirectRun"
    WAVELENGTH_LOWER = "WavelengthLowerBound"
    WAVELENGTH_UPPER = "WavelengthUpperBound"
    GROUPING_FRACTION = "DeltaQFractionBinning"
    ANGLE_OPTION = "AngleOption"
    BKG_METHOD_DIRECT = "DirectFlatBackground"
    BKG_METHOD = "ReflFlatBackground"
    THETA = "Theta"
    SCALE_FACTOR = "GlobalScaleFactor"
    SUM_TYPE = "SummationType"

    HIGH_FRG_HALF_WIDTH = "ReflHighAngleFrgHalfWidth"
    HIGH_FRG_HALF_WIDTH_DIRECT = "DirectHighAngleFrgHalfWidth"
    HIGH_BKG_OFFSET = "ReflHighAngleBkgOffset"
    HIGH_BKG_OFFSET_DIRECT = "DirectHighAngleBkgOffset"
    HIGH_BKG_WIDTH = "ReflHighAngleBkgWidth"
    HIGH_BKG_WIDTH_DIRECT = "DirectHighAngleBkgWidth"

    LOW_FRG_HALF_WIDTH = "ReflLowAngleFrgHalfWidth"
    LOW_FRG_HALF_WIDTH_DIRECT = "DirectLowAngleFrgHalfWidth"
    LOW_BKG_OFFSET = "ReflLowAngleBkgOffset"
    LOW_BKG_OFFSET_DIRECT = "DirectLowAngleBkgOffset"
    LOW_BKG_WIDTH = "ReflLowAngleBkgWidth"
    LOW_BKG_WIDTH_DIRECT = "DirectLowAngleBkgWidth"

    START_WS_INDEX = "ReflFitStartWorkspaceIndex"
    END_WS_INDEX = "ReflFitEndWorkspaceIndex"
    START_WS_INDEX_DIRECT = "DirectFitStartWorkspaceIndex"
    END_WS_INDEX_DIRECT = "DirectFitEndWorkspaceIndex"
    XMAX = "ReflFitWavelengthUpperBound"
    XMAX_DIRECT = "DirectFitWavelengthUpperBound"
    XMIN = "ReflFitWavelengthLowerBound"
    XMIN_DIRECT = "DirectFitWavelengthLowerBound"

    POLARIZATION_OPTION = "PolarizationOption"
    RB00 = "Run00"
    RB01 = "Run01"
    RB10 = "Run10"
    RB11 = "Run11"
    EFFICIENCY_FILE = "PolarizationEfficiencyFile"

    # all these array properties must have either single value, or
    # as many, as there are reflected beams (i.e. angle configurations)
    PROPERTIES_TO_SIZE_MATCH = [
        DB,
        ANGLE_OPTION,
        THETA,
        SUM_TYPE,
        GROUPING_FRACTION,
        HIGH_FRG_HALF_WIDTH,
        HIGH_FRG_HALF_WIDTH_DIRECT,
        HIGH_BKG_OFFSET,
        HIGH_BKG_OFFSET_DIRECT,
        HIGH_BKG_WIDTH,
        HIGH_BKG_WIDTH_DIRECT,
        LOW_FRG_HALF_WIDTH,
        LOW_FRG_HALF_WIDTH_DIRECT,
        LOW_BKG_OFFSET,
        LOW_BKG_OFFSET_DIRECT,
        LOW_BKG_WIDTH,
        LOW_BKG_WIDTH_DIRECT,
        START_WS_INDEX,
        END_WS_INDEX,
        START_WS_INDEX_DIRECT,
        END_WS_INDEX_DIRECT,
        WAVELENGTH_LOWER,
        WAVELENGTH_UPPER,
    ]

    DAN = "DetectorAngle"
    SAN = "SampleAngle"
    UAN = "UserAngle"
    INCOHERENT = "Incoherent"
    COHERENT = "Coherent"

    MANUAL_SCALE_FACTORS = "ManualScaleFactors"
    CACHE_DIRECT_BEAM = "CacheDirectBeam"


class ReflectometryILLAutoProcess(DataProcessorAlgorithm):
    @staticmethod
    def _compose_run_string(run: Union[list, str]) -> str:
        """Returns the string that will be passed to load the files. For multiple run numbers, they will be summed

        Keyword arguments:
        run -- string containing run numbers to be loaded
        """
        if isinstance(run, list):
            return "+".join(run)
        else:
            return run

    @staticmethod
    def _detector_angle_from_logs(ws: str) -> float:
        """Returns the detector angle from sample logs.

        Keyword arguments:
        ws -- workspace name used as source of metadata
        """
        run = mtd[ws].run() if not isinstance(mtd[ws], WorkspaceGroup) else mtd[ws][0].run()
        if run.hasProperty("DAN.value"):
            return run.getLogData("DAN.value").value
        elif run.hasProperty("dan.value"):
            return run.getLogData("dan.value").value
        elif run.hasProperty("VirtualAxis.DAN_actual_angle"):
            return run.getLogData("VirtualAxis.DAN_actual_angle").value
        else:
            raise RuntimeError("Unable to retrieve the detector angle from " + ws)

    @staticmethod
    def _foreground_centre_from_logs(ws: str) -> float:
        """Returns the foreground centre from sample logs.

        Keyword arguments:
        ws -- workspace name used as source of metadata
        """
        run = mtd[ws].run() if not isinstance(mtd[ws], WorkspaceGroup) else mtd[ws][0].run()
        if run.hasProperty(common.SampleLogs.LINE_POSITION):
            return run.getLogData(common.SampleLogs.LINE_POSITION).value
        else:
            raise RuntimeError("Unable to retrieve the direct beam foreground centre needed for DAN option.")

    @staticmethod
    def _get_numor_string(runs: Union[list, str]) -> str:
        """Returns numor string based on the provided runs list."""
        if not isinstance(runs, list):
            runs = [runs]
        if isinstance(runs[0], list):
            runs = runs[0]
        return "+".join([numor[numor.rfind("/") + 1 : -4] for numor in runs])

    def category(self):
        """Return the categories of the algorithm."""
        return "ILL\\Reflectometry;ILL\\Auto;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryILLAutoProcess"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Performs reduction of ILL reflectometry data, instruments D17 and FIGARO."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return [
            "ReflectometryILLConvertToQ",
            "ReflectometryILLPolarizationCor",
            "ReflectometryILLPreprocess",
            "ReflectometryILLSumForeground",
        ]

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def validateInputs(self):
        """Returns a dictionary containing issues found in properties."""
        issues = dict()
        rb = self.getProperty(PropertyNames.RB).value
        if not rb:
            rb = self.getProperty(PropertyNames.RB00).value
        dimensionality = len(rb)
        if dimensionality != 0:
            for property_name in PropertyNames.PROPERTIES_TO_SIZE_MATCH:
                value = self.getProperty(property_name).value
                if len(value) != dimensionality and len(value) != 1:
                    issues[property_name] = (
                        "Parameter size mismatch: must have a single value or as many as there are reflected beams:"
                        " given {}, but there are {} reflected beam runs".format(len(value), dimensionality)
                    )
        if not self.getProperty(PropertyNames.MANUAL_SCALE_FACTORS).isDefault:
            manual_scale_factors = self.getProperty(PropertyNames.MANUAL_SCALE_FACTORS).value
            if len(manual_scale_factors) != dimensionality - 1:
                issues[PropertyNames.MANUAL_SCALE_FACTORS] = (
                    "Provide N-1 manual scale factors, where N is the number of different angle configurations"
                )
        angle_options = self.getProperty(PropertyNames.ANGLE_OPTION).value
        for angle_option in angle_options:
            if angle_option not in [PropertyNames.DAN, PropertyNames.SAN, PropertyNames.UAN]:
                issues[PropertyNames.ANGLE_OPTION] = "Invalid angle option given: " + angle_option
                break
        sum_types = self.getProperty(PropertyNames.SUM_TYPE).value
        for sum_type in sum_types:
            if sum_type not in [PropertyNames.INCOHERENT, PropertyNames.COHERENT]:
                issues[PropertyNames.SUM_TYPE] = "Invalid summation option given: " + sum_type
                break
        if self.getPropertyValue(PropertyNames.POLARIZATION_OPTION) == "NotPolarized" and not self.getPropertyValue(PropertyNames.RB):
            issues[PropertyNames.RB] = "Reflected beam input runs are mandatory"
        if self.getPropertyValue(PropertyNames.POLARIZATION_OPTION) == "Polarized":
            run00 = self.getPropertyValue(PropertyNames.RB00)
            run11 = self.getPropertyValue(PropertyNames.RB11)
            if not run00:
                issues[PropertyNames.RB00] = "Reflected beam runs are mandatory for 00 (or 0)."
            if not run11:
                issues[PropertyNames.RB11] = "Reflected beam runs are mandatory for 11 (or 1)."
        return issues

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        non_negative_ints = IntArrayBoundedValidator()
        non_negative_ints.setLower(0)
        non_negative_float_array = FloatArrayBoundedValidator()
        non_negative_float_array.setLower(0.0)
        string_array_validator = StringArrayLengthValidator()
        string_array_validator.setLengthMin(1)

        # ======================== Main Properties ========================
        self.declareProperty(
            PropertyNames.POLARIZATION_OPTION,
            "NonPolarized",
            StringListValidator(["NonPolarized", "Polarized"]),
            "Indicate whether measurements are polarized",
        )

        is_polarized = EnabledWhenProperty(PropertyNames.POLARIZATION_OPTION, PropertyCriterion.IsEqualTo, "Polarized")
        is_not_polarized = EnabledWhenProperty(PropertyNames.POLARIZATION_OPTION, PropertyCriterion.IsEqualTo, "NonPolarized")
        polarized = "Inputs for polarized measurements"

        self.declareProperty(
            MultipleFileProperty(PropertyNames.RB, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="A list of reflected run numbers/files.",
        )
        self.setPropertySettings(PropertyNames.RB, is_not_polarized)

        self.declareProperty(
            MultipleFileProperty(PropertyNames.RB00, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="A list of reflected run numbers/files for 00 (or 0).",
        )
        self.setPropertySettings(PropertyNames.RB00, is_polarized)
        self.setPropertyGroup(PropertyNames.RB00, polarized)

        self.declareProperty(
            MultipleFileProperty(PropertyNames.RB01, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="A list of reflected run numbers/files for 01.",
        )
        self.setPropertySettings(PropertyNames.RB01, is_polarized)
        self.setPropertyGroup(PropertyNames.RB01, polarized)

        self.declareProperty(
            MultipleFileProperty(PropertyNames.RB10, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="A list of reflected run numbers/files for 10.",
        )
        self.setPropertySettings(PropertyNames.RB10, is_polarized)
        self.setPropertyGroup(PropertyNames.RB10, polarized)

        self.declareProperty(
            MultipleFileProperty(PropertyNames.RB11, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="A list of reflected run numbers/files for 11 (or 1).",
        )
        self.setPropertySettings(PropertyNames.RB11, is_polarized)
        self.setPropertyGroup(PropertyNames.RB11, polarized)

        self.declareProperty(
            FileProperty(PropertyNames.EFFICIENCY_FILE, defaultValue="", action=FileAction.OptionalLoad),
            doc="A file containing the polarization efficiency factors.",
        )
        self.setPropertySettings(PropertyNames.EFFICIENCY_FILE, is_polarized)
        self.setPropertyGroup(PropertyNames.EFFICIENCY_FILE, polarized)

        self.declareProperty(
            MultipleFileProperty(PropertyNames.DB, action=FileAction.Load, extensions=["nxs"]), doc="A list of direct run numbers/files."
        )
        self.declareProperty(
            WorkspaceGroupProperty(Prop.OUTPUT_WS, defaultValue="", direction=Direction.Output), doc="The output workspace group."
        )
        self.declareProperty(
            PropertyNames.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.AVERAGE,
            validator=StringListValidator([BkgMethod.AVERAGE, BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
            doc="Flat background calculation method for background subtraction.",
        )
        self.declareProperty(
            PropertyNames.BKG_METHOD,
            defaultValue=BkgMethod.AVERAGE,
            validator=StringListValidator([BkgMethod.AVERAGE, BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
            doc="Flat background calculation method for background subtraction.",
        )
        self.copyProperties(
            "ReflectometryILLPreprocess", [Prop.SUBALG_LOGGING, Prop.CLEANUP, Prop.WATER_REFERENCE, Prop.SLIT_NORM, Prop.FLUX_NORM_METHOD]
        )
        self.declareProperty(PropertyNames.SCALE_FACTOR, defaultValue=1.0, doc="Scale factor.")

        self.declareProperty(
            FloatArrayProperty(PropertyNames.MANUAL_SCALE_FACTORS, values=[]),
            doc="An optional list of manual scale factors for stitching (number of angle configurations minus 1)",
        )

        self.declareProperty(
            PropertyNames.CACHE_DIRECT_BEAM,
            defaultValue=False,
            doc="Cache the processed direct beam in ADS for ready use with further reflected beams;"
            "saves important execution time, however assumes that the direct beam processing "
            "configuration must be invariant for different reflected beams.",
        )

        # ======================== Common Properties ========================
        common_prop = "Preprocessing common properties: provide a list or a single value"

        self.declareProperty(
            StringArrayProperty(
                PropertyNames.ANGLE_OPTION, values=[PropertyNames.DAN], validator=string_array_validator, direction=Direction.Input
            ),
            doc="Angle option used for detector positioning",
        )
        self.declareProperty(FloatArrayProperty(PropertyNames.THETA, values=[-1.0]), doc="A user-defined angle theta in degree")
        self.declareProperty(
            StringArrayProperty(
                PropertyNames.SUM_TYPE, values=[PropertyNames.INCOHERENT], validator=string_array_validator, direction=Direction.Input
            ),
            doc="Type of summation to perform",
        )
        self.declareProperty(
            FloatArrayProperty(PropertyNames.WAVELENGTH_LOWER, values=[2.0], validator=non_negative_float_array),
            doc="The lower wavelength bound (Angstrom)",
        )
        self.declareProperty(
            FloatArrayProperty(PropertyNames.WAVELENGTH_UPPER, values=[30.0], validator=non_negative_float_array),
            doc="The upper wavelength bound (Angstrom)",
        )
        self.declareProperty(
            FloatArrayProperty(PropertyNames.GROUPING_FRACTION, values=[0.5], validator=non_negative_float_array),
            doc="If set, group the output by steps of this fraction multiplied by Q resolution",
        )
        self.setPropertyGroup(PropertyNames.ANGLE_OPTION, common_prop)
        self.setPropertyGroup(PropertyNames.THETA, common_prop)
        self.setPropertyGroup(PropertyNames.SUM_TYPE, common_prop)
        self.setPropertyGroup(PropertyNames.WAVELENGTH_LOWER, common_prop)
        self.setPropertyGroup(PropertyNames.WAVELENGTH_UPPER, common_prop)
        self.setPropertyGroup(PropertyNames.GROUPING_FRACTION, common_prop)

        # ======================== Direct Run Properties ========================
        pre_process_direct = "Preprocessing for direct runs: provide a list or a single value"

        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT, values=[2], validator=non_negative_ints),
            doc="Number of foreground pixels at lower angles from the centre pixel.",
        )
        self.setPropertyGroup(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_BKG_OFFSET_DIRECT, values=[5], validator=non_negative_ints),
            doc="Distance of flat background region towards smaller detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_BKG_WIDTH_DIRECT, values=[5], validator=non_negative_ints),
            doc="Width of flat background region towards smaller detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT, values=[2], validator=non_negative_ints),
            doc="Number of foreground pixels at higher angles from the centre pixel.",
        )
        self.setPropertyGroup(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_BKG_OFFSET_DIRECT, values=[5], validator=non_negative_ints),
            doc="Distance of flat background region towards larger detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_BKG_WIDTH_DIRECT, values=[5], validator=non_negative_ints),
            doc="Width of flat background region towards larger detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.START_WS_INDEX_DIRECT, values=[0], validator=non_negative_ints),
            doc="Start histogram index used for peak fitting",
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX_DIRECT, pre_process_direct)
        self.declareProperty(
            IntArrayProperty(PropertyNames.END_WS_INDEX_DIRECT, values=[255], validator=non_negative_ints),
            doc="Last histogram index used for peak fitting",
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX_DIRECT, pre_process_direct)
        self.declareProperty(PropertyNames.XMIN_DIRECT, defaultValue=-1.0, doc="Minimum x value (unit wavelength) used for peak fitting.")
        self.setPropertyGroup(PropertyNames.XMIN_DIRECT, pre_process_direct)
        self.declareProperty(PropertyNames.XMAX_DIRECT, defaultValue=-1.0, doc="Maximum x value (unit wavelength) used for peak fitting.")
        self.setPropertyGroup(PropertyNames.XMAX_DIRECT, pre_process_direct)

        # ======================== Preprocessing For Reflected Runs ========================
        pre_process_reflected = "Preprocessing for reflected runs: provide a list or a single value"

        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_FRG_HALF_WIDTH, values=[2], validator=non_negative_ints),
            doc="Number of foreground pixels at lower angles from the centre pixel.",
        )
        self.setPropertyGroup(PropertyNames.LOW_FRG_HALF_WIDTH, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_BKG_OFFSET, values=[5], validator=non_negative_ints),
            doc="Distance of flat background region towards smaller detector angles from the foreground centre, in pixels.",
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.LOW_BKG_WIDTH, values=[5], validator=non_negative_ints),
            doc="Width of flat background region towards smaller detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_FRG_HALF_WIDTH, values=[2], validator=non_negative_ints),
            doc="Number of foreground pixels at higher angles from the centre pixel.",
        )
        self.setPropertyGroup(PropertyNames.HIGH_FRG_HALF_WIDTH, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_BKG_OFFSET, values=[5], validator=non_negative_ints),
            doc="Distance of flat background region towards larger detector angles from the foreground centre, in pixels",
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.HIGH_BKG_WIDTH, values=[5], validator=non_negative_ints),
            doc="Width of flat background region towards larger detector angles from the foreground centre, in pixels.",
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.START_WS_INDEX, values=[0], validator=non_negative_ints),
            doc="Start histogram index used for peak fitting",
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX, pre_process_reflected)
        self.declareProperty(
            IntArrayProperty(PropertyNames.END_WS_INDEX, values=[255], validator=non_negative_ints),
            doc="Last histogram index used for peak fitting",
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX, pre_process_reflected)
        self.declareProperty(
            FloatArrayProperty(PropertyNames.XMIN, values=[-1.0]), doc="Minimum x value (unit wavelength) used for peak fitting"
        )
        self.setPropertyGroup(PropertyNames.XMIN, pre_process_reflected)
        self.declareProperty(
            FloatArrayProperty(PropertyNames.XMAX, values=[-1.0]), doc="Maximum x value (unit wavelength) used for peak fitting"
        )
        self.setPropertyGroup(PropertyNames.XMAX, pre_process_reflected)

        self.declareProperty(name="SaveReductionParams", defaultValue=True, doc="Whether to save reduction parameters in an ASCII file.")

        self.declareProperty(name="CorrectGravity", defaultValue=False, doc="Whether to correct for gravity effects (FIGARO only).")

        self.copyProperties("ReflectometryILLPreprocess", ["LogsToReplace"])

    def PyExec(self):
        """Execute the algorithm."""
        self.log().purge()
        self.setup()
        to_group = []
        scale_factor = self.getProperty(PropertyNames.SCALE_FACTOR).value
        progress = Progress(self, start=0.0, end=1.0, nreports=self._dimensionality)
        log = "\nNumber of angles treated: {}\n".format(self._dimensionality)
        self._accumulate_log(log)
        for angle_index in range(self._dimensionality):
            if len(self._db) == 1:
                run_db = self.make_name(self._db[0])
                direct_beam_names = self._get_numor_string(self._db)
            else:
                run_db = self.make_name(self._db[angle_index])
                direct_beam_names = self._get_numor_string(self._db[angle_index])
            log = "Angle {}:\n".format(angle_index + 1)
            self._accumulate_log(log)
            log = "Direct Beam(s): {}\n".format(direct_beam_names)
            self._accumulate_log(log)
            direct_beam_name = "{}_direct".format(run_db)
            direct_foreground_name = "{}_frg".format(direct_beam_name)
            # always process direct beam; even if it can be the same for different angles,
            # the foreground and background regions might be different
            self.process_direct_beam(direct_beam_name, direct_foreground_name, angle_index)
            corrected_theta_ws = ""
            if not self.is_polarized():
                runRB = self.make_name(self._rb[angle_index])
                refl_beam_names = self._get_numor_string(self._rb[angle_index])
                log = "Reflected Beam(s): {}\n".format(refl_beam_names)
                self._accumulate_log(log)
                reflected_beam_input = self._compose_run_string(self._rb[angle_index])
                reflected_beam_name = "{}_reflected".format(runRB)
                to_convert_to_q, direct_foreground_name, corrected_theta_ws = self.process_reflected_beam(
                    reflected_beam_input, reflected_beam_name, direct_beam_name, angle_index
                )
            else:
                foreground_names = []
                run_inputs, run_names = self.compose_polarized_runs_list(angle_index)
                for run, name in zip(run_inputs, run_names):
                    reflected_pol_foreground_ws_name, direct_foreground_name, corrected_theta_ws = self.process_reflected_beam(
                        run, name, direct_beam_name, angle_index
                    )
                    foreground_names.append(reflected_pol_foreground_ws_name)
                to_convert_to_q = "{}_pol_{}".format(self._out_ws, str(angle_index))
                self.polarization_correction(",".join(foreground_names), to_convert_to_q)
                for workspace in mtd[to_convert_to_q]:
                    self._auto_cleanup.cleanupLater(workspace.name())

            converted_to_q_name = "{}_{}".format(self._out_ws, str(angle_index))
            self.convert_to_momentum_transfer(to_convert_to_q, converted_to_q_name, direct_foreground_name, angle_index, corrected_theta_ws)
            if scale_factor != 1:
                Scale(InputWorkspace=converted_to_q_name, OutputWorkspace=converted_to_q_name, Factor=scale_factor)
            to_group.append(converted_to_q_name)
            if self.getProperty("SaveReductionParams").value:
                self.save_parameters(converted_to_q_name)
            self._auto_cleanup.protect(converted_to_q_name)
            progress.report()

        if len(to_group) > 1:
            try:
                stitched = "{}_stitched".format(self._out_ws)
                scale_factors = self.getProperty(PropertyNames.MANUAL_SCALE_FACTORS).value
                Stitch(InputWorkspaces=to_group, OutputWorkspace=stitched, ManualScaleFactors=scale_factors)
                to_group.append(stitched)
            except RuntimeError as re:
                self.log().warning("Unable to stitch automatically, consider stitching manually: {}".format(str(re)))

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=self._out_ws)
        self.setProperty(Prop.OUTPUT_WS, self._out_ws)
        self._auto_cleanup.finalCleanup()
        self.log().flushNotice()

    def _accumulate_log(self, log: str) -> None:
        """Wrapper function for simultaneously calling log accumulation and appending for the output configuration file.

        Keyword arguments:
        log -- reduction progress information to be accumulated
        """
        self.log().accumulate(log)
        self._logs.append(log)

    def compose_polarized_runs_list(self, angle_index: int) -> Tuple[List[str], List[str]]:
        """Returns two lists: one of runs, the other of names for different flipper configurations at the given angle_index

        Keyword arguments:
        angle_index -- index of the currently processed polarized runs
        """
        run_inputs = []
        run_names = []
        if self._rb00:
            run00_name = "{}_00".format(self.make_name(self._rb00[angle_index]))
            run00_input = self._compose_run_string(self._rb00[angle_index])
            run_names.append(run00_name)
            run_inputs.append(run00_input)
        if self._rb01:
            run01_name = "{}_01".format(self.make_name(self._rb01[angle_index]))
            run01_input = self._compose_run_string(self._rb01[angle_index])
            run_names.append(run01_name)
            run_inputs.append(run01_input)
        if self._rb10:
            run10_name = "{}_10".format(self.make_name(self._rb10[angle_index]))
            run10_input = self._compose_run_string(self._rb10[angle_index])
            run_names.append(run10_name)
            run_inputs.append(run10_input)
        if self._rb11:
            run11_name = "{}_11".format(self.make_name(self._rb11[angle_index]))
            run11_input = self._compose_run_string(self._rb11[angle_index])
            run_names.append(run11_name)
            run_inputs.append(run11_input)
        return run_inputs, run_names

    def convert_to_momentum_transfer(
        self, input_ws_name: str, output_ws_name: str, direct_foreground_name: str, angle_index: int, theta_correction=""
    ) -> None:
        """Run the ReflectometryILLConvertToQ on the input workspace.

        Keyword arguments:
        input_ws_name -- ame of the workspace to be converted to momentum exchange
        output_ws_name -- name for the output workspace
        direct_foreground_name -- name of the direct foreground workspace
        angle_index -- index of the currently processed reflected runs
        theta_correction -- name of the workspace containing twoTheta gravity corrections (FIGARO only, optional)
        """
        rebinned_direct_foreground = "{}_rebinned".format(direct_foreground_name)
        RebinToWorkspace(
            WorkspaceToRebin=direct_foreground_name, WorkspaceToMatch=input_ws_name, OutputWorkspace=rebinned_direct_foreground
        )

        ReflectometryILLConvertToQ(
            InputWorkspace=input_ws_name,
            OutputWorkspace=output_ws_name,
            DirectForegroundWorkspace=rebinned_direct_foreground,
            GroupingQFraction=float(self.get_value(PropertyNames.GROUPING_FRACTION, angle_index)),
            SubalgorithmLogging=self._subalg_logging,
            ThetaCorrection=theta_correction,
            Cleanup=self._cleanup,
        )
        self._auto_cleanup.cleanupLater(direct_foreground_name)
        self._auto_cleanup.cleanupLater(rebinned_direct_foreground)

    def crop_theta_correction(self, reflected_beam_name: str, theta_ws_name: str, angle_index: int) -> str:
        """Prepares the angular gravity correction workspace to have a consistent X axis range with the reflected beam
        workspace.

        Keyword arguments:
        reflected_beam_name -- name of the reflected beam workspace
        theta_ws_name -- name to be given to cropped workspace with twoTheta gravity correction
        angle_index -- index of the currently processed reflected runs
        """
        if theta_ws_name not in mtd:
            return ""
        theta_ws_name_cropped = "{}_cropped".format(theta_ws_name)
        wavelengthRange = [
            float(self.get_value(PropertyNames.WAVELENGTH_LOWER, angle_index)),
            float(self.get_value(PropertyNames.WAVELENGTH_UPPER, angle_index)),
        ]
        RebinToWorkspace(WorkspaceToRebin=theta_ws_name, WorkspaceToMatch=reflected_beam_name, OutputWorkspace=theta_ws_name_cropped)
        CropWorkspace(
            InputWorkspace=theta_ws_name_cropped, OutputWorkspace=theta_ws_name_cropped, XMin=wavelengthRange[0], XMax=wavelengthRange[1]
        )
        return theta_ws_name_cropped

    def get_value(self, property_name: str, angle_index: int) -> float:
        """Returns the value of the property at given angle index.

        Keyword arguments:
        property_name -- name of the property to fetch from logs
        angle_index -- index for the property to be fetched
        """
        value = self.getProperty(property_name).value
        if len(value) == 1:
            return value[0]
        else:
            return value[angle_index]

    def is_polarized(self) -> bool:
        """Returns True, if a polarization file is given and False otherwise."""
        return self.getProperty(PropertyNames.POLARIZATION_OPTION).value == "Polarized"

    def log_foreground_centres(self, reflected_beam_name: str, direct_beam_name: str) -> None:
        """Accumulates logs on the fractional foreground centres for direct and reflected beams.

        Keyword arguments:
        reflected_beam_name -- name of the reflected beam workspace
        direct_beam_name -- name of the direct beam workspace
        """
        db = mtd[direct_beam_name] if not isinstance(mtd[direct_beam_name], WorkspaceGroup) else mtd[direct_beam_name][0]
        db_frg_centre = db.run().getLogData(common.SampleLogs.LINE_POSITION).value
        rb_frg_centre = mtd[reflected_beam_name].run().getLogData(common.SampleLogs.LINE_POSITION).value
        log = "Direct beam foreground centre [pixel]: {:.5f}\n".format(db_frg_centre)
        self._accumulate_log(log)
        log = "Reflected beam foreground centre [pixel]: {:.5f}\n".format(rb_frg_centre)
        self._accumulate_log(log)

    def make_name(self, run_name: Union[list, str]) -> str:
        """Returns a name suitable to put in the ADS: the run number.

        Keyword arguments:
        run_name -- raw string (or list of string) containing full path to data
        """
        name = ""
        if isinstance(run_name, list):
            # for multiple files return the run number of the first run
            try:
                name = self.make_name(run_name[0])
            except IndexError:
                pass
        else:
            name = run_name[-10:-4]  # removes .nxs and takes final 6 characters containing run number
        return name

    def polarization_correction(self, input_ws_name: str, output_ws_name: str) -> None:
        """Run the ReflectometryILLPolarizationCor on the input workspace.

        Keyword arguments:
        input_ws_name -- ame of the workspace to be corrected for polarization
        output_ws_name -- name for the output workspace
        """
        ReflectometryILLPolarizationCor(
            InputWorkspaces=input_ws_name,
            OutputWorkspace=output_ws_name,
            EfficiencyFile=self.getProperty(PropertyNames.EFFICIENCY_FILE).value,
            SubalgorithmLogging=self._subalg_logging,
            Cleanup=self._cleanup,
        )

    def preprocess_direct_beam(self, run: str, out_ws: str, angle_index: int) -> None:
        """Runs preprocess algorithm on the direct beam runs.

        Keyword arguments:
        run -- string of runs to be preprocessed
        out_ws -- name for the output workspace
        angle_index -- index of the currently processed direct runs
        """
        ReflectometryILLPreprocess(
            Run=run,
            Measurement="DirectBeam",
            OutputWorkspace=out_ws,
            SlitNormalisation=self.getProperty(Prop.SLIT_NORM).value,
            FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
            SubalgorithmLogging=self._subalg_logging,
            Cleanup=self._cleanup,
            ForegroundHalfWidth=[
                int(self.get_value(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT, angle_index)),
                int(self.get_value(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT, angle_index)),
            ],
            LowAngleBkgOffset=int(self.get_value(PropertyNames.LOW_BKG_OFFSET_DIRECT, angle_index)),
            LowAngleBkgWidth=int(self.get_value(PropertyNames.LOW_BKG_WIDTH_DIRECT, angle_index)),
            HighAngleBkgOffset=int(self.get_value(PropertyNames.HIGH_BKG_OFFSET_DIRECT, angle_index)),
            HighAngleBkgWidth=int(self.get_value(PropertyNames.HIGH_BKG_WIDTH_DIRECT, angle_index)),
            FlatBackground=self.getPropertyValue(PropertyNames.BKG_METHOD_DIRECT),
            FitStartWorkspaceIndex=int(self.get_value(PropertyNames.START_WS_INDEX_DIRECT, angle_index)),
            FitEndWorkspaceIndex=int(self.get_value(PropertyNames.END_WS_INDEX_DIRECT, angle_index)),
            FitRangeLower=self.getProperty(PropertyNames.XMIN_DIRECT).value,
            FitRangeUpper=self.getProperty(PropertyNames.XMAX_DIRECT).value,
            CorrectGravity=self.getProperty("CorrectGravity").value,
            LogsToReplace=self.getProperty("LogsToReplace").value,
        )

    def preprocess_reflected_beam(self, run: str, output_ws_name: str, direct_beam_name: str, angle_index: int) -> None:
        """Runs preprocess algorithm on the reflected beam data.

        Keyword arguments:
        run -- string of runs to be preprocessed
        output_ws_name -- name for the output workspace
        direct_beam_name -- name of the direct beam workspace at the same angle index
        angle_index -- index of the currently processed reflected runs
        """
        angle_option = self.get_value(PropertyNames.ANGLE_OPTION, angle_index)
        preprocess_args = {
            "Run": run,
            "Measurement": "ReflectedBeam",
            "OutputWorkspace": output_ws_name,
            "AngleOption": angle_option,
            "SlitNormalisation": self.getProperty(Prop.SLIT_NORM).value,
            "FluxNormalisation": self.getProperty(Prop.FLUX_NORM_METHOD).value,
            "SubalgorithmLogging": self._subalg_logging,
            "Cleanup": self._cleanup,
            "FlatBackground": self.getPropertyValue(PropertyNames.BKG_METHOD),
            "ForegroundHalfWidth": [
                int(self.get_value(PropertyNames.LOW_FRG_HALF_WIDTH, angle_index)),
                int(self.get_value(PropertyNames.HIGH_FRG_HALF_WIDTH, angle_index)),
            ],
            "LowAngleBkgOffset": int(self.get_value(PropertyNames.LOW_BKG_OFFSET, angle_index)),
            "LowAngleBkgWidth": int(self.get_value(PropertyNames.LOW_BKG_WIDTH, angle_index)),
            "HighAngleBkgOffset": int(self.get_value(PropertyNames.HIGH_BKG_OFFSET, angle_index)),
            "HighAngleBkgWidth": int(self.get_value(PropertyNames.HIGH_BKG_WIDTH, angle_index)),
            "FitStartWorkspaceIndex": int(self.get_value(PropertyNames.START_WS_INDEX, angle_index)),
            "FitEndWorkspaceIndex": int(self.get_value(PropertyNames.END_WS_INDEX, angle_index)),
            "FitRangeLower": self.get_value(PropertyNames.XMIN, angle_index),
            "FitRangeUpper": self.get_value(PropertyNames.XMAX, angle_index),
            "CorrectGravity": self.getProperty("CorrectGravity").value,
            "LogsToReplace": self.getProperty("LogsToReplace").value,
        }
        if angle_option == PropertyNames.UAN:
            preprocess_args["BraggAngle"] = self.get_value(PropertyNames.THETA, angle_index)
        elif angle_option == PropertyNames.DAN:
            preprocess_args["DirectBeamDetectorAngle"] = self._detector_angle_from_logs(direct_beam_name)
            preprocess_args["DirectBeamForegroundCentre"] = self._foreground_centre_from_logs(direct_beam_name)
        ReflectometryILLPreprocess(**preprocess_args)
        log = "Angle method: {}\n".format(angle_option)
        self._accumulate_log(log)

    def process_direct_beam(self, direct_beam_name: str, direct_foreground_name: str, angle_index: int) -> None:
        """Processes the direct beam for the given angle configuration by calling preprocessing, summing foreground,
        correcting gravity (FIGARO only), and in case of polarized processing, polarization correction.

        Keyword arguments:
        direct_beam_name -- name to be given to the direct preprocessed workspace
        direct_foreground_name -- name to be given to the direct foreground workspace
        angle_index -- index of the currently processed direct runs
        """
        db_run = self._db[0] if len(self._db) == 1 else self._db[angle_index]
        direct_beam_input = self._compose_run_string(db_run)
        self.preprocess_direct_beam(direct_beam_input, direct_beam_name, angle_index)
        sum_type = self.get_value(PropertyNames.SUM_TYPE, angle_index)
        sum_type = common.SUM_IN_LAMBDA if sum_type == PropertyNames.INCOHERENT else common.SUM_IN_Q
        self.sum_foreground(direct_beam_name, direct_foreground_name, sum_type, angle_index)
        if self.getProperty(PropertyNames.CACHE_DIRECT_BEAM).value:
            self._auto_cleanup.protect(direct_beam_name)
            self._auto_cleanup.protect(direct_foreground_name)
        else:
            self._auto_cleanup.cleanupLater(direct_beam_name)
            self._auto_cleanup.cleanupLater(direct_foreground_name)
        if self.is_polarized():
            self.polarization_correction(direct_foreground_name, direct_foreground_name)
            # the output is a workspace group with one entry, which needs to be flatten to MatrixWorkspace:
            frg_ws_name = mtd[direct_foreground_name][0].name()
            RenameWorkspace(InputWorkspace=frg_ws_name, OutputWorkspace=direct_foreground_name)

    def process_reflected_beam(
        self, reflected_beam_input, reflected_beam_name: str, direct_beam_name: str, angle_index: int
    ) -> Tuple[str, str, str]:
        """Processes the reflected beam for the given angle configuration by calling preprocessing, summing foreground,
        correcting gravity (FIGARO only), and in case of polarized processing, polarization correction.

        Keyword arguments:
        reflected_beam_input -- name of the reflected beam run
        reflected_beam_name -- name to be given to the reflected preprocessed workspace
        direct_beam_name -- name of the direct beam workspace at the same angle index
        angle_index -- index of the currently processed reflected runs
        """
        self.preprocess_reflected_beam(reflected_beam_input, reflected_beam_name, direct_beam_name, angle_index)
        self.log_foreground_centres(reflected_beam_name, direct_beam_name)
        foreground_name = "{}_frg".format(reflected_beam_name)
        direct_foreground_name = "{}_frg".format(direct_beam_name)
        sum_type = self.get_value(PropertyNames.SUM_TYPE, angle_index)
        log = "Summation method: {}\n".format(sum_type)
        self._accumulate_log(log)
        sum_type = common.SUM_IN_LAMBDA if sum_type == PropertyNames.INCOHERENT else common.SUM_IN_Q
        foreground_name, direct_foreground_name = self.sum_foreground(
            reflected_beam_name, foreground_name, sum_type, angle_index, direct_foreground_name
        )
        theta_ws_name_cropped = ""
        theta_ws_name = "{}_new_twoTheta".format(reflected_beam_name[: reflected_beam_name.find("_")])
        if self.getProperty("CorrectGravity").value:
            theta_ws_name_cropped = self.crop_theta_correction(foreground_name, theta_ws_name, angle_index)
        final_two_theta = mtd[foreground_name].spectrumInfo().twoTheta(0) * 180 / math.pi
        log = "Calibrated 2theta of foreground centre [degree]: {0:.5f}\n".format(final_two_theta)
        self._accumulate_log(log)
        if sum_type == common.SUM_IN_Q:
            is_Bent = mtd[foreground_name].run().getProperty("beam_stats.bent_sample").value
            log = "Sample: {0}\n".format("Bent" if is_Bent == 1 else "Flat")
            self._accumulate_log(log)
        self._auto_cleanup.cleanupLater(reflected_beam_name)
        self._auto_cleanup.cleanupLater(foreground_name)
        self._auto_cleanup.cleanupLater(theta_ws_name)
        self._auto_cleanup.cleanupLater(theta_ws_name_cropped)
        return foreground_name, direct_foreground_name, theta_ws_name_cropped

    def save_parameters(self, ws: str) -> None:
        """Saves logs and workspace parameters used in the reduction to an ASCII file.

        Keyword parameters:
        ws -- workspace used to obtain reduction parameters
        """
        save_path = config["defaultsave.directory"]
        parameter_output = os.path.join(save_path, "{}.out".format(mtd[ws].name()))

        is_group = isinstance(mtd[ws], WorkspaceGroup)
        run = mtd[ws][0].run() if is_group else mtd[ws].run()
        try:
            wksp = mtd[ws][0] if is_group else mtd[ws]
            log_list = wksp.getInstrument().getStringParameter("reduction_logs_to_save")[0]
        except IndexError:
            self.log().warning("A list of reduction logs to save not specified, cannot save them.")
            return
        property_list = [log.strip() for log in log_list.split(",")]  # removes empty spaces from log names
        with open(parameter_output, "w") as outfile:
            log_output = "".join(self._logs)
            outfile.write(log_output)
            outfile.write("\n")  # creates a visual break between reduction and sample logs
            for prop in property_list:
                if run.hasProperty(prop):
                    property_val = run.getLogData(prop).value
                    if isinstance(property_val, int) or isinstance(property_val, float):
                        property_val = round(property_val, 4)
                    outfile.write("{}\t{}\n".format(prop, str(property_val)))

    def setup(self) -> None:
        self._subalg_logging = self.getProperty(Prop.SUBALG_LOGGING).value
        self._cleanup = self.getProperty(Prop.CLEANUP).value
        self._auto_cleanup = utils.Cleanup(self._cleanup, self._subalg_logging == SubalgLogging.ON)
        self._out_ws = self.getPropertyValue(Prop.OUTPUT_WS)
        self._db = self.getProperty(PropertyNames.DB).value
        self._rb = self.getProperty(PropertyNames.RB).value
        self._rb00 = self.getProperty(PropertyNames.RB00).value
        self._rb01 = self.getProperty(PropertyNames.RB01).value
        self._rb10 = self.getProperty(PropertyNames.RB10).value
        self._rb11 = self.getProperty(PropertyNames.RB11).value
        self._dimensionality = len(self._rb) if self._rb else len(self._rb00)
        self._logs = list()

    def sum_foreground(
        self, input_ws_name: str, output_ws_name: str, sum_type: str, angle_index: int, direct_foreground_name=""
    ) -> Tuple[str, str]:
        """Runs the ReflectometryILLSumForeground algorithm on the input workspace. The direct_foreground_name argument decides,
        if reflected beam is present, when it is empty, the summing is of direct beam, and of reflected beam otherwise.

        Keyword arguments:
        input_ws_name -- name of the workspace to be summed
        output_ws_name -- name for the output workspace
        sum_type -- defines the summation type to be performed; one out of SumInQ and SumInLambda
        angle_index -- index of the currently processed reflected runs
        direct_foreground_name -- name of the direct foreground workspace, if empty, the input_ws_name is assumed to be reflected beam
        """
        wavelengthRange = [
            float(self.get_value(PropertyNames.WAVELENGTH_LOWER, angle_index)),
            float(self.get_value(PropertyNames.WAVELENGTH_UPPER, angle_index)),
        ]
        if direct_foreground_name == "" and sum_type == common.SUM_IN_Q:
            wavelengthRange = [
                float(self.getProperty(PropertyNames.WAVELENGTH_LOWER).getDefault),
                float(self.getProperty(PropertyNames.WAVELENGTH_UPPER).getDefault),
            ]
            sum_type = common.SUM_IN_LAMBDA

        direct_beam_name = direct_foreground_name[:-4] if direct_foreground_name else ""
        ReflectometryILLSumForeground(
            InputWorkspace=input_ws_name,
            OutputWorkspace=output_ws_name,
            SummationType=sum_type,
            DirectForegroundWorkspace=direct_foreground_name,
            DirectLineWorkspace=direct_beam_name,
            WavelengthRange=wavelengthRange,
            SubalgorithmLogging=self._subalg_logging,
            Cleanup=self._cleanup,
        )
        if direct_foreground_name:
            if "{}_rebinned".format(direct_foreground_name) in mtd:
                direct_foreground_name = "{}_rebinned".format(direct_foreground_name)
            log = "Final source (mid chopper) to sample distance [m]: {:.5f}\n".format(mtd[output_ws_name].spectrumInfo().l1())
            self._accumulate_log(log)
            log = "Final reflected foreground centre distance [m]: {:.5f}\n".format(mtd[output_ws_name].spectrumInfo().l2(0))
            self._accumulate_log(log)
            log = "Final source (mid chopper) to foreground centre distance [m]: {:.5f}\n".format(
                mtd[output_ws_name].spectrumInfo().l1() + mtd[output_ws_name].spectrumInfo().l2(0)
            )
            self._accumulate_log(log)
        return output_ws_name, direct_foreground_name


AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
