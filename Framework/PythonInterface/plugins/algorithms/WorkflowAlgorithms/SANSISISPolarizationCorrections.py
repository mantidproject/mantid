# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    AnalysisDataService as ADS,
    DataProcessorAlgorithm,
    Progress,
    WorkspaceGroup,
    WorkspaceFactory,
    FileProperty,
    FileAction,
)
from mantid.kernel import (
    ConfigService,
    Direction,
    LogicOperator,
    FloatBoundedValidator,
    StringArrayProperty,
    StringListValidator,
    SpinStateValidator,
    PropertyCriterion,
    EnabledWhenProperty,
)
from mantid.simpleapi import DeleteWorkspace, DeleteWorkspaces

from sans.user_file.toml_parsers.toml_v2_schema import TomlSchemaV2Validator
from sans.user_file.toml_parsers.toml_base_schema import TomlValidationError

from contextlib import contextmanager
from collections import namedtuple
from dataclasses import dataclass, asdict
from functools import reduce
from operator import getitem
from typing import Union, Any, TYPE_CHECKING

import numpy as np
import os
import toml

if TYPE_CHECKING:
    from mantid.api import MatrixWorkspace


# Dataclasses
@dataclass
class InstrumentConfig:
    name: str = "LARMOR"
    trans_mon: int = 4
    norm_mon: int = 1
    z_dis: float = 0.0
    detector_offset: float = 0
    sample_offset: float = 0
    wav_min: float = 3.1
    wav_max: float = 12.5
    wav_step: float = 0.024
    spin_state_str: str = None
    units: str = "Wavelength"


@dataclass
class Parameter:
    name: str = ""
    value: float = float("nan")
    error: float = 0.0
    initial: float = 0.0
    timestamp: str = ""


@dataclass
class WsInfo:
    ads_name: str = None
    path: str = None  # path loaded from toml file


FIT_PROPERTIES = ["pxd", "lifetime", "pol"]
FIT_TABLE_COLUMN_NAMES = ["name", "value", "error", "initial", "timestamp"]
FIT_TABLE_COLUMN_TYPES = ["str", *["float"] * 3, "str"]
FIT_PROPERTIES_GROUP_NAME = "Fitting Initial Values"
AUX_WS_BASENAMES_CALIBRATION = ["analyzer_table", "direct", "depolarized", "depolarized_transmission", "empty_cell"]
AUX_WS_BASENAMES_CORRECTION = ["analyzer_time", "unpolarized_transmission"]
ALLOWED_NON_GROUPS = ["direct"]

_inst_nt = namedtuple("CompatibleInstruments", ["zoom", "larmor"], defaults=["ZOOM", "LARMOR"])
_prop_nt = namedtuple(
    "MainProperties",
    [
        "scattering",
        "transmission",
        "direct",
        "cell",
        "user",
        "path",
        "reduction",
        "suffix",
        "ads",
        "sp_assert",
        "second_flipper",
        "delete_partial",
        *FIT_PROPERTIES,
    ],
    defaults=[
        "ScatteringRuns",
        "TransmissionRuns",
        "DirectBeamRun",
        "DepolarizedCellRun",
        "UserFilePath",
        "SavePath",
        "ReductionType",
        "OutputSuffix",
        "KeepWsOnADS",
        "AssertSpinState",
        "SecondFlipperEfficiency",
        "DeletePartialWsOnFail",
        "PxDInitialValue",
        "LifetimeInitialValue",
        "HePolInitialValue",
    ],
)
_reduction_nt = namedtuple(
    "ReductionTypes", ["calibration", "both", "correction"], defaults=["Calibration", "CalibrationAndCorrection", "Correction"]
)
_parameter_nt = namedtuple("FitParameters", [*FIT_PROPERTIES], defaults=["PXD", "Lifetime", "InitialPolarization"])

_algs_nt = namedtuple(
    "ReductionAlgorithms",
    [
        "assert_spin",
        "clone",
        "units",
        "conjoin",
        "create",
        "delete",
        "mult_delete",
        "depolarized",
        "det_spin",
        "divide",
        "extract_sp",
        "flipper",
        "group",
        "analyzer",
        "analyzer_t",
        "int_rebin",
        "join_eff",
        "load",
        "mean",
        "move_inst",
        "pol_eff",
        "polarizer",
        "rebin",
        "rebin_ws",
        "rename",
        "save_nxs",
        "scale",
        "time_diff",
        "ungroup",
        "mean_w",
    ],
    defaults=[
        "AssertSpinStateOrder",
        "CloneWorkspace",
        "ConvertUnits",
        "ConjoinWorkspaces",
        "CreateWorkspace",
        "DeleteWorkspace",
        "DeleteWorkspaces",
        "DepolarizedAnalyserTransmission",
        "DetermineSpinStateOrder",
        "Divide",
        "ExtractSingleSpectrum",
        "FlipperEfficiency",
        "GroupWorkspaces",
        "HeliumAnalyserEfficiency",
        "HeliumAnalyserEfficiencyTime",
        "InterpolatingRebin",
        "JoinISISPolarizationEfficiencies",
        "Load",
        "Mean",
        "MoveInstrumentComponent",
        "PolarizationEfficiencyCor",
        "PolarizerEfficiency",
        "Rebin",
        "RebinToWorkspace",
        "RenameWorkspace",
        "SaveNexusProcessed",
        "Scale",
        "TimeDifference",
        "UnGroupWorkspace",
        "WeightedMean",
    ],
)

# named tuple instances
INST = _inst_nt()
MAIN_PROPERTIES = _prop_nt()
REDUCTION = _reduction_nt()
FIT = _parameter_nt()
ALGS = _algs_nt()

EFF_KEYS_USER = dict(
    analyzer_eff=["polarization", "analyzer", "efficiency"],
    flipper_eff=["polarization", "flipper", "polarizing", "efficiency"],
    flipper_a_eff=["polarization", "flipper", "flipping", "efficiency"],
    pol_fit_table=["polarization", "analyzer", "initial_polarization"],
    polarizer_eff=["polarization", "polarizer", "efficiency"],
    empty_cell=["polarization", "analyzer", "empty_cell"],
)

