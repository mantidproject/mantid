# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import datetime
import json
import os
import platform
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Union, Tuple, TypeAlias

import matplotlib.pyplot as plt
from matplotlib.axes import Axes
import numpy as np
from mantid.api import Workspace
from mantid.api import AnalysisDataService as ADS
from mantid.geometry import CrystalStructure, ReflectionConditionFilter, ReflectionGenerator
from mantid.simpleapi import (
    CreateEmptyTableWorkspace,
    CreateSampleWorkspace,
    CreateWorkspace,
    DeleteWorkspace,
    Load,
    LoadCIF,
    LoadGSS,
    logger,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.gsas2_handler import (
    GSAS2Handler,
    SaveDirectories,
    RefinementSettings,
    FilePaths,
    GSAS2Config,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import (
    SampleLogsGroupWorkspace,
    _generate_workspace_name,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.workspace_record import (
    FittingWorkspaceRecordContainer,
)


@dataclass
class GSAS2ModelConfig:
    """
    Configuration dataclass for the GSAS-II model, including limits and timeout settings.

    Attributes:
        limits: A list of limits for the configuration.
        timeout: Maximum time (in seconds) to wait for GSAS-II subprocesses.
    """

    path_to_gsas2: str = ""
    timeout: int = 100


@dataclass
class XLimitsState:
    """
    Represents x-axis limits for the GSAS-II model.

    Attributes:
        x_min: User-inputted minimum x-values for data regions.
        x_max: User-inputted maximum x-values for data regions.
        data_x_min: Minimum x-values determined from the loaded data.
        data_x_max: Maximum x-values determined from the loaded data.
        limits: Limits for each region as [min, max] pairs.
    """

    x_min: List[float] = field(default_factory=list)
    x_max: List[float] = field(default_factory=list)
    data_x_min: Optional[List[float]] = None
    data_x_max: Optional[List[float]] = None
    limits: Optional[List[List[float]]] = field(default_factory=list)


@dataclass
class RefinementState:
    """
    Represents refinement-related attributes for the GSAS-II model.

    Attributes:
        override_cell_length_string: String to override cell length values.
        override_cell_lengths: List of cell length overrides.
        mantid_pawley_reflections: Pawley reflections data.
        crystal_structures: List of crystal structure objects.
        chosen_cell_lengths: Chosen cell lengths for the phases.
    """

    override_cell_length_string: Optional[str] = None
    override_cell_lengths: Optional[List[List[float]]] = None
    mantid_pawley_reflections: Optional[List[Union[str, int]]] = None
    crystal_structures: List[CrystalStructure] = field(default_factory=list)
    chosen_cell_lengths: Optional[List[List[float]]] = None


@dataclass
class OutputState:
    """
    Represents runtime outputs for the GSAS-II model.

    Attributes:
        out_call_gsas2: Standard output from a GSAS-II call.
        err_call_gsas2: Standard error from a GSAS-II call.
        phase_names_list: List of phase names.
    """

    out_call_gsas2: Optional[str] = None
    err_call_gsas2: Optional[str] = None
    phase_names_list: List[str] = field(default_factory=list)


@dataclass
class GSAS2RuntimeState:
    """
    Represents the high-level runtime state of the GSAS-II model for engineering diffraction.

    Attributes:
        number_of_regions: Number of regions in the model.
        number_histograms: Number of histograms in the model.
        d_spacing_min: The minimum d-spacing value.
    """

    number_of_regions: int = 0
    number_histograms: int = 0
    d_spacing_min: float = 1.0


class GSAS2Model:
    """
    GSAS2Model is a class that provides an interface for managing and executing GSAS-II refinements.
    It handles the preparation of input parameters, validation, execution of GSAS-II subprocesses,
    and processing of output results. The class is designed to integrate with Mantid's Engineering
    Diffraction interface.
    """

    ReflectionType: TypeAlias = Tuple[List[int], float, int]

    def __init__(self) -> None:
        # Configuration and State
        self.config = GSAS2ModelConfig()
        self.state = GSAS2RuntimeState()
        self.file_paths = FilePaths()
        self.x_limits = XLimitsState()
        self.refinement_state = RefinementState()
        self.output_state = OutputState()

        self.save_directories = SaveDirectories(temporary_save_directory="", project_name="")

        # Refinement settings
        self.refinement_method: str = "Pawley"
        self.dSpacing_min: float = 1.0
        self.refine_microstrain: bool = False
        self.refine_sigma_one: bool = True
        self.refine_gamma: bool = True
        self.refine_background: bool = True
        self.refine_unit_cell: bool = True
        self.refine_histogram_scale_factor: bool = True

        # Workspace management
        self._data_workspaces: FittingWorkspaceRecordContainer = FittingWorkspaceRecordContainer()
        self._suffix: str = "_GSASII"
        self._sample_logs_workspace_group: SampleLogsGroupWorkspace = SampleLogsGroupWorkspace(self._suffix)
        self.output_state.phase_names_list = []
        self.refinement_state.crystal_structures = []
        self.user_save_directory: Optional[str] = None

    def clear_input_components(self) -> None:
        """
        Clears all input components and resets the configuration, state, and refinement settings
        to their default values.
        """
        # Reset configuration and state to their default values
        self.config = GSAS2ModelConfig()
        self.state = GSAS2RuntimeState()

        # Reset refinement settings
        refinement_defaults = {
            "dSpacing_min": 1.0,
            "refine_microstrain": False,
            "refine_sigma_one": False,
            "refine_gamma": False,
            "refine_background": False,
            "refine_unit_cell": False,
            "refine_histogram_scale_factor": False,
        }
        for attr, default in refinement_defaults.items():
            setattr(self, attr, default)

    def run_model(
        self,
        load_parameters: list,
        refinement_parameters: list,
        project_name: str,
        rb_num: Optional[str] = None,
        user_x_limits: Optional[List[List[float]]] = None,
    ) -> Optional[Dict[str, int]]:
        """
        Returns a dictionary mapping data file names to their result counts
        """
        data_files = load_parameters[2]  # Extract data files list
        num_hist = None

        for data_file in data_files:
            # Create unique project name for each file
            file_basename = os.path.splitext(os.path.basename(data_file))[0]
            individual_project_name = f"{project_name}_{file_basename}"

            # Create modified load_parameters for single file
            single_file_load_params = [
                load_parameters[0],  # instrument_files (reuse same)
                load_parameters[1],  # phase_filepaths (reuse same)
                [data_file],  # single data file
            ]

            num_hist = self._run_single_refinement(
                single_file_load_params, refinement_parameters, individual_project_name, rb_num, user_x_limits
            )

            if not num_hist:
                return

        return num_hist

    def _run_single_refinement(
        self,
        load_parameters: list,
        refinement_parameters: list,
        project_name: str,
        rb_num: Optional[str] = None,
        user_x_limits: Optional[List[List[float]]] = None,
    ) -> Optional[int]:
        self.clear_input_components()
        if not self.initial_validation(project_name, load_parameters):
            return None
        self.set_components_from_inputs(load_parameters, refinement_parameters, project_name, rb_num)
        self.read_phase_files()
        self.generate_reflections_from_space_group()

        formatted_limits: Optional[List[List[float]]] = None
        # Ensure both elements are lists of floats and pass formatted limits to validate_x_limits
        if isinstance(user_x_limits, list) and len(user_x_limits) == 2:
            formatted_limits = [
                user_x_limits[0] if isinstance(user_x_limits[0], list) else [user_x_limits[0]],
                user_x_limits[1] if isinstance(user_x_limits[1], list) else [user_x_limits[1]],
            ]

        if not self.validate_x_limits(formatted_limits):
            return None
        if not self.further_validation():
            return None

        runtime = self.call_gsas2()
        if not runtime:
            return None
        runtime_str = runtime[1]  # Extract the string part of the runtime tuple
        report_result = self.report_on_outputs(runtime_str)
        if report_result is not None:
            gsas_result_filepath, _ = report_result  # Unpack the tuple
        else:
            logger.error("Failed to unpack the result from report_on_outputs.")
            return None
        gsas_result = gsas_result_filepath
        if not gsas_result:
            return None
        self.load_basic_outputs(gsas_result)

        return self.state.number_of_regions

    # ===============
    # Prepare Inputs
    # ===============

    def set_components_from_inputs(self, load_params: list, refinement_params: list, project: str, rb_number: Optional[str] = None) -> None:
        self.config.path_to_gsas2 = output_settings.get_path_to_gsas2()
        self.save_directories.project_name = project
        self.organize_save_directories(rb_number)

        self.file_paths.instrument_files = load_params[0]
        self.file_paths.phase_filepaths = load_params[1]
        self.file_paths.data_files = load_params[2]

        self.refinement_method = refinement_params[0]
        self.refinement_state.override_cell_length_string = refinement_params[1]
        self.refine_microstrain = refinement_params[2]
        self.refine_sigma_one = refinement_params[3]
        self.refine_gamma = refinement_params[4]

        # Currently hardcoded
        self.refine_background = True
        self.refine_unit_cell = True
        self.refine_histogram_scale_factor = True  # True by default

    # ===========
    # Validation
    # ===========

    def initial_validation(self, project: str, load_params: list) -> Optional[bool]:
        if not project:
            logger.error("* There must be a valid Project Name for GSASII refinement")
            return None
        if not load_params[0] or not load_params[1] or not load_params[2]:
            logger.error("* All filepaths for Instrument, Phase and Focused Data must be valid")
            return None
        return True

    def further_validation(self) -> bool:
        if not self.x_limits.limits:
            return False  # any errors logged in validate_x_limits()
        if self.refinement_method == "Pawley" and not self.refinement_state.mantid_pawley_reflections:
            logger.error("No Pawley Reflections were generated for the phases provided. Not calling GSAS-II.")
            return False
        if not self.config.path_to_gsas2:
            logger.error("The Path to GSAS2 setting is empty. Please provide a valid path in Engineering Diffraction Settings")
            return False
        return True

    # ===============
    # Calling GSASII
    # ===============
    def call_gsas2(self) -> Optional[Tuple[str, str]]:
        """
        Prepares and executes the GSAS-II subprocess call.
        """
        self._validate_required_attributes()

        save_directories = self._create_save_directories()
        refinement_settings = self._create_refinement_settings()
        config = self._create_gsas2_config()

        gsas2_handler = self._initialize_gsas2_handler(save_directories, refinement_settings, self.file_paths, config)
        call_g2sc_path = self._locate_call_g2sc_script(gsas2_handler)
        serialized_inputs = gsas2_handler.to_json()
        gsasii_call_args = self._construct_gsasii_call(gsas2_handler, call_g2sc_path, serialized_inputs)

        return self.call_subprocess(gsasii_call_args, gsas2_handler.python_binaries)

    def _validate_required_attributes(self) -> None:
        if not self.config.path_to_gsas2.strip():
            raise ValueError("Path to GSAS-II is not set. Please provide a valid path.")
        if not self.save_directories.project_name:
            raise ValueError("Project name is not set. Please provide a valid project name.")

    def _create_save_directories(self) -> SaveDirectories:
        self._validate_required_attributes()
        return SaveDirectories(
            temporary_save_directory=self.save_directories.temporary_save_directory,
            project_name=self.save_directories.project_name,
        )

    def _create_refinement_settings(self) -> RefinementSettings:
        return RefinementSettings(
            method=self.refinement_method,
            background=self.refine_background,
            microstrain=self.refine_microstrain,
            sigma_one=self.refine_sigma_one,
            gamma=self.refine_gamma,
            histogram_scale_factor=self.refine_histogram_scale_factor,
            unit_cell=self.refine_unit_cell,
        )

    def _create_gsas2_config(self) -> GSAS2Config:
        return GSAS2Config(
            limits=self.x_limits.limits,
            mantid_pawley_reflections=self.refinement_state.mantid_pawley_reflections,
            override_cell_lengths=self.get_override_lattice_parameters(),
            d_spacing_min=self.dSpacing_min,
            number_of_regions=self.state.number_of_regions,
        )

    def _initialize_gsas2_handler(
        self, save_directories: SaveDirectories, refinement_settings: RefinementSettings, file_paths: FilePaths, config: GSAS2Config
    ) -> GSAS2Handler:
        gsas2_handler = GSAS2Handler(
            path_to_gsas2=self.config.path_to_gsas2,
            save_directories=save_directories,
            refinement_settings=refinement_settings,
            file_paths=file_paths,
            config=config,
        )

        gsas2_handler.set_gsas2_python_path()
        gsas2_handler.set_binaries()

        return gsas2_handler

    def _locate_call_g2sc_script(self, gsas2_handler: GSAS2Handler) -> Path:
        call_g2sc_path = next(
            gsas2_handler.limited_rglob(Path(__file__).parent, "call_G2sc.py", max_depth=1),
            None,
        )
        if not call_g2sc_path:
            raise FileNotFoundError("The file 'call_G2sc.py' could not be found in the expected directory.")
        return call_g2sc_path

    def _construct_gsasii_call(self, gsas2_handler: GSAS2Handler, call_g2sc_path: Path, serialized_inputs: str) -> List[str]:
        if not gsas2_handler.gsas2_python_path:
            gsas2_handler.set_gsas2_python_path()

        return [
            str(gsas2_handler.gsas2_python_path),
            str(call_g2sc_path),
            serialized_inputs,
        ]

    def _prepare_gsas2_environment(self, gsas_binary_paths: List[str], serialized_inputs: str) -> dict:
        """
        Prepares the environment variables for the GSAS-II subprocess call.
        """
        env = os.environ.copy()
        env["PYTHONHOME"] = self.config.path_to_gsas2
        env["PATH"] = ";".join(gsas_binary_paths + [env["PATH"]])
        gsasii_scriptable_path = json.loads(serialized_inputs).get("gsasii_scriptable_path")
        if gsasii_scriptable_path:
            gsasii_dir = Path(gsasii_scriptable_path).parent
            env["PYTHONPATH"] = str(gsasii_dir.parent) if platform.system() == "Windows" else str(gsasii_dir)
        return env

    def call_subprocess(self, command_string_list: List[str], gsas_binary_paths: List[str]) -> Optional[Tuple[str, str]]:
        """
        Notes:
        1. **Binary Search Order**:
            - The GSAS-II binaries are searched in the GSASII directory before the Mantid conda environment.
            - Ensure that the GSASII conda environment is activated by calling the "activate" script for proper functionality.

        2. **PyCharm Debugger Behavior**:
            - The PyCharm debugger attempts to debug into any Python subprocesses spawned by Workbench.
            - On Windows (see `pydev_monkey.py`), this results in the command line arguments being manipulated,
            causing the GSASII JSON parameter string to get corrupted.
            - To avoid this, pass the GSASII Python executable via the `executable` parameter instead of `argv[0]`,
            and set `argv[0]` to something that does not contain the string 'python'.

        3. **Disabling PyCharm Subprocess Debugging**:
            - The PyCharm behavior can also be disabled by unchecking the property in File > Settings:
            "Attach to subprocess automatically while debugging".
        """
        shell_process = None  # Initialize shell_process to None to avoid unbound local error if exception occurs before try block
        try:
            env = self._prepare_gsas2_environment(gsas_binary_paths, command_string_list[2])
            empty_argv0 = "_"
            shell_process = subprocess.Popen(
                [empty_argv0] + command_string_list[1:],
                executable=command_string_list[0],
                shell=False,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                close_fds=True,
                universal_newlines=True,
                env=env,
            )

            shell_output = shell_process.communicate(timeout=self.config.timeout)

            if shell_process.returncode != 0:
                logger.error(f"GSAS-II call failed with error: {shell_output[-1]}")

                return None
            return shell_output
        except subprocess.TimeoutExpired:
            if shell_process:  # Check if shell_process is not None
                shell_process.terminate()
            logger.error(
                f"GSAS-II call did not complete after {self.config.timeout} seconds, so it was"
                f" aborted. Check the inputs, such as Refinement Method are correct. The"
                f" timeout interval can be increased in the Engineering Diffraction Settings."
            )
            return None
        except (FileNotFoundError, ValueError, subprocess.SubprocessError) as exc:
            logger.error(
                f"GSAS-II call failed with error: {str(exc)}. "
                "Please check the Path to GSASII in the Engineering Diffraction Settings is valid. "
                "Path must be outermost directory of GSAS-II installation."
            )
            return None

    def format_shell_output(self, title: str, shell_output_string: str) -> str:
        double_line: str = "-" * (len(title) + 2) + "\n" + "-" * (len(title) + 2)
        return "\n" * 3 + double_line + "\n " + title + " \n" + double_line + "\n" + shell_output_string + "\n" + double_line + "\n" * 3

    # ===========
    # Read Files
    # ===========

    def find_in_file(
        self, file_path: str, marker_string: str, start_of_value: str, end_of_value: str, strip_separator: Optional[str] = None
    ) -> Optional[str]:
        value_string: Optional[str] = None
        if os.path.exists(file_path):
            with open(file_path, "rt", encoding="utf-8") as file:
                full_file_string: str = file.read().replace("\n", "")
                where_marker: int = full_file_string.rfind(marker_string)
                if where_marker != -1:
                    where_value_start: int = full_file_string.find(start_of_value, where_marker)
                    if where_value_start != -1:
                        where_value_end: int = full_file_string.find(end_of_value, where_value_start + 1)
                        value_string = full_file_string[where_value_start:where_value_end]
                        if strip_separator:
                            value_string = value_string.strip(strip_separator + " ")
                        else:
                            value_string = value_string.strip(" ")
        return value_string

    def get_crystal_params_from_instrument(self, instrument: str) -> Optional[List[float]]:
        crystal_params: List[Optional[str]] = []
        if not self.state.number_of_regions:
            crystal_params = [self.find_in_file(instrument, "ICONS", "S", "INS", strip_separator="ICONS\t")]

        else:
            for region_index in range(1, self.state.number_of_regions + 1):
                loop_crystal_params = self.find_in_file(instrument, f"{region_index} ICONS", "S", "INS", strip_separator="ICONS\t")
                crystal_params.append(loop_crystal_params)
        tof_min: List[float] = []
        file_mismatch: str = f"Different number of banks in {instrument} and {self.file_paths.data_files}"
        for loop_crystal_param_string in crystal_params:
            if not loop_crystal_param_string:
                logger.error(file_mismatch)
                return None
            list_crystal_params: List[str] = loop_crystal_param_string.split(" ")
            if len(list_crystal_params) == 1:
                list_crystal_params = list_crystal_params[0].split("\t")
            list_crystal_params = list(filter(None, list_crystal_params))
            if len(list_crystal_params):
                dif_c: float = float(list_crystal_params[0] or 0.0)
                dif_a: float = float(list_crystal_params[1] or 0.0)
                t_zero: float = float(list_crystal_params[2] or 0.0)
                tof_min.append((dif_c * self.dSpacing_min) + (dif_a * (self.dSpacing_min**2)) + t_zero)
        if not tof_min:
            logger.error(file_mismatch)
            return None
        return tof_min

    # ===================
    # Pawley Reflections
    # ===================

    def create_pawley_reflections(self, crystal_structure: CrystalStructure) -> List[Tuple[List[int], float, int]]:
        generator = ReflectionGenerator(crystal_structure)
        hkls: List[List[int]] = generator.getUniqueHKLsUsingFilter(self.dSpacing_min, 4.2, ReflectionConditionFilter.StructureFactor)
        dValues: List[float] = generator.getDValues(hkls)
        pg = crystal_structure.getSpaceGroup().getPointGroup()
        # Make list of tuples and sort by d-values, descending, include point group for multiplicity.
        generated_reflections: List[Tuple[List[int], float, int]] = sorted(
            [(list(hkl), d, len(pg.getEquivalents(hkl))) for hkl, d in zip(hkls, dValues)],
            key=lambda x: x[1] - x[0][0] * 1e-6 if isinstance(x[0], list) and len(x[0]) > 0 else x[1],
            reverse=True,
        )
        return generated_reflections

    def read_phase_files(self) -> None:
        self.refinement_state.crystal_structures.clear()
        for phase_filepath in self.file_paths.phase_filepaths:
            self.refinement_state.crystal_structures.append(self._read_cif_file(phase_filepath))
        if self.refinement_state.override_cell_length_string:
            # override lattice parameters of first file
            self.refinement_state.crystal_structures[0] = self.set_lattice_params_from_user_input(
                self.refinement_state.crystal_structures[0]
            )

    def get_override_lattice_parameters(self) -> Optional[List[List[float]]]:
        if self.refinement_state.override_cell_length_string:
            return [self._get_lattice_parameters_from_crystal_structure(xtal) for xtal in self.refinement_state.crystal_structures]
        else:
            return None

    def _get_lattice_parameters_from_crystal_structure(self, crystal_structure: CrystalStructure) -> List[float]:
        return [getattr(crystal_structure.getUnitCell(), method)() for method in ("a", "b", "c", "alpha", "beta", "gamma")]

    def _read_cif_file(self, phase_filepath: str) -> CrystalStructure:
        ws = CreateSampleWorkspace(StoreInADS=False)
        LoadCIF(ws, phase_filepath, StoreInADS=False)  # error if not StoreInADS=False even though no output
        return ws.sample().getCrystalStructure()

    def set_lattice_params_from_user_input(self, input_crystal_structure: CrystalStructure) -> CrystalStructure:
        # User can delimit with commas or whitespace
        assert isinstance(self.refinement_state.override_cell_length_string, str), "override_cell_length_string should be a string"
        user_alatt: List[str] = list(self.refinement_state.override_cell_length_string.replace(",", " ").split())
        if len(user_alatt) == 1:
            user_alatt = 3 * user_alatt  # Assume cubic lattice

        updated_crystal_structure: CrystalStructure = CrystalStructure(
            " ".join(user_alatt),
            input_crystal_structure.getSpaceGroup().getHMSymbol(),
            ";".join(input_crystal_structure.getScatterers()),
        )
        return updated_crystal_structure

    def generate_reflections_from_space_group(self) -> None:
        self.refinement_state.mantid_pawley_reflections = []
        assert isinstance(self.refinement_state.crystal_structures, list), "crystal_structures should be a list"
        for crystal_structure in self.refinement_state.crystal_structures:
            self.refinement_state.mantid_pawley_reflections.append(self.create_pawley_reflections(crystal_structure))
        return

    # =========
    # X Limits
    # =========

    def get_no_banks(self, prm_file):
        with open(prm_file) as f:
            for line in f:
                if "BANK" in line and len(line.split()) == 3:
                    return int(line.split()[-1])

        return -1

    def understand_data_structure(self) -> None:
        if len(self.file_paths.instrument_files) != 1:
            logger.error("* You must provide exactly one instrument file.")
            return False

        self.x_limits.data_x_min = []
        self.x_limits.data_x_max = []
        number_of_regions = 0
        banks_per_file = []  # Track banks per file for validation

        for input_file in self.file_paths.data_files:
            loop_focused_workspace = LoadGSS(Filename=input_file, OutputWorkspace="GSASII_input_data", EnableLogging=False)
            file_bank_count = loop_focused_workspace.getNumberHistograms()
            banks_per_file.append(file_bank_count)

            no_banks = self.get_no_banks(self.file_paths.instrument_files[0])

            if file_bank_count != no_banks:
                logger.error("* All data files should have the same number of banks as the instrument file.")
                return False

            for workspace_index in range(file_bank_count):
                self.x_limits.data_x_min.append(loop_focused_workspace.readX(workspace_index)[0])
                self.x_limits.data_x_max.append(loop_focused_workspace.readX(workspace_index)[-1])
                number_of_regions += 1
            DeleteWorkspace(loop_focused_workspace)

        expected_total_regions = len(self.file_paths.data_files) * banks_per_file[0]
        if number_of_regions != expected_total_regions:
            logger.error(f"* Expected {expected_total_regions} total regions, but found {number_of_regions}.")
            return False

        self.state.number_of_regions = number_of_regions
        return True

    def validate_x_limits(self, users_limits: Optional[List[List[float]]]) -> bool:
        if not self.understand_data_structure():
            return False
        if users_limits:
            if len(users_limits[0]) != self.state.number_of_regions:
                users_limits[0] *= self.state.number_of_regions
                users_limits[1] *= self.state.number_of_regions
        self.state.number_histograms = len(self.file_paths.data_files)
        if users_limits:
            self.x_limits.x_min = [float(k) for k in users_limits[0]]
            self.x_limits.x_max = [float(k) for k in users_limits[1]]
            if len(self.x_limits.x_min) != self.state.number_histograms:
                if len(self.x_limits.x_min) == 1:
                    self.x_limits.x_min = self.x_limits.x_min * self.state.number_histograms
                    self.x_limits.x_max = self.x_limits.x_max * self.state.number_histograms
        else:
            self.x_limits.x_min = self.x_limits.data_x_min if self.x_limits.data_x_min is not None else []
            self.x_limits.x_max = self.x_limits.data_x_max if self.x_limits.data_x_max is not None else []

        self.x_limits.limits = [self.x_limits.x_min, self.x_limits.x_max]

        return True

    # ===============
    # Handle Outputs
    # ===============

    def read_gsas_lst_and_print_wR(self, result_filepath: str, histogram_data_files: List[str], test: bool = False) -> Optional[str]:
        with open(result_filepath, "rt", encoding="utf-8") as file:
            result_string = file.read().replace("\n", "")
            for loop_histogram in histogram_data_files:
                where_loop_histogram = result_string.rfind(loop_histogram)
                if where_loop_histogram != -1:
                    where_loop_histogram_wR = result_string.find("Final refinement wR =", where_loop_histogram)
                    if where_loop_histogram_wR != -1:
                        where_loop_histogram_wR_end = result_string.find("%", where_loop_histogram_wR)
                        logger.notice(loop_histogram)
                        logged_lst_result = result_string[where_loop_histogram_wR : where_loop_histogram_wR_end + 1]
                        logger.notice(logged_lst_result)
                        if test:
                            return logged_lst_result
        return None

    def find_phase_names_in_lst(self, file_path: str) -> List[str]:
        phase_names: List[str] = []
        marker_string: str = "Phase name"
        start_of_value: str = ":"
        end_of_value: str = "="
        strip_separator: str = ":"
        with open(file_path, "rt", encoding="utf-8") as file:
            full_file_string: str = file.read().replace("\n", "")
            where_marker: int = full_file_string.find(marker_string)
            while where_marker != -1:
                where_value_start: int = full_file_string.find(start_of_value, where_marker)
                if where_value_start != -1:
                    where_value_end: int = full_file_string.find(end_of_value, where_value_start + 1)
                    phase_names.append(full_file_string[where_value_start:where_value_end].strip(strip_separator + " "))
                    where_marker = full_file_string.find(marker_string, where_value_end)
                else:
                    where_marker = -1
        return phase_names

    def report_on_outputs(self, runtime: str, test: bool = False) -> Optional[Tuple[str, str]]:
        """
        Reports on the outputs of the GSAS-II call.

        Args:
            runtime: The runtime of the GSAS-II call.
            test: If True, return additional test information.

        Returns:
            A tuple containing the result file path and a success message.
        """
        gsas_project_filepath = self.check_for_output_file(".gpx", "project")
        gsas_result_filepath = self.check_for_output_file(".lst", "result")
        if not gsas_project_filepath or not gsas_result_filepath:
            return None
        logged_success = f"\nGSAS-II call complete in {runtime} seconds.\n"
        logger.notice(logged_success)
        if test:
            return gsas_result_filepath, logged_success
        return gsas_result_filepath, logged_success

    def check_for_output_file(self, file_extension: str, file_descriptor: str, test: bool = False) -> Optional[Union[str, None]]:
        gsas_output_filename = self.save_directories.project_name + file_extension
        if gsas_output_filename not in os.listdir(self.save_directories.temporary_save_directory):
            logged_failure = (
                f"GSAS-II call must have failed, as the output {file_descriptor} file was not found."
                + self.format_shell_output(title="Errors from GSAS-II", shell_output_string=self.output_state.err_call_gsas2 or "")
            )
            logger.error(logged_failure)
            if test:
                return logged_failure
            return None
        return os.path.join(self.save_directories.temporary_save_directory, gsas_output_filename)

    def organize_save_directories(self, rb_num_string: Optional[str]) -> None:
        save_dir: str = os.path.join(output_settings.get_output_path())
        self.file_paths.gsas2_save_dirs = [os.path.join(save_dir, "GSAS2", "")]
        save_directory: str = self.file_paths.gsas2_save_dirs[0]
        if rb_num_string:
            self.file_paths.gsas2_save_dirs.append(
                os.path.join(save_dir, "User", rb_num_string, "GSAS2", self.save_directories.project_name, "")
            )
            # TODO: Once texture is supported, pass calibration observer like currently done for focus tab
            # if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            #     calib_dirs.pop(0)  # only save to RB directory to limit number files saved
        self.user_save_directory = os.path.join(save_directory, self.save_directories.project_name)
        self.save_directories.temporary_save_directory = os.path.join(
            save_directory, datetime.datetime.now().strftime("tmp_EngDiff_GSASII_%Y-%m-%d_%H-%M-%S")
        )
        os.makedirs(self.save_directories.temporary_save_directory)

    def move_output_files_to_user_save_location(self) -> str:
        for new_directory in self.file_paths.gsas2_save_dirs:
            os.makedirs(new_directory, exist_ok=True)

        save_success_message: str = f"\n\nOutput GSAS-II files saved in {self.user_save_directory}"

        exist_extra_save_dirs: bool = False
        if len(self.config.path_to_gsas2) > 1:
            exist_extra_save_dirs = True
            self.file_paths.gsas2_save_dirs.pop(0)

        if self.user_save_directory and os.path.exists(self.user_save_directory):
            shutil.rmtree(self.user_save_directory)
        if self.user_save_directory is not None:
            os.makedirs(self.user_save_directory, exist_ok=True)
        else:
            raise ValueError("user_save_directory is None. Cannot create directory.")
        for output_file_index, output_file in enumerate(os.listdir(self.save_directories.temporary_save_directory)):
            os.rename(
                os.path.join(self.save_directories.temporary_save_directory, output_file),
                os.path.join(self.user_save_directory, output_file),
            )
            if exist_extra_save_dirs:
                for extra_save_dir in self.file_paths.gsas2_save_dirs:
                    shutil.copy(os.path.join(self.user_save_directory, output_file), os.path.join(extra_save_dir, output_file))
                    if output_file_index == 0:
                        save_success_message += f" and in {extra_save_dir}"
        os.rmdir(self.save_directories.temporary_save_directory)
        return save_success_message

    # =============
    # Load Outputs
    # =============

    def load_basic_outputs(self, gsas_result_filepath: str) -> None:
        logger.notice(f"GSAS-II .lst result file found. Opening {self.save_directories.project_name}.lst")
        self.read_gsas_lst_and_print_wR(gsas_result_filepath, self.file_paths.data_files)
        save_message = self.move_output_files_to_user_save_location()
        if self.user_save_directory is None:
            raise ValueError("user_save_directory is None. Cannot construct the file path.")
        self.output_state.phase_names_list = self.find_phase_names_in_lst(
            os.path.join(self.user_save_directory, self.save_directories.project_name + ".lst")
        )
        logger.notice(save_message)

        self.output_state.phase_names_list: List[str] = self.find_phase_names_in_lst(
            os.path.join(self.user_save_directory, self.save_directories.project_name + ".lst")
        )
        self.create_lattice_parameter_table()
        self.create_instrument_parameter_table()
        self.create_reflections_table()

    def load_result_for_plot(self, index_histograms: int) -> Tuple[Optional[object], Optional[List[np.ndarray]]]:
        workspace = self.load_gsas_histogram(index_histograms)
        reflections: Optional[List[np.ndarray]] = None
        if self.refinement_method == "Pawley":
            reflections = self.load_gsas_reflections_per_histogram_for_plot(index_histograms)
        return workspace, reflections

    def chop_to_limits(self, input_array: np.ndarray, x: np.ndarray, min_x: float, max_x: float) -> np.ndarray:
        input_array[x <= min_x] = np.nan
        input_array[x >= max_x] = np.nan
        return input_array

    def load_gsas_histogram(self, histogram_index: int) -> object:
        if not self.user_save_directory:
            raise ValueError("user_save_directory is None. Cannot construct the file path.")
        result_csv = os.path.join(self.user_save_directory, self.save_directories.project_name + f"_{histogram_index}.csv")
        my_data = np.transpose(np.genfromtxt(result_csv, delimiter=",", skip_header=39))
        # x  y_obs	weight	y_calc	y_bkg	Q
        x_values = my_data[0]
        y_obs = self.chop_to_limits(
            np.array(my_data[1]), x_values, self.x_limits.x_min[histogram_index - 1], self.x_limits.x_max[histogram_index - 1]
        )
        y_calc = self.chop_to_limits(
            np.array(my_data[3]), x_values, self.x_limits.x_min[histogram_index - 1], self.x_limits.x_max[histogram_index - 1]
        )
        y_diff = y_obs - y_calc
        y_diff -= np.max(np.ma.masked_invalid(y_diff))
        y_bkg = self.chop_to_limits(
            np.array(my_data[4]), x_values, self.x_limits.x_min[histogram_index - 1], self.x_limits.x_max[histogram_index - 1]
        )
        y_data = np.concatenate((y_obs, y_calc, y_diff, y_bkg))

        gsas_histogram = CreateWorkspace(
            OutputWorkspace=f"gsas_histogram_{histogram_index}", DataX=np.tile(my_data[0], 4), DataY=y_data, NSpec=4, EnableLogging=False
        )
        return gsas_histogram

    def load_gsas_reflections_per_histogram_for_plot(self, histogram_index: int) -> List[np.ndarray]:
        loaded_reflections: List[np.ndarray] = []
        for phase_name in self.output_state.phase_names_list:
            if not self.user_save_directory:
                raise ValueError("user_save_directory is None. Cannot construct the file path.")
            result_reflections_txt = os.path.join(
                self.user_save_directory, self.save_directories.project_name + f"_reflections_{histogram_index}_{phase_name}.txt"
            )
            if os.path.exists(result_reflections_txt):
                # omit first 2 lines in file (which are histogram and phase name)
                loaded_reflections.append(np.genfromtxt(result_reflections_txt, skip_header=2))
            else:
                logger.warning(f"No reflections found for phase {phase_name} within x-limits of the fit.")
        return loaded_reflections

    def load_gsas_reflections_all_histograms_for_table(self) -> List[List[str]]:
        result_reflections_rows: List[List[str]] = []
        output_reflections_files: List[str] = self.get_txt_files_that_include("_reflections_")
        for file in output_reflections_files:
            # note delimiter="," specified so as to accept whitespace in phase name (2nd header row)
            loop_file_text: np.ndarray = np.genfromtxt(file, delimiter=",", dtype="str")
            loop_histogram_name: str = loop_file_text[0]
            loop_phase_name: str = loop_file_text[1]
            loop_reflections_text: str = ",    ".join(str(num) for num in loop_file_text[2:])
            result_reflections_rows.append([loop_histogram_name, loop_phase_name, loop_reflections_text])
        return result_reflections_rows

    def create_reflections_table(self, test: bool = False) -> Optional[Union[None, object]]:
        table_rows: List[List[str]] = self.load_gsas_reflections_all_histograms_for_table()
        if not table_rows or self.refinement_method == "Rietveld":
            # cannot generate a valid reflections workspace. If one with the same project_name exists, remove it.
            # This is to cover the case where a user runs a Pawley then Rietveld Refinement for the same project_name.
            if ADS.doesExist(f"{self.save_directories.project_name}_GSASII_reflections"):
                DeleteWorkspace(f"{self.save_directories.project_name}_GSASII_reflections", EnableLogging=False)
            return None
        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.save_directories.project_name}_GSASII_reflections", EnableLogging=False)
        table.addReadOnlyColumn("str", "Histogram name")
        table.addReadOnlyColumn("str", "Phase name")
        table.addReadOnlyColumn("str", "Reflections")
        for row in table_rows:
            table.addRow(row)
        if test:
            return table
        return None

    def get_txt_files_that_include(self, sub_string: str) -> List[str]:
        output_files: List[str] = []
        if not self.user_save_directory:
            raise ValueError("user_save_directory is None. Cannot iterate over it.")
        for _, _, filenames in os.walk(self.user_save_directory):
            for loop_filename in filenames:
                if sub_string in loop_filename and loop_filename[-4:] == ".txt":
                    output_files.append(os.path.join(self.user_save_directory, loop_filename))
        output_files.sort()
        return output_files

    def create_instrument_parameter_table(self, test: bool = False) -> Optional[Union[None, object]]:
        INST_TABLE_PARAMS: List[str] = ["Histogram name", "Sigma-1", "Gamma (Y)"]
        table = CreateEmptyTableWorkspace(
            OutputWorkspace=f"{self.save_directories.project_name}_GSASII_instrument_parameters", EnableLogging=False
        )
        table.addReadOnlyColumn("str", "Histogram name")
        table.addReadOnlyColumn("double", "Sigma-1{}".format(" (Refined)" if self.refine_sigma_one else ""))
        table.addReadOnlyColumn("double", "Gamma (Y){}".format(" (Refined)" if self.refine_gamma else ""))
        table.addReadOnlyColumn("double", "Fit X Min")
        table.addReadOnlyColumn("double", "Fit X Max")

        output_inst_param_files: List[str] = self.get_txt_files_that_include("_inst_parameters_")

        for inst_parameters_txt in output_inst_param_files:
            with open(inst_parameters_txt, "rt", encoding="utf-8") as file:
                full_file_string: str = file.read().replace("\n", "")
            inst_parameter_dict: dict = json.loads(full_file_string)
            loop_inst_parameters: List[Union[str, float]] = [inst_parameter_dict["Histogram name"]]
            for param in INST_TABLE_PARAMS[1:]:
                loop_inst_parameters.append(float(inst_parameter_dict[param][1]))

            bank_number_and_extension: str = str(list(inst_parameters_txt.split("_"))[-1])
            bank_number_from_gsas2_histogram_name: int = int(bank_number_and_extension.replace(".txt", ""))
            loop_inst_parameters.append(float(self.x_limits.x_min[bank_number_from_gsas2_histogram_name - 1]))
            loop_inst_parameters.append(float(self.x_limits.x_max[bank_number_from_gsas2_histogram_name - 1]))
            table.addRow(loop_inst_parameters)
        if test:
            return table
        return None

    def create_lattice_parameter_table(self, test: bool = False) -> Optional[Union[None, object]]:
        LATTICE_TABLE_PARAMS: List[str] = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]

        table = CreateEmptyTableWorkspace(
            OutputWorkspace=f"{self.save_directories.project_name}_GSASII_lattice_parameters", EnableLogging=False
        )
        table.addReadOnlyColumn("str", "Phase name")

        for param in LATTICE_TABLE_PARAMS:
            table.addReadOnlyColumn("double", param.split("_")[-1])
        table.addReadOnlyColumn("double", "Microstrain{}".format(" (Refined)" if self.refine_microstrain else ""))
        for phase_name in self.output_state.phase_names_list:
            if not self.user_save_directory:
                raise ValueError("user_save_directory is None. Cannot construct the file path.")
            parameters_txt: str = os.path.join(
                self.user_save_directory, self.save_directories.project_name + f"_cell_parameters_{phase_name}.txt"
            )
            if not os.path.exists(parameters_txt):
                logger.error(f"File not found: {parameters_txt}. Skipping this phase.")
                continue
            with open(parameters_txt, "rt", encoding="utf-8") as file:
                full_file_string: str = file.read().replace("\n", "")
            parameter_dict: dict = json.loads(full_file_string)
            loop_parameters: List[Union[str, float]] = [phase_name]
            for param in LATTICE_TABLE_PARAMS:
                loop_parameters.append(float(parameter_dict[param]))
            loop_parameters.append(float(parameter_dict["Microstrain"]))
            table.addRow(loop_parameters)

        if test:
            return table
        return None

    # ===========
    # Sample Logs
    # ===========

    def load_focused_nxs_for_logs(self, filenames: List[str]) -> None:
        banks_filenames = []
        for filename in filenames:
            if "all_banks" in filename:
                banks_filenames.extend([filename.replace("all_banks", "bank_1"), filename.replace("all_banks", "bank_2")])
            else:
                banks_filenames.append(filename)
        for filename in banks_filenames:
            filename = filename.replace(".gss", ".nxs")
            ws_name = _generate_workspace_name(filename, self._suffix)
            if ws_name not in self._data_workspaces.get_loaded_workpace_names():
                try:
                    ws = Load(filename, OutputWorkspace=ws_name)
                    if ws.getNumberHistograms() == 1:
                        self._data_workspaces.add(ws_name, loaded_ws=ws)
                    else:
                        logger.warning(f"Invalid number of spectra in workspace {ws_name}. Skipping loading of file.")
                except RuntimeError as e:
                    logger.error(f"Failed to load file: {filename}. Error: {e}. \n Continuing loading of other files.")
            else:
                logger.warning(f"File {ws_name} has already been loaded")
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def update_sample_log_workspace_group(self) -> None:
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def get_all_workspace_names(self) -> List[str]:
        return self._data_workspaces.get_loaded_workpace_names() + self._data_workspaces.get_bgsub_workpace_names()

    def get_log_workspaces_name(self) -> Union[List[str], str]:
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        return [ws.name() for ws in current_log_workspaces] if current_log_workspaces else ""

    def set_log_workspaces_none(self) -> None:
        # to be used in the event of Ads clear, as trying to reference the deleted grp ws results in an error
        self._sample_logs_workspace_group.clear_log_workspaces()

    # handle ADS remove. name workspace has already been deleted
    def remove_workspace(self, name: str) -> Optional[object]:
        ws_loaded = self._data_workspaces.get(name, None)
        if ws_loaded:
            bgsub_ws_name = self._data_workspaces[name].bgsub_ws_name
            removed = self._data_workspaces.pop(name).loaded_ws
            # deleting bg sub workspace will generate further remove_workspace event so ensure this is done after
            # removing record from _data_workspaces to avoid circular call
            if bgsub_ws_name:
                DeleteWorkspace(bgsub_ws_name)
            self.update_sample_log_workspace_group()
            return removed
        else:
            ws_loaded_name = self._data_workspaces.get_loaded_workspace_name_from_bgsub(name)
            if ws_loaded_name:
                removed = self._data_workspaces[ws_loaded_name].bgsub_ws
                self._data_workspaces[ws_loaded_name].bgsub_ws = None
                self._data_workspaces[ws_loaded_name].bgsub_ws_name = None
                self._data_workspaces[ws_loaded_name].bg_params = []
                return removed
        return None

    def replace_workspace(self, name, workspace) -> None:
        self._data_workspaces.replace_workspace(name, workspace)

    def update_workspace_name(self, old_name: str, new_name: str) -> None:
        if new_name not in self.get_all_workspace_names():
            self._data_workspaces.rename(old_name, new_name)
            current_log_values = self._sample_logs_workspace_group.get_log_values()
            if old_name in current_log_values:
                self._sample_logs_workspace_group.update_log_value(new_key=new_name, old_key=old_name)
        else:
            logger.warning(f"There already exists a workspace with name {new_name}.")
            self.update_sample_log_workspace_group()

    # handle ADS clear
    def clear_workspaces(self) -> None:
        self._data_workspaces.clear()
        self.set_log_workspaces_none()

    def delete_workspaces(self) -> List[object]:
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        if current_log_workspaces:
            ws_name = current_log_workspaces.name()
            self._sample_logs_workspace_group.clear_log_workspaces()
            DeleteWorkspace(ws_name)
        removed_ws_list: List[object] = []
        for ws_name in self._data_workspaces.get_loaded_workpace_names():
            removed_ws_list.extend(self.delete_workspace(ws_name))
        return removed_ws_list

    def delete_workspace(self, loaded_ws_name: str) -> List[object]:
        removed = self._data_workspaces.pop(loaded_ws_name)
        removed_ws_list: List[object] = [removed.loaded_ws]
        DeleteWorkspace(removed.loaded_ws)
        if removed.bgsub_ws:
            DeleteWorkspace(removed.bgsub_ws)
            removed_ws_list.append(removed.bgsub_ws)
            self.update_sample_log_workspace_group()
        return removed_ws_list

    # =========
    # Plotting
    # =========
    def plot_result(self, index_histograms: int, axis: Axes) -> str:
        gsas_histogram_workspace, reflections = self.load_result_for_plot(index_histograms)
        plot_window_title = self.plot_gsas_histogram(axis, gsas_histogram_workspace, reflections, index_histograms)
        return plot_window_title

    def plot_gsas_histogram(
        self, axis: Axes, gsas_histogram: Workspace, reflection_positions: Optional[List[np.ndarray]], histogram_index: int
    ) -> str:
        axis.plot(gsas_histogram, color="#1105f0", label="observed", linestyle="None", marker="+", wkspIndex=0)
        axis.plot(gsas_histogram, color="#246b01", label="calculated", wkspIndex=1)
        axis.plot(gsas_histogram, color="#09acb8", label="difference", wkspIndex=2)
        axis.plot(gsas_histogram, color="#ff0000", label="background", wkspIndex=3)
        _, y_max = axis.get_ylim()

        reflection_labels = self._create_reflection_labels(reflection_positions or [])

        for i_phase, positions in enumerate(reflection_positions or []):
            if len(positions) > 0:
                y_offset_positions = [(i_phase + 1) * -0.05 * y_max] * len(positions)
                axis.plot(
                    positions,
                    y_offset_positions,
                    label=reflection_labels[i_phase],
                    linestyle="None",
                    marker="|",
                    mew=1.5,
                    ms=8,
                )
        axis.set_xlabel("Time-of-flight ($\\mu s$)")
        axis.set_ylabel("Normalized Intensity")
        plt.show()

        input_file_name = ""
        if self.file_paths.data_files:
            if len(self.file_paths.data_files) == 1:
                input_file_name = os.path.basename(self.file_paths.data_files[0])
            else:
                input_file_name = os.path.basename(self.file_paths.data_files[histogram_index - 1])
        return "GSAS-II Refinement " + input_file_name

    def _create_reflection_labels(self, reflection_positions: List[np.ndarray]) -> List[str]:
        """Create the labels used to identify different phase reflection lists
        The primary format, if the number of phase names equals the number of positions, is 'reflections_{phase_name}'
        The fallback format is 'reflections_phase_{index}' where index starts at 1
        """
        reflection_labels = [f"reflections_{phase_name}" for phase_name in self.output_state.phase_names_list]
        if len(reflection_labels) != len(reflection_positions):
            return [f"reflections_phase_{i}" for i in range(1, len(reflection_positions) + 1)]
        return reflection_labels