# If we retrieve this property from child algs:
OUT_WS = "OutputWorkspace"
# Prefix for temporary ADS workspaces
TMP_NAME = "__polsans_"
# Digits to prepend to corrected scattering output workspaces
PREPEND_OUTPUT = "000"
# Suffices for corrected scattering  output workspaces
EVENT_GROUP_SUFFIX = "-add"
EVENT_SUFFIX = "_added_event_data"
EVENT_MONITOR_SUFFIX = "_monitors_added_event_data"


# Helpers
def _add_suffix(name: str, suffix: str) -> str:
    return f"{name}_{suffix}" if suffix else name


def _str_to_list(str_list: Union[list[str], str]) -> list[str]:
    return [str_list] if not isinstance(str_list, list) else str_list


def _lower_dict_keys(d: dict[str, Any]) -> dict[str, Any]:
    # This is just a style choice, to avoid Caps in python dataclass fields as we unpack directly the fit
    # table rows to initialize the Parameter objects.
    for k in list(d.keys()):
        d[k.lower()] = d.pop(k)
    return d


def _build_fit_table(table_name: str, parameter_dict: dict[str, Parameter]):
    table = WorkspaceFactory.Instance().createTable()
    for n, t in zip(FIT_TABLE_COLUMN_NAMES, FIT_TABLE_COLUMN_TYPES):
        table.addColumn(name=n, type=t)
    for row in parameter_dict.values():
        table.addRow(asdict(row))
    _add_or_replace(table_name, table)


def _build_parameters_from_table(table_name: str) -> dict[str, Parameter]:
    parameters = {}
    fit_table = ADS.retrieve(table_name)
    for row in fit_table:
        parameters.update({row["name"]: Parameter(**row)})
    return parameters


# Userfile
def _get_value_from_dict(input_dict: dict[str, Any], keys: list[str], default_value: Any, mandatory: bool = True) -> Any:
    """
    Access userfile parsed from TOML file from a set of keys indicating a path into the nested dictionary.
    A default-value is used if the value found is empty. If there is a missing key, an error is raised
    only if the property is required for the reduction to run.
    """
    val = None
    try:
        val = reduce(getitem, keys, input_dict)
    except KeyError as key_err:
        if mandatory:
            raise KeyError(key_err)
    return val or default_value


def _user_file_reader(file_path: str, names_dict: dict[str, WsInfo]) -> InstrumentConfig:
    """
    Opens the TOML user file and retrieves only the required values to build the helper dataclasses that are needed for the reduction.
    Returns the calibration object and fills the efficiency files paths.
    """
    try:
        with open(file_path, "r") as f:
            user_file = toml.load(f)
        # We make sure the user file has keys in the SANS TOML V2 schema
        TomlSchemaV2Validator(user_file).validate()

        # Initialize helper objects
        instrument = InstrumentConfig()

        # Instrument
        inst_list = ["instrument"]
        conf_list = inst_list + ["configuration"]
        instrument.name = _get_value_from_dict(user_file, inst_list + ["name"], instrument.name)
        instrument.sample_offset = _get_value_from_dict(user_file, conf_list + ["sample_offset"], instrument.sample_offset)
        norm_monitor_key = _get_value_from_dict(user_file, conf_list + ["norm_monitor"], "")
        trans_monitor_key = _get_value_from_dict(user_file, conf_list + ["trans_monitor"], "")
        # Detector
        instrument.detector_offset = _get_value_from_dict(
            user_file, ["detector", "correction", "position", "rear_z"], instrument.detector_offset, False
        )
        # Monitors
        instrument.trans_mon = _get_value_from_dict(
            user_file, ["transmission", "monitor", trans_monitor_key, "spectrum_number"], instrument.trans_mon
        )
        instrument.norm_mon = _get_value_from_dict(
            user_file, ["normalisation", "monitor", norm_monitor_key, "spectrum_number"], instrument.norm_mon
        )
        instrument.z_dis = _get_value_from_dict(user_file, ["transmission", "monitor", trans_monitor_key, "shift"], instrument.z_dis, False)
        # Binning
        wav_dict = _get_value_from_dict(
            user_file,
            ["binning", "wavelength"],
            {"start": instrument.wav_min, "stop": instrument.wav_max, "step": instrument.wav_step},
            False,
        )
        for attr_val, dict_key in zip(["wav_min", "wav_max", "wav_step"], ["start", "stop", "step"]):
            setattr(instrument, attr_val, wav_dict[dict_key])
        # Polarization
        instrument.spin_state_str = _get_value_from_dict(user_file, ["polarization", "flipper_configuration"], instrument.spin_state_str)
        # Efficiency files paths
        for name, key_list in EFF_KEYS_USER.items():
            if name in names_dict:
                names_dict[name].path = _get_value_from_dict(user_file, key_list, "", False)

        return instrument

    except (KeyError, TomlValidationError) as key_err:
        raise RuntimeError(f"There are missing or incorrect keys in the user file : {key_err}")


# ADS management


def _clean_temporary_ws(additional_names=None):
    """
    Cleans workspaces in ADS with temporary suffix, or if they are on an additional list
    """
    if additional_names is None:
        additional_names = []
    sel_names = list(filter(lambda name: name.startswith(TMP_NAME) or name in additional_names, ADS.getObjectNames()))
    if sel_names:
        DeleteWorkspaces(sel_names)


def _add_or_replace(ws_name: str, workspace: Union["MatrixWorkspace", WorkspaceGroup, str]):
    """
    AnalysisDataService `addOrReplace` method does not remove group members if the workspace to be replaced is a group.
    Here we make sure all members of the group are deleted too by calling DeleteWorkspace algorithm first.
    """
    if not ADS.doesExist(ws_name):
        ADS.add(ws_name, workspace)
    else:
        if isinstance(ADS.retrieve(ws_name), WorkspaceGroup):
            DeleteWorkspace(ws_name)
            ADS.add(ws_name, workspace)
        else:
            ADS.addOrReplace(ws_name, workspace)


@contextmanager
def _manage_temps(inst_name: str, initial_temp_names: dict[str, WsInfo], delete_on_fail: bool = False):
    """
    Context manager to handle temporary workspace deletion and settings of ConfigService
    """

    def set_config(conf_dict: dict[str, str]):
        for k, v in conf_dict.items():
            ConfigService[k] = v

    reduction_config = {
        "default.facility": "ISIS",
        "datasearch.searcharchive": "On",
        "default.instrument": inst_name,
        "MantidOptions.InvisibleWorkspaces": "1",
    }
    current_conf = {k: ConfigService[k] for k in reduction_config.keys()}
    set_config(reduction_config)
    temp_names = initial_temp_names
    try:
        yield temp_names
    except RuntimeError as e:
        if delete_on_fail:
            _clean_temporary_ws(additional_names=[v.ads_name for v in temp_names.values()])
        raise RuntimeError(e)
    finally:
        set_config(current_conf)


class SANSISISPolarizationCorrections(DataProcessorAlgorithm):
    instrument: InstrumentConfig = None
    parameters: dict[str, Parameter] = None
    aux_ws: dict[str, WsInfo] = None
    delete_ads_ws: bool = False
    delete_partial: bool = True
    assert_spin: bool = True
    has_2nd_flipper: bool = False
    reduction_type: str = None
    suffix: str = None
    progress: Progress = None
    eff_basenames: list[str] = None
    scattering_runs: list[str] = None
    transmission_runs: list[str] = None

    def category(self):
        return "SANS\\PolarizationCorrections"

    def name(self):
        return "SANSISISPolarizationCorrections"

    def summary(self):
        return (
            "Wrapper algorithm that calculates a spin leakage calibration and correction for a PA-SANS setup. "
            "In ISIS, compatible with ZOOM and LARMOR instruments."
        )

    def seeAlso(self):
        return ["HeliumAnalyserEfficiency", "PolarizerEfficiency", "FlipperEfficiency", "PolarizationEfficiencyCor"]

    def PyInit(self):
        self.declareProperty(
            FileProperty(MAIN_PROPERTIES.user, "Path to user file in SANS TOML V2 FORMAT", action=FileAction.Load, extensions=[".toml"])
        )

        self.declareProperty(
            StringArrayProperty(MAIN_PROPERTIES.transmission, direction=Direction.Input),
            doc="Input transmission runs. Comma separated run numbers of the transmission runs. Used for calibration of efficiencies.",
        )

        self.declareProperty(
            StringArrayProperty(MAIN_PROPERTIES.scattering, direction=Direction.Input),
            doc="Input scattering runs. Comma separated run numbers to which to apply corrections.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.direct,
            direction=Direction.Input,
            defaultValue="",
            doc="Direct beam run. Used for calibration of efficiencies.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.cell,
            direction=Direction.Input,
            defaultValue="",
            doc="Depolarized cell run. Used for calibration of efficiencies.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.reduction,
            defaultValue=REDUCTION.both,
            direction=Direction.Input,
            validator=StringListValidator(list(REDUCTION)),
            doc="The type of reduction: Choose Calibration to generate and save efficiency files, "
            "Correction to apply efficiency corrections to scattering data, or CalibrationAndCorrections to do both.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.suffix,
            defaultValue="",
            doc="Suffix to identify processed efficiencies. "
            "This is useful, for example, if a set of different cells or samples "
            "are used as inputs.",
        )
        self.declareProperty(
            FileProperty(MAIN_PROPERTIES.path, "", action=FileAction.OptionalDirectory, direction=Direction.Input),
            doc="Path to save directory of processed workspaces.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.ads,
            defaultValue=True,
            doc="Whether to keep reduced files on the ADS. Can't be unchecked unless a save path is provided in SavePath.",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.sp_assert,
            defaultValue=True,
            doc=(
                "Whether to use AssertSpinStateOrder to determine that the expected spin state order is the same for all transmission "
                "runs. Set to False if there is no confidence that the spin can be determined accurately "
                "from the DetermineSpinStateOrder algorithm"
            ),
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.delete_partial,
            defaultValue=True,
            doc=(
                "If the algorithm fails due to poor fitting or some other reason, partial workspaces are automatically deleted"
                "unless this property is set to False. Useful for diagnostics."
            ),
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.second_flipper,
            defaultValue=False,
            doc=(
                "Whether to perform calculation for second flipper efficiency in calibration, as well as use second flipper efficiency "
                "on correction if a path for the second flipper is found on the UserFile"
            ),
        )

        always_positive = FloatBoundedValidator(lower=0)

        self.declareProperty(
            name=MAIN_PROPERTIES.pxd,
            direction=Direction.Input,
            validator=always_positive,
            defaultValue=12.6,
            doc="Initial value of mean path length (pxd) for fitting in DepolarizedAnalyserTransmission algorithm",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.lifetime,
            direction=Direction.Input,
            validator=always_positive,
            defaultValue=54.0,
            doc="Initial value of lifetime (in hours) for fitting in HeliumAnalyserEfficiency algorithm",
        )

        self.declareProperty(
            name=MAIN_PROPERTIES.pol,
            direction=Direction.Input,
            validator=always_positive,
            defaultValue=0.6,
            doc="Initial value of helium gas polarization for fitting in HeliumAnalyserEfficiency algorithm",
        )

        for prop_name in [MAIN_PROPERTIES.pxd, MAIN_PROPERTIES.lifetime, MAIN_PROPERTIES.pol]:
            self.setPropertyGroup(prop_name, FIT_PROPERTIES_GROUP_NAME)

        self.setPropertySettings(MAIN_PROPERTIES.ads, EnabledWhenProperty(MAIN_PROPERTIES.path, PropertyCriterion.IsNotDefault))
        calibration = EnabledWhenProperty(MAIN_PROPERTIES.reduction, PropertyCriterion.IsEqualTo, REDUCTION.calibration)
        calibration_or_both = EnabledWhenProperty(
            calibration, EnabledWhenProperty(MAIN_PROPERTIES.reduction, PropertyCriterion.IsEqualTo, REDUCTION.both), LogicOperator.Or
        )
        self.setPropertySettings(
            MAIN_PROPERTIES.scattering,
            EnabledWhenProperty(MAIN_PROPERTIES.reduction, PropertyCriterion.IsNotEqualTo, REDUCTION.calibration),
        )

        for prop_name in [MAIN_PROPERTIES.cell, MAIN_PROPERTIES.direct, MAIN_PROPERTIES.transmission]:
            self.setPropertySettings(prop_name, calibration_or_both)

    def validateInputs(self):
        issues = dict()

        red_type = self.getPropertyValue(MAIN_PROPERTIES.reduction)
        run_props = [self.getProperty(prop_name) for prop_name in MAIN_PROPERTIES[:4]]
        match red_type:
            case REDUCTION.correction:
                necessary_props = [run_props[0]]
            case REDUCTION.calibration:
                necessary_props = run_props[1:]
            case REDUCTION.both:
                necessary_props = run_props
            case _:
                necessary_props = []
        for prop in necessary_props:
            if prop.isDefault:
                issues[prop.name] = f"This property is necessary for reduction of type {red_type}"

        return issues

    def PyExec(self):
        self._init_reduction_settings()
        with _manage_temps(self.instrument.name, self.aux_ws, self.delete_partial) as self.aux_ws:
            if self.reduction_type != REDUCTION.correction:
                self._polarization_calibration()

            if self.reduction_type != REDUCTION.calibration:
                self._polarization_corrections()

    def _init_reduction_settings(self):
        """
        Prepare lists of runs, read user file and initialize the config and parameter objects as well as
        the progress bar
        """
        self.suffix = self.getPropertyValue(MAIN_PROPERTIES.suffix)
        self.scattering_runs = self.getProperty(MAIN_PROPERTIES.scattering).value
        self.transmission_runs = self.getProperty(MAIN_PROPERTIES.transmission).value
        self.delete_ads_ws = not self.getProperty(MAIN_PROPERTIES.ads).value
        self.delete_partial = self.getProperty(MAIN_PROPERTIES.delete_partial).value
        self.assert_spin = self.getProperty(MAIN_PROPERTIES.sp_assert).value
        self.has_2nd_flipper = self.getProperty(MAIN_PROPERTIES.second_flipper).value

        # We add efficiency auxiliary names here. We'll only delete those in the ADS at the end in case of error.
        excluded = ["flipper_a_eff"] if not self.has_2nd_flipper else []
        self.eff_basenames = [key for key in [*EFF_KEYS_USER.keys()] if key not in excluded]
        self.aux_ws = {
            name: WsInfo(ads_name=_add_suffix(name, self.suffix)) for name in [*self.eff_basenames, *AUX_WS_BASENAMES_CALIBRATION]
        }

        self.parameters = {
            (fit_name := getattr(FIT, k)): Parameter(name=fit_name, initial=self.getProperty(getattr(MAIN_PROPERTIES, k)).value)
            for k in FIT_PROPERTIES
        }
        self.instrument = _user_file_reader(self.getPropertyValue(MAIN_PROPERTIES.user), self.aux_ws)

        # We report at the beginning of the main steps in calibration and correction and in the bottlenecks, which are
        # applying the scattering corrections and saving the corrected data.
        self.reduction_type = self.getPropertyValue(MAIN_PROPERTIES.reduction)
        n_reports = (1 + len(self.scattering_runs) * 2) * int(self.reduction_type != REDUCTION.calibration) + 2 * int(
            self.reduction_type != REDUCTION.correction
        )
        self.progress = Progress(self, start=0.0, end=1.0, nreports=n_reports)

    def _create_run_list(self, run_list: Union[str, list[str]], prefix: str, append_suffix=False) -> list[str]:
        """
        Creates a list of run names based on a prefix and suffix property. Adds these runs to the dict of added runs in aux_ws
        """
        names = []
        for name in _str_to_list(run_list):
            ads_name = _add_suffix(f"{prefix}_{name}", self.suffix if append_suffix else None)
            dict_key = (
                prefix if prefix in [*EFF_KEYS_USER.keys(), *AUX_WS_BASENAMES_CALIBRATION, *AUX_WS_BASENAMES_CORRECTION] else ads_name
            )
            self.aux_ws.update({dict_key: WsInfo(ads_name=ads_name)})
            names.append(ads_name)

        return names

    def _filter_ads_names(self, prefix_list: Union[str, list[str]]):
        """
        Filter from the list of added ws based on prefix
        """
        return list(
            filter(
                lambda name: any([name.startswith(prefix) for prefix in _str_to_list(prefix_list)]),
                [v.ads_name for v in self.aux_ws.values()],
            )
        )

    def _polarization_calibration(self):
        """
        Calculates the efficiency of the polarizer, flipper and analyzer elements.
        """
        self._calculate_cell_opacity()
        transmission_runs_list = self._load_and_process_runs(
            self.transmission_runs, "transmission", average_group=False, move_instrument=False, convert_units=True, rebin=True
        )
        self._process_efficiencies(transmission_runs_list)
        self._save_and_clean_results(delete_ads=self.delete_ads_ws and self.reduction_type != REDUCTION.both)

    def _polarization_corrections(self):
        """
        Applies polarization corrections to a series of polarized scattering runs
        """
        self.progress.report("Starting Polarization Corrections to Scattering Runs")

        if self.getProperty(MAIN_PROPERTIES.reduction).value == REDUCTION.correction:
            # We already have the efficiencies in the ADS and the parameters from the calibration
            self._load_workspace_from_path([key for key in self.eff_basenames if key != "empty_cell"])
            self.parameters = _build_parameters_from_table(self.aux_ws["pol_fit_table"].ads_name)

        scattering_runs_list = self._load_and_process_runs(
            self.scattering_runs, "scattering", average_group=False, move_instrument=False, convert_units=True, rebin=False
        )
        self._apply_corrections(scattering_runs_list)
        self._save_and_clean_results(scattering_runs_list, delete_ads=self.delete_ads_ws)

    def _apply_corrections(self, scattering_runs: list[str]):
        """
        This is the main correction step. For each scattering run, get its efficiencies and apply the correction
        """
        for name in scattering_runs:
            self.progress.report(f"Applying corrections to run {name}")
            self._load_child_algorithm(
                ALGS.pol_eff,
                InputWorkspaceGroup=name,
                OutputWorkspace=TMP_NAME + name,
                Efficiencies=self._get_efficiencies(name),
                Flippers=self.instrument.spin_state_str,
            )
            self._load_child_algorithm(ALGS.rename, InputWorkspace=TMP_NAME + name, OutputWorkspace=name)

    def _get_efficiencies(self, ws_name: str) -> "MatrixWorkspace":
        """
        Flipper and polarizer efficiencies are loaded from file, or retrieved from ads. Analyzer efficiency needs to be calculated
        if we have decay information from the calibration step.
        """
        eff_list = [self.aux_ws[k].ads_name for k in ["polarizer_eff", "flipper_eff"]]

        if np.isnan(self.parameters[FIT.lifetime].value):
            eff_list.append(self.aux_ws["analyzer_eff"].ads_name)  # need scientist commentary on this
        else:
            self._helium_analyzer_efficiency_at_scattering_run_time(ws_name)
            self._scale_group(ws_name, divisor_ws=ADS.retrieve(self.aux_ws["unpolarized_transmission"].ads_name), scale_factor=1.0)
            eff_list.append(self.aux_ws["analyzer_time"].ads_name)

        return self._join_efficiencies(eff_list)

    def _load_run(self, run: str, group_name: str = TMP_NAME, load_monitors: bool = False):
        """
        Loads the run and sets its name to `group_name`, optionally loads the monitor if applicable
        An error is raised is the workspace is not a group.
        """
        self._load_child_algorithm(ALGS.load, Filename=run, OutputWorkspace=group_name, LoadMonitors=str(int(load_monitors)))
        if not (
            any([group_name.startswith(excluded) for excluded in ALLOWED_NON_GROUPS])
            or isinstance(ADS.retrieve(group_name), WorkspaceGroup)
        ):
            raise RuntimeError(f"Run {run} is not compatible with the polarization reduction")

    def _average_workspaces(self, ws_list: list[str], out_name: str, ungroup: bool = False):
        """
        Average a group of workspaces using the Mean algorithm. The averaged workspaces are also deleted.
        """
        if ungroup:
            # If some of the workspaces to average are in a group, we unroll the group and retrieve the names
            temp_list = []
            for ws_name in ws_list:
                if isinstance(ws := ADS.retrieve(ws_name), WorkspaceGroup):
                    temp_list.extend(list(ws.getNames()))
                    self._load_child_algorithm(ALGS.ungroup, InputWorkspace=ws_name)
                else:
                    temp_list.append(ws_name)
            ws_list = temp_list

        self._load_child_algorithm(ALGS.mean, Workspaces=",".join(ws_list), OutputWorkspace=out_name)
        self._load_child_algorithm(ALGS.mult_delete, WorkspaceList=ws_list)

    def _move_instrument(self, ws_name: str):
        """
        Moves the relevant components of instruments to desired calibration positions.
        """
        components = ["monitor-tbd", "some-sample-holder", *(["rear_detector"] if self.instrument.name is INST.zoom else [])]
        det_ids = [self.instrument.trans_mon, None, None]
        z_offset = [self.instrument.z_dis, self.instrument.sample_offset, self.instrument.z_dis - self.instrument.detector_offset]

        for component, det_id, value in zip(components, det_ids, z_offset):
            if value:
                params = dict(zip(["DetectorID", "Z"], [det_id, value])) if det_id else {"Z": value}
                self._load_child_algorithm(ALGS.move_inst, Workspace=ws_name, ComponentName=component, **params)

    def _scale_group(self, ws_name: str, divisor_ws: str, scale_factor: float = 0.5):
        self._load_child_algorithm(ALGS.scale, InputWorkspace=ws_name, Factor=scale_factor, OutputWorkspace=ws_name)
        self._load_child_algorithm(ALGS.divide, LHSWorkspace=ws_name, RHSWorkspace=divisor_ws, OutputWorkspace=ws_name)

    def _calculate_cell_opacity(self):
        """
        Calculates helium cell opacity and depolarized transmission
        """
        self.progress.report("Calculating cell opacity")
        # Load depolarized cell and direct runs
        runs = [self.getProperty(prop).value for prop in [MAIN_PROPERTIES.cell, MAIN_PROPERTIES.direct]]
        for run, name in zip(runs, ["depolarized", "direct"]):
            self._load_and_process_runs(run, name, average_group=True, move_instrument=True, convert_units=True, rebin=True)

        depol_norm, direct_norm = self._calculate_transmission(
            self.aux_ws["depolarized"].ads_name, self.aux_ws["depolarized_transmission"].ads_name, extract_normalized_spectra=True
        )

        self._calculate_depolarized_analyzer_parameters(depol_norm, direct_norm)

        _clean_temporary_ws()

    def _join_efficiencies(self, eff_list: list[str] = None) -> "MatrixWorkspace":
        """
        Join efficiencies algorithm. If there is not a second flipper for LARMOR or ZOOM,
        we create a flat workspace for F2 based off the first flipper efficiency
        """
        pol_name, flip_p_name, analyzer_name = eff_list

        if not self.has_2nd_flipper:
            flip_a_name = f"{TMP_NAME}_flipper_a_eff"
            fp = ADS.retrieve(flip_p_name)
            x = fp.readX(0).tolist()
            ones = np.ones_like(fp.readY(0)).tolist()
            self._load_child_algorithm(
                ALGS.create, OutputWorkspace=flip_a_name, DataX=x, DataY=ones, DataE=ones, UnitX=self.instrument.units
            )
        else:
            flip_a_name = self.aux_ws["flipper_a_eff"].ads_name

        return self._load_child_algorithm(ALGS.join_eff, OUT_WS, P1=pol_name, F1=flip_p_name, P2=analyzer_name, F2=flip_a_name)

    def _helium_analyzer_efficiency_at_scattering_run_time(self, ws_name: str):
        """
        Runs the helium analyzer efficiency algorithm at arbirary time
        and rebins the efficiency ws to the set reduction binning
        """
        pol_params = {}
        ref_times = []
        for row in self.parameters.values():
            if (param := row.name) in FIT[:]:
                pol_params.update({param: row.value, param + "Error": row.error})
            elif param not in FIT[:] and (ref_time := row.timestamp):
                ref_times.append(ref_time)

        self.aux_ws.update({name: WsInfo(ads_name=_add_suffix(name, self.suffix)) for name in AUX_WS_BASENAMES_CORRECTION})
        out_name = self.aux_ws["analyzer_time"].ads_name
        self._load_child_algorithm(
            ALGS.analyzer_t,
            InputWorkspace=ws_name,
            ReferenceTimeStamp=ref_times[0],
            UnpolarizedTransmission=self.aux_ws["unpolarized_transmission"].ads_name,
            OutputWorkspace=out_name,
            **pol_params,
        )

        binning = [self.instrument.wav_min, self.instrument.wav_step, self.instrument.wav_max]
        self._load_child_algorithm(ALGS.int_rebin, InputWorkspace=out_name, OutputWorkspace=out_name, Params=binning)

    def _calculate_helium_analyzer_efficiency_and_parameters(self, transmission_runs: list[str] = None) -> WorkspaceGroup:
        """
        Runs the helium analyzer efficiency algorithm and stores the fit results in a table
        and in the parameters' dict. Returns a reference to the efficiency group
        """
        self._load_child_algorithm(
            ALGS.analyzer,
            InputWorkspaces=transmission_runs,
            SpinStates=self.instrument.spin_state_str,
            PXD=self.parameters[FIT.pxd].value,
            PXDError=self.parameters[FIT.pxd].error,
            StartX=self.instrument.wav_min,
            EndX=self.instrument.wav_max,
            DecayTimeInitial=self.parameters[FIT.lifetime].initial,
            H3PolarizationInitial=self.parameters[FIT.pol].initial,
            OutputFitParameters=self.aux_ws["analyzer_table"].ads_name,
            OutputWorkspace=self.aux_ws["analyzer_eff"].ads_name,
        )

        table_decay = ADS.retrieve(self.aux_ws["analyzer_table"].ads_name)
        table_times = self._load_child_algorithm(ALGS.time_diff, OUT_WS, InputWorkspaces=transmission_runs)

        # Get reference time stamps for transmission data
        for rowIndex in range(len(transmission_runs)):
            phe_eff_row = table_decay.row(rowIndex)
            phe_eff_row["timestamp"] = table_times.row(rowIndex)["midtime_stamp"]
            name = f"Polarization_{transmission_runs[rowIndex]}"
            phe_eff_row["Name"] = name
            self.parameters.update({name: Parameter(**_lower_dict_keys(phe_eff_row))})

        # Add fit results to parameters dict
        if ("Height" and "Lifetime") in table_decay.column("Name"):
            index_height = table_decay.column("Name").index("Height")
            index_lifetime = table_decay.column("Name").index("Lifetime")
            self.parameters[FIT.pol].value = table_decay.column("Value")[index_height]
            self.parameters[FIT.pol].error = table_decay.column("Error")[index_height]
            self.parameters[FIT.lifetime].value = table_decay.column("Value")[index_lifetime]
            self.parameters[FIT.lifetime].error = table_decay.column("Error")[index_lifetime]

        _build_fit_table(self.aux_ws["pol_fit_table"].ads_name, self.parameters)

        return ADS.retrieve(self.aux_ws["analyzer_eff"].ads_name)

    def _calculate_depolarized_analyzer_parameters(self, ws_depol_name: str, direct_norm_name: str):
        """
        Calculates the mean gas length (p x d parameter) using the DepolarizedAnalyserTransmission algorithm
        and stores result in parameters struct
        """
        try:
            self._load_workspace_from_path("empty_cell")
        except RuntimeError:
            # We use the normalized transmission spectra of the direct run as empty_cell
            self._load_child_algorithm(ALGS.rename, InputWorkspace=direct_norm_name, OutputWorkspace=self.aux_ws["empty_cell"].ads_name)

        output = self._load_child_algorithm(
            ALGS.depolarized,
            OUT_WS,
            DepolarizedWorkspace=ws_depol_name,
            EmptyCellWorkspace=self.aux_ws["empty_cell"].ads_name,
            StartX=self.instrument.wav_min,
            EndX=self.instrument.wav_max,
            PxDStartingValue=self.parameters[FIT.pxd].initial,
            IgnoreFitQualityError=True,
        )

        self.parameters[FIT.pxd].value, self.parameters[FIT.pxd].error = output.column("Value")[0:2]

    def _check_spin_states(self, ws_name: str):
        """
        Checks each input run has the expected spin state or determines it if it is not set on the user file;
        Also, raises error if the determined spin is not valid or the expected spin value fails to assert.
        """
        if not self.instrument.spin_state_str:
            """ If the expected spin state is not set in the user file,
            we'll take the first run as reference for comparison with the rest"""
            spin = self._load_child_algorithm(ALGS.det_spin, "SpinStates", InputWorkspace=ws_name)
            if (msg := SpinStateValidator(allowedNumberOfSpins=[4]).isValid(spin)) == "":
                self.instrument.spin_state_str = spin
            else:
                raise RuntimeError(f"Failed to retrieve a valid spin configuration for run {ws_name} with determined spin {spin}: {msg}")
        else:
            if not self.assert_spin:
                # We check at least that the number of periods in the group is the same as the number of spins
                if ADS.retrieve(ws_name).getNumberOfEntries() != (sp_len := len(self.instrument.spin_state_str.split(","))):
                    raise RuntimeError(f"The number of periods in {ws_name} differs from the expected: {sp_len}")
            elif not self._load_child_algorithm(
                ALGS.assert_spin, "Result", InputWorkspace=ws_name, ExpectedSpinStates=self.instrument.spin_state_str
            ):
                raise RuntimeError(
                    f"The spin configuration for workspace {ws_name} differs from the expected: {self.instrument.spin_state_str}"
                )

    def _process_efficiencies(self, transmission_run_list: list[str]):
        """
        Process a single transmission run and calculate flipping ratios and efficiencies.
        """
        self.progress.report("Calculating Flipper, Polarizer and Analyzer efficiencies")

        pol_names = self._create_run_list(transmission_run_list, "pol")
        flipper_p_names = self._create_run_list(transmission_run_list, "flipper_p")
        flipper_a_names = self._create_run_list(transmission_run_list, "flipper_a") if self.has_2nd_flipper else None

        analyzer_eff_group = self._calculate_helium_analyzer_efficiency_and_parameters(transmission_run_list)

        for index, run in enumerate(transmission_run_list):
            params = {"InputWorkspace": run, "SpinStates": self.instrument.spin_state_str, "OutputWorkspace": flipper_p_names[index]}
            self._load_child_algorithm(ALGS.flipper, **params)
            self._load_child_algorithm(
                ALGS.polarizer, **dict(params, OutputWorkspace=pol_names[index]), AnalyserEfficiency=analyzer_eff_group.getItem(index)
            )
            if flipper_a_names:
                self._load_child_algorithm(ALGS.flipper, **dict(params, Flipper="Analyzer", OutputWorkspace=flipper_a_names[index]))

        # Average out the efficiencies before storing, the partial workspaces are deleted
        self._average_workspaces(flipper_p_names, self.aux_ws["flipper_eff"].ads_name)
        self._average_workspaces(pol_names, self.aux_ws["polarizer_eff"].ads_name)
        self._average_workspaces([self.aux_ws["analyzer_eff"].ads_name], self.aux_ws["analyzer_eff"].ads_name, ungroup=True)
        if flipper_a_names:
            self._average_workspaces(flipper_a_names, self.aux_ws["flipper_a_eff"].ads_name)

    def _save_results(self, ws_names_list: Union[set[str], list[str]] = None, is_scattering_data: bool = False):
        """
        Saving a list of names from the ads. If the saving is on scattering data, we need to merge the monitors before.
        """
        if not ((base_path := self.getPropertyValue(MAIN_PROPERTIES.path)) and ws_names_list):
            return

        for name in ws_names_list:
            if is_scattering_data:
                self.progress.report(f"Saving corrected workspace {name}")
            path = os.path.join(base_path, name + ".nxs")
            self._load_child_algorithm(ALGS.save_nxs, InputWorkspace=name, Filename=path, Title=name, PreserveEvents=False)

    def _prepare_corrected_data_for_save(self, names_to_correct: Union[str, list[str]]) -> list[str]:
        """
        Add the loaded monitors workspace to the corrected scattering workspaces
        """
        output_names = []
        for ws_name in _str_to_list(names_to_correct):
            _, run_name = ws_name.split("scattering_", 1)
            output, aux_output = [f"{pre}{self.instrument.name}{PREPEND_OUTPUT}{run_name}{EVENT_GROUP_SUFFIX}" for pre in ["", TMP_NAME]]
            monitor_name, aux_monitor_name = [f"{pre}{ws_name}_monitors" for pre in ["", TMP_NAME]]
            output_names.append(output)

            self._load_child_algorithm(ALGS.units, InputWorkspace=ws_name, OutputWorkspace=aux_output, Target="TOF", AlignBins=True)
            if has_monitor := ADS.doesExist(monitor_name):
                self.aux_ws[monitor_name] = WsInfo(ads_name=monitor_name)  # This is for deleting later
                self._load_child_algorithm(
                    ALGS.rebin_ws, WorkspaceToRebin=monitor_name, WorkspaceToMatch=aux_output, OutputWorkspace=monitor_name
                )
                # Conjoin doesn't have output workspace, Input1 swallows Input2, we clone the monitors ws in case it is reused
                self._load_child_algorithm(ALGS.clone, InputWorkspace=monitor_name, OutputWorkspace=aux_monitor_name)
                self._load_child_algorithm(ALGS.conjoin, InputWorkspace1=aux_monitor_name, InputWorkspace2=aux_output)
            self._load_child_algorithm(ALGS.rename, InputWorkspace=aux_monitor_name if has_monitor else aux_output, OutputWorkspace=output)

        return output_names

    def _prepare_workspace(
        self, ws_name: str, average_group: bool = False, move_instrument: bool = False, convert_units: bool = True, rebin: bool = True
    ):
        """
        Different preparation steps to input runs: average, move instrument, change units or rebinning
        """
        if average_group:
            self._average_group(ws_name)

        if move_instrument:
            self._move_instrument(ws_name)

        if convert_units:
            self._load_child_algorithm(
                ALGS.units, InputWorkspace=ws_name, OutputWorkspace=ws_name, Target=self.instrument.units, AlignBins=True
            )

        if rebin:
            binning = [self.instrument.wav_min, self.instrument.wav_step, self.instrument.wav_max]
            self._load_child_algorithm(ALGS.rebin, InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=binning, PreserveEvents=False)

    def _extract_normalized_monitor_spectra(self, ws_name: str) -> str:
        """
        Extracts normalized to monitor transmission spectra
        """
        trans, mon, out = [f"{TMP_NAME}{ws_name}_{suffix}" for suffix in ["trans_mon", "norm_mon", "normalized"]]

        self._load_child_algorithm(
            ALGS.extract_sp, InputWorkspace=ws_name, WorkspaceIndex=self.instrument.trans_mon - 1, OutputWorkspace=trans
        )
        self._load_child_algorithm(
            ALGS.extract_sp, InputWorkspace=ws_name, WorkspaceIndex=self.instrument.norm_mon - 1, OutputWorkspace=mon
        )

        self._load_child_algorithm(ALGS.divide, LHSWorkspace=trans, RHSWorkspace=mon, OutputWorkspace=out)
        return out

    def _calculate_transmission(
        self, ws_name: str, out_name: str = None, extract_normalized_spectra: bool = False
    ) -> Union[None, tuple[str, ...]]:
        """
        Calculate transmission dividing a given transmission run with name ws_name on the ads by the direct run. Optionally
        can output the normalized monitor spectra if necessary.
        """
        ws_norm_name = self._extract_normalized_monitor_spectra(ws_name)
        ws_direct_name = self._extract_normalized_monitor_spectra(self.aux_ws["direct"].ads_name)

        out_name = ws_name if not out_name else out_name
        self._load_child_algorithm(ALGS.divide, LHSWorkspace=ws_norm_name, RHSWorkspace=ws_direct_name, OutputWorkspace=out_name)

        return ws_norm_name, ws_direct_name if extract_normalized_spectra else None

    def _load_and_process_runs(
        self, run_list: Union[str, list[str]], prefix: str, average_group: bool, move_instrument: bool, convert_units: bool, rebin: bool
    ) -> list[str]:
        """
        This method loads a polarized run, which should be a workspace group. The group will be given a name for the ads and processed
        depending on the type of run. Generally we can average the group periods, convert units, rebin or move instrument.  Transmission
        runs (prefix=='transmission') will have additional processing.
        """
        run_list = _str_to_list(run_list)
        names = self._create_run_list(run_list, prefix)
        for run, name in zip(run_list, names):
            if not ADS.doesExist(name):
                self._load_run(run, name, load_monitors=prefix == "scattering")
                self._prepare_workspace(name, average_group, move_instrument, convert_units, rebin)
                if prefix == "transmission":
                    self._calculate_transmission(name)
                    self._scale_group(name, self.aux_ws["depolarized_transmission"].ads_name)
                    self._check_spin_states(name)
        _clean_temporary_ws()
        return names

    def _load_workspace_from_path(self, key_list: Union[str, list[str]]):
        """
        Loads a workspace into the ads from a path in aux_ws dict paths if there is a path and the workspace is not already on the ads.
        """
        for key in _str_to_list(key_list):
            info = self.aux_ws[key]
            ads_name, path = info.ads_name, info.path
            if not ADS.doesExist(ads_name):
                if not path:
                    # load can be quite slow if it's looking for a file even with empty paths, we raise error early for such a case.
                    raise RuntimeError(f"Missing path for file or ws with name: {ads_name}")
                try:
                    ws = self.load(path, load_quiet=True)
                    _add_or_replace(ads_name, ws)
                except RuntimeError:
                    raise RuntimeError(f"Error in loading workspace, {ads_name} was not found in ADS or in filesystem at: {path}")

    def _save_and_clean_results(self, scattering_list: Union[str, list[str]] = None, delete_ads: bool = True):
        """
        Selects which workspaces to save. If it is correction, we prepare the workspaces for correction and save those.
        Additionally, we delete result workspaces if that is chosen in the properties.
        """
        scat_out_names = []
        prefixes = ["transmission", *self.eff_basenames, *AUX_WS_BASENAMES_CALIBRATION]

        if scattering_list:
            prefixes.extend(["scattering", *AUX_WS_BASENAMES_CORRECTION])
            scat_out_names = self._prepare_corrected_data_for_save(scattering_list)

        red_names = self._filter_ads_names(prefixes)
        self._save_results(scat_out_names if scattering_list else red_names, scattering_list is not None)
        _clean_temporary_ws(additional_names=red_names + scat_out_names if delete_ads else None)

    def _average_group(self, ws_name: str = TMP_NAME):
        """
        Calculates the average transmission across all periods in a workspace group using a weighted mean.
        """
        if not isinstance(group_in := ADS.retrieve(ws_name), WorkspaceGroup):
            return

        temp_name = TMP_NAME + ADS.unique_hidden_name()
        self._load_child_algorithm(ALGS.clone, InputWorkspace=group_in, OutputWorkspace=temp_name)
        group = ADS.retrieve(temp_name)

        mean = group.getItem(0)
        for index in range(1, group.getNumberOfEntries()):
            mean = self._load_child_algorithm(ALGS.mean_w, OUT_WS, InputWorkspace1=mean, InputWorkspace2=group.getItem(index))

        self._load_child_algorithm(ALGS.delete, Workspace=temp_name)
        _add_or_replace(ws_name, mean)

    def _load_child_algorithm(self, name: str, output: str = None, **kwargs) -> Union[None, str, "MatrixWorkspace"]:
        """
        Loading child algorithms given by a name and kwargs. If the output string is set, we retrieve
        the output property with the name in `output`.
        """
        try:
            alg = self.createChildAlgorithm(name, **kwargs)
            alg.setAlwaysStoreInADS(not output)
            alg.execute()
        except Exception as e:
            raise RuntimeError(f"Error in execution of child algorithm {name} : {str(e)}")

        return alg.getProperty(output).value if output else None


AlgorithmFactory.subscribe(SANSISISPolarizationCorrections)
