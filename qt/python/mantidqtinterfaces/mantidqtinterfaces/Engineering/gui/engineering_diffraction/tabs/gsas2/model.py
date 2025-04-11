# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import datetime
import fnmatch
import json
import os
import platform
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Union, Tuple, TypeAlias

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
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import (
    SampleLogsGroupWorkspace,
    _generate_workspace_name,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.workspace_record import (
    FittingWorkspaceRecordContainer,
)


@dataclass
class SaveDirectories:
    temporary_save_directory: str
    project_name: str


@dataclass
class RefinementSettings:
    method: str
    background: bool
    microstrain: bool
    sigma_one: bool
    gamma: bool
    histogram_scale_factor: bool
    unit_cell: bool


@dataclass
class FilePaths:
    data_files: List[str]
    phase_files: List[str]
    instrument_files: List[str]


@dataclass
class GSAS2Config:
    limits: Optional[List[List[float]]] = field(default_factory=list)
    mantid_pawley_reflections: Optional[List[List[List[Union[float, int]]]]] = None
    override_cell_lengths: Optional[List[List[float]]] = None
    d_spacing_min: float = 1.0
    number_of_regions: int = 1


@dataclass
class GSAS2ModelConfig:
    path_to_gsas2: str = ""
    project_name: Optional[str] = None
    temporary_save_directory: str = ""
    gsas2_save_dirs: Optional[List[str]] = None
    timeout: int = 10


@dataclass
class GSAS2ModelState:
    data_files: List[str] = field(default_factory=list)
    instrument_files: List[str] = field(default_factory=list)
    phase_filepaths: List[str] = field(default_factory=list)
    x_min: Optional[List[float]] = None
    x_max: Optional[List[float]] = None
    data_x_min: Optional[List[float]] = None
    data_x_max: Optional[List[float]] = None
    number_of_regions: int = 0
    number_histograms: int = 0
    limits: Optional[List[List[float]]] = None
    override_cell_length_string: Optional[str] = None
    mantid_pawley_reflections: Optional[List[List[List[float]]]] = None
    crystal_structures: Optional[List[CrystalStructure]] = None
    chosen_cell_lengths: Optional[List[List[float]]] = None
    out_call_gsas2: Optional[str] = None
    err_call_gsas2: Optional[str] = None
    phase_names_list: List[str] = field(default_factory=list)


class GSAS2Handler(object):
    """
    A class that encapsulates logic for managing GSAS-II input parameters, paths, and configurations.

    This class is responsible for:
    - Validating and organizing input parameters required for GSAS-II refinements.
    - Locating and configuring the GSAS-II Python executable and associated binaries.
    - Serializing input parameters into a JSON format for use with GSAS-II scripts.
    - Providing utility methods for searching files and directories within the GSAS-II installation.

    Attributes:
        - path_to_gsas2: Path to the GSAS-II installation directory.
        - save_directories: Directories for temporary and project-specific saves.
        - refinement_settings: Refinement options such as method and parameters.
        - file_paths: Paths to data, phase, and instrument files.
        - limits: X-axis limits for the refinement.
        - mantid_pawley_reflections: Pawley reflections data.
        - override_cell_lengths: User-specified lattice parameters.
        - d_spacing_min: Minimum d-spacing value for reflections.
        - number_of_regions: Number of regions in the refinement.
        - os_platform: Operating system platform (e.g., "Windows", "Linux").
        - python_binaries: Paths to additional binaries required for GSAS-II.

    Methods:
        - validate_inputs: Validates input parameters to ensure they meet expected criteria.
        - gsasii_scriptable_path: Finds the path to the GSASIIscriptable.py file.
        - to_json: Serializes input parameters into a JSON string.
        - set_gsas2_python_path: Configures the path to the GSAS-II Python executable.
        - set_binaries: Locates and sets additional binary paths required for GSAS-II.
        - limited_rglob: Recursively searches for files or directories matching a pattern up to a specified depth.
        - gsas2_python_path: Returns the path to the GSAS-II Python executable.

    Raises:
        - ValueError: If required parameters are missing or invalid.
        - FileNotFoundError: If required files or directories are not found.
    """

    def __init__(
        self,
        path_to_gsas2: Union[str, Path],
        save_directories: SaveDirectories,
        refinement_settings: RefinementSettings,
        file_paths: FilePaths,
        config: GSAS2Config,
    ):
        # Ensure path_to_gsas2 is always a valid Path object
        if not path_to_gsas2 or not str(path_to_gsas2).strip():
            raise ValueError("path_to_gsas2 must be provided and cannot be None or empty.")
        self.path_to_gsas2 = Path(path_to_gsas2).resolve()
        if not self.path_to_gsas2.exists() or not self.path_to_gsas2.is_dir():
            raise ValueError(f"Invalid path_to_gsas2: {self.path_to_gsas2} must be a valid directory.")

        # Store grouped parameters
        self.save_directories = save_directories
        self.refinement_settings = refinement_settings
        self.file_paths = file_paths
        self.config = config

        # GSAS-II configuration
        self._gsas2_python_path: Optional[Path] = None
        self.os_platform: Optional[str] = None
        self.python_binaries: List[str] = []

        # Validate inputs
        self.validate_inputs()

    def validate_inputs(self) -> None:
        """
        Validates the input parameters to ensure they meet the expected criteria.
        Raises a ValueError if any parameter is invalid.
        """
        for attr, name in [
            (self.file_paths.data_files, "data_files"),
            (self.file_paths.phase_files, "phase_files"),
            (self.file_paths.instrument_files, "instrument_files"),
        ]:
            if not attr:
                raise ValueError(f"Invalid {name}: must be a non-empty list.")

    @property
    def gsas2_python_path(self) -> Path:
        """
        Returns the path to the GSAS-II Python executable.
        Raises an error if the path has not been set.
        """
        if self._gsas2_python_path:
            return self._gsas2_python_path
        raise ValueError("GSAS-II Python path has not been set. Call set_gsas2_python_path() first.")

    @property
    def gsasii_scriptable_path(self) -> Path:
        """
        Finds the path to the GSASIIscriptable.py file within the GSAS-II directory.

        Returns:
            Path: The absolute path to GSASIIscriptable.py.

        Raises:
            FileNotFoundError: If GSASIIscriptable.py is not found.
        """
        scriptable_path = next(
            self.limited_rglob(self.path_to_gsas2, "GSASIIscriptable.py", max_depth=3),
            None,
        )
        if not scriptable_path:
            raise FileNotFoundError(f"GSASIIscriptable.py could not be found in the specified GSAS-II path: {self.path_to_gsas2}")
        return scriptable_path

    def set_gsas2_python_path(self) -> None:
        """
        Sets the path to the Python executable for GSAS-II based on the operating system.

        Raises:
            FileNotFoundError: If the Python executable is not found in the specified GSAS-II path.
        """
        if self.os_platform == "Windows":
            python_executable = "python.exe"
        else:
            python_executable = "python"

        # Search for the Python executable in the GSAS-II directory
        python_path = next(
            self.limited_rglob(self.path_to_gsas2, python_executable, max_depth=2),
            None,
        )

        if not python_path:
            raise FileNotFoundError(f"{python_executable} not found in the specified GSAS-II path: {self.path_to_gsas2}")

        self._gsas2_python_path = python_path

    def set_binaries(self) -> None:
        """
        Finds and sets additional binary paths required for GSAS-II.
        """
        if self.os_platform == "Windows":
            # On Windows, search for binaries in specific subdirectories
            extra_paths = [os.fspath(path) for path in self.limited_rglob(self.path_to_gsas2, "*", max_depth=1, search_for_file=False)]
            extra_paths.extend(
                os.fspath(path) for path in self.limited_rglob(self.path_to_gsas2 / "Library", "*", max_depth=2, search_for_file=False)
            )
        else:
            # On Linux/macOS, search for binaries in the "bin" directory
            extra_paths = [os.fspath(path) for path in self.limited_rglob(self.path_to_gsas2, "bin", max_depth=1, search_for_file=False)]

        self.python_binaries = extra_paths

    def limited_rglob(self, directory: Path, pattern: str, max_depth: int, search_for_file: bool = True):
        """
        Recursively search for files or directories matching the pattern in the directory up to a specified depth.

        Args:
            directory (Path): The root directory to start the search.
            pattern (str): The pattern to match files or directories.
            max_depth (int): The maximum depth to search.
            search_for_file (bool): If True, search for files; if False, search for directories.
        """
        if not directory.is_dir():
            raise FileNotFoundError(f"The provided directory path '{directory}' is not a valid directory.")
        for root, dirs, files in os.walk(directory):
            current_depth = len(Path(root).parts) - len(directory.parts)
            if current_depth > max_depth:
                dirs[:] = []
            if current_depth <= max_depth:
                for file in files if search_for_file else dirs:
                    if fnmatch.fnmatch(file, pattern):
                        yield Path(root) / file

    def to_json(self) -> str:
        """
        Converts the input parameters to a JSON string, including the GSASIIscriptable.py path.
        """
        inputs_dict = {
            "path_to_gsas2": str(self.path_to_gsas2),
            "temporary_save_directory": self.save_directories.temporary_save_directory,
            "project_name": self.save_directories.project_name,
            "refinement_settings": self.refinement_settings.__dict__,
            "file_paths": self.file_paths.__dict__,
            "limits": self.config.limits,  # Access from GSAS2Config
            "mantid_pawley_reflections": self.config.mantid_pawley_reflections,  # Access from GSAS2Config
            "override_cell_lengths": self.config.override_cell_lengths,  # Access from GSAS2Config
            "d_spacing_min": self.config.d_spacing_min,  # Access from GSAS2Config
            "number_of_regions": self.config.number_of_regions,  # Access from GSAS2Config
            "gsasii_scriptable_path": str(self.gsasii_scriptable_path),  # Add the path to GSASIIscriptable.py
        }
        return json.dumps(inputs_dict, separators=(",", ":"))


class GSAS2Model:
    """
    GSAS2Model is a class that provides an interface for managing and executing GSAS-II refinements.
    It handles the preparation of input parameters, validation, execution of GSAS-II subprocesses,
    and processing of output results. The class is designed to integrate with Mantid's Engineering
    Diffraction interface.
    """

    ReflectionType: TypeAlias = Tuple[List[int], float, int]

    def __init__(self) -> None:
        self.config = GSAS2ModelConfig()
        self.state = GSAS2ModelState()

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
        self.phase_names_list: List[str] = []
        self.gsas2_save_dirs: List[str] = []
        self.crystal_structures: List[CrystalStructure] = []
        self.temporary_save_directory = ""
        self.user_save_directory: Optional[str] = None

    def clear_input_components(self) -> None:
        """
        Clears all input components and resets the configuration, state, and refinement settings
        to their default values.
        """
        # Reset configuration and state to their default values
        self.config = GSAS2ModelConfig()
        self.state = GSAS2ModelState()

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
        user_x_limits: Optional[List[float]] = None,
    ) -> Optional[int]:
        self.clear_input_components()
        if not self.initial_validation(project_name, load_parameters):
            return None
        self.set_components_from_inputs(load_parameters, refinement_parameters, project_name, rb_num)
        self.read_phase_files()
        self.generate_reflections_from_space_group()
        if user_x_limits and isinstance(user_x_limits, list) and all(isinstance(x, float) for x in user_x_limits):
            formatted_limits = [[x] for x in user_x_limits]
        else:
            formatted_limits = None

        self.validate_x_limits(formatted_limits)
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

        if self.number_of_regions > self.number_histograms:
            return self.number_of_regions
        return self.number_histograms

    # ===============
    # Prepare Inputs
    # ===============

    def set_components_from_inputs(self, load_params: list, refinement_params: list, project: str, rb_number: Optional[str] = None) -> None:
        self.path_to_gsas2 = output_settings.get_path_to_gsas2()
        self.project_name = project
        self.organize_save_directories(rb_number)

        self.instrument_files = load_params[0]
        self.phase_filepaths = load_params[1]
        self.data_files = load_params[2]

        self.refinement_method = refinement_params[0]
        self.override_cell_length_string = refinement_params[1]
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

    def further_validation(self) -> Optional[bool]:
        if not self.limits:
            return None  # any errors logged in validate_x_limits()
        if self.refinement_method == "Pawley" and not self.mantid_pawley_reflections:
            logger.error("No Pawley Reflections were generated for the phases provided. Not calling GSAS-II.")
            return None
        if not self.path_to_gsas2:
            logger.error("The Path to GSAS2 setting is empty. Please provide a valid path in Engineering Diffraction Settings")
            return None
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
        file_paths = self._create_file_paths()
        config = self._create_gsas2_config()

        gsas2_handler = self._initialize_gsas2_handler(save_directories, refinement_settings, file_paths, config)

        call_g2sc_path = self._locate_call_g2sc_script(gsas2_handler)

        serialized_inputs = gsas2_handler.to_json()

        gsasii_call_args = self._construct_gsasii_call(gsas2_handler, call_g2sc_path, serialized_inputs)

        return self.call_subprocess(gsasii_call_args, gsas2_handler.python_binaries)

    def _validate_required_attributes(self) -> None:
        if not self.path_to_gsas2.strip():
            raise ValueError("Path to GSAS-II is not set. Please provide a valid path.")
        if not self.project_name:
            raise ValueError("Project name is not set. Please provide a valid project name.")

    def _create_save_directories(self) -> SaveDirectories:
        if not self.project_name:
            raise ValueError("Project name is not set. Please provide a valid project name.")
        return SaveDirectories(
            temporary_save_directory=self.temporary_save_directory,
            project_name=self.project_name,
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

    def _create_file_paths(self) -> FilePaths:
        return FilePaths(
            data_files=self.data_files,
            phase_files=self.phase_filepaths,
            instrument_files=self.instrument_files,
        )

    def _create_gsas2_config(self) -> GSAS2Config:
        return GSAS2Config(
            limits=self.limits,
            mantid_pawley_reflections=self.mantid_pawley_reflections,
            override_cell_lengths=self.get_override_lattice_parameters(),
            d_spacing_min=self.dSpacing_min,
            number_of_regions=self.number_of_regions,
        )

    def _initialize_gsas2_handler(
        self, save_directories: SaveDirectories, refinement_settings: RefinementSettings, file_paths: FilePaths, config: GSAS2Config
    ) -> GSAS2Handler:
        gsas2_handler = GSAS2Handler(
            path_to_gsas2=self.path_to_gsas2,
            save_directories=save_directories,
            refinement_settings=refinement_settings,
            file_paths=file_paths,
            config=config,
        )

        gsas2_handler.os_platform = platform.system()
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

    def call_subprocess(self, command_string_list: List[str], gsas_binary_paths: List[str]) -> Optional[Tuple[str, str]]:
        """
        Notes:
        1. **Binary Search Order**:
            - The GSAS-II binaries are searched in the GSASII directory before the Mantid Conda environment.
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
            env = os.environ.copy()
            env["PYTHONHOME"] = self.config.path_to_gsas2
            # Search for binaries in GSASII directory before Mantid Conda environment
            env["PATH"] = ";".join(gsas_binary_paths + [env["PATH"]])
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
        if not self.number_of_regions:
            crystal_params = [self.find_in_file(instrument, "ICONS", "S", "INS", strip_separator="ICONS\t")]

        else:
            for region_index in range(1, self.number_of_regions + 1):
                loop_crystal_params = self.find_in_file(instrument, f"{region_index} ICONS", "S", "INS", strip_separator="ICONS\t")
                crystal_params.append(loop_crystal_params)
        tof_min: List[float] = []
        file_mismatch: str = f"Different number of banks in {instrument} and {self.data_files}"
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
        self.crystal_structures.clear()
        for phase_filepath in self.phase_filepaths:
            self.crystal_structures.append(self._read_cif_file(phase_filepath))
        if self.override_cell_length_string:
            # override lattice parameters of first file
            self.crystal_structures[0] = self.set_lattice_params_from_user_input(self.crystal_structures[0])

    def get_override_lattice_parameters(self) -> Optional[List[List[float]]]:
        if self.override_cell_length_string:
            return [self._get_lattice_parameters_from_crystal_structure(xtal) for xtal in self.crystal_structures]
        else:
            None

    def _get_lattice_parameters_from_crystal_structure(self, crystal_structure: CrystalStructure) -> List[float]:
        return [getattr(crystal_structure.getUnitCell(), method)() for method in ("a", "b", "c", "alpha", "beta", "gamma")]

    def _read_cif_file(self, phase_filepath: str) -> CrystalStructure:
        ws = CreateSampleWorkspace(StoreInADS=False)
        LoadCIF(ws, phase_filepath, StoreInADS=False)  # error if not StoreInADS=False even though no output
        return ws.sample().getCrystalStructure()

    def set_lattice_params_from_user_input(self, input_crystal_structure: CrystalStructure) -> CrystalStructure:
        # User can delimit with commas or whitespace
        user_alatt: List[str] = list(self.override_cell_length_string.replace(",", " ").split())
        if len(user_alatt) == 1:
            user_alatt = 3 * user_alatt  # Assume cubic lattice

        updated_crystal_structure: CrystalStructure = CrystalStructure(
            " ".join(user_alatt),
            input_crystal_structure.getSpaceGroup().getHMSymbol(),
            ";".join(input_crystal_structure.getScatterers()),
        )
        return updated_crystal_structure

    def generate_reflections_from_space_group(self) -> None:
        self.mantid_pawley_reflections = []
        assert isinstance(self.crystal_structures, list), "crystal_structures should be a list"
        for crystal_structure in self.crystal_structures:
            self.mantid_pawley_reflections.append(self.create_pawley_reflections(crystal_structure))
        return

    # =========
    # X Limits
    # =========

    def understand_data_structure(self) -> None:
        self.data_x_min = []
        self.data_x_max = []
        number_of_regions = 0
        for input_file in self.data_files:
            loop_focused_workspace = LoadGSS(Filename=input_file, OutputWorkspace="GSASII_input_data", EnableLogging=False)
            for workspace_index in range(loop_focused_workspace.getNumberHistograms()):
                self.data_x_min.append(loop_focused_workspace.readX(workspace_index)[0])
                self.data_x_max.append(loop_focused_workspace.readX(workspace_index)[-1])
                number_of_regions += 1
            DeleteWorkspace(loop_focused_workspace)
        self.number_of_regions = number_of_regions

    def validate_x_limits(self, users_limits: Optional[List[List[float]]]) -> bool:
        self.understand_data_structure()
        if users_limits:
            if len(users_limits[0]) != self.number_of_regions:
                users_limits[0] *= self.number_of_regions
                users_limits[1] *= self.number_of_regions
        self.number_histograms = len(self.data_files)
        if len(self.instrument_files) != 1 and len(self.instrument_files) != self.number_histograms:
            logger.error(
                f"The number of instrument files ({len(self.instrument_files)}) must be 1 "
                f"or equal to the number of input histograms {self.number_histograms}"
            )
            return False
        if users_limits:
            self.x_min = [float(k) for k in users_limits[0]]
            self.x_max = [float(k) for k in users_limits[1]]
            if len(self.x_min) != self.number_histograms:
                if len(self.x_min) == 1:
                    self.x_min = self.x_min * self.number_histograms
                    self.x_max = self.x_max * self.number_histograms
        else:
            self.x_min = self.data_x_min
            self.x_max = self.data_x_max

        self.limits = [self.x_min, self.x_max]
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
        return gsas_result_filepath, logged_success  # Always return a tuple

    def check_for_output_file(self, file_extension: str, file_descriptor: str, test: bool = False) -> Optional[Union[str, None]]:
        gsas_output_filename = self.project_name + file_extension
        if gsas_output_filename not in os.listdir(self.temporary_save_directory):
            logged_failure = (
                f"GSAS-II call must have failed, as the output {file_descriptor} file was not found."
                + self.format_shell_output(title="Errors from GSAS-II", shell_output_string=self.state.err_call_gsas2 or "")
            )
            logger.error(logged_failure)
            if test:
                return logged_failure
            return None
        return os.path.join(self.temporary_save_directory, gsas_output_filename)

    def organize_save_directories(self, rb_num_string: Optional[str]) -> None:
        save_dir: str = os.path.join(output_settings.get_output_path())
        self.gsas2_save_dirs = [os.path.join(save_dir, "GSAS2", "")]
        save_directory: str = self.gsas2_save_dirs[0]
        if rb_num_string:
            self.gsas2_save_dirs.append(os.path.join(save_dir, "User", rb_num_string, "GSAS2", self.project_name, ""))
            # TODO: Once texture is supported, pass calibration observer like currently done for focus tab
            # if calibration.group == GROUP.TEXTURE20 or calibration.group == GROUP.TEXTURE30:
            #     calib_dirs.pop(0)  # only save to RB directory to limit number files saved
        self.user_save_directory = os.path.join(save_directory, self.project_name)
        self.temporary_save_directory: str = os.path.join(
            save_directory, datetime.datetime.now().strftime("tmp_EngDiff_GSASII_%Y-%m-%d_%H-%M-%S")
        )
        os.makedirs(self.temporary_save_directory)

    def move_output_files_to_user_save_location(self) -> str:
        for new_directory in self.gsas2_save_dirs:
            os.makedirs(new_directory, exist_ok=True)

        save_success_message: str = f"\n\nOutput GSAS-II files saved in {self.user_save_directory}"

        exist_extra_save_dirs: bool = False
        if len(self.gsas2_save_dirs) > 1:
            exist_extra_save_dirs = True
            self.gsas2_save_dirs.pop(0)

        if self.user_save_directory and os.path.exists(self.user_save_directory):
            shutil.rmtree(self.user_save_directory)
        if self.user_save_directory is not None:
            os.makedirs(self.user_save_directory, exist_ok=True)
        else:
            raise ValueError("user_save_directory is None. Cannot create directory.")
        for output_file_index, output_file in enumerate(os.listdir(self.temporary_save_directory)):
            os.rename(os.path.join(self.temporary_save_directory, output_file), os.path.join(self.user_save_directory, output_file))
            if exist_extra_save_dirs:
                for extra_save_dir in self.gsas2_save_dirs:
                    shutil.copy(os.path.join(self.user_save_directory, output_file), os.path.join(extra_save_dir, output_file))
                    if output_file_index == 0:
                        save_success_message += f" and in {extra_save_dir}"
        os.rmdir(self.temporary_save_directory)
        return save_success_message

    # =============
    # Load Outputs
    # =============

    def load_basic_outputs(self, gsas_result_filepath: str) -> None:
        logger.notice(f"GSAS-II .lst result file found. Opening {self.project_name}.lst")
        self.read_gsas_lst_and_print_wR(gsas_result_filepath, self.data_files)
        save_message = self.move_output_files_to_user_save_location()
        if self.user_save_directory is None:
            raise ValueError("user_save_directory is None. Cannot construct the file path.")
        self.phase_names_list = self.find_phase_names_in_lst(os.path.join(self.user_save_directory, self.project_name + ".lst"))
        logger.notice(save_message)

        self.phase_names_list: List[str] = self.find_phase_names_in_lst(os.path.join(self.user_save_directory, self.project_name + ".lst"))
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
        result_csv = os.path.join(self.user_save_directory, self.project_name + f"_{histogram_index}.csv")
        my_data = np.transpose(np.genfromtxt(result_csv, delimiter=",", skip_header=39))
        # x  y_obs	weight	y_calc	y_bkg	Q
        x_values = my_data[0]
        y_obs = self.chop_to_limits(np.array(my_data[1]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_calc = self.chop_to_limits(np.array(my_data[3]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_diff = y_obs - y_calc
        y_diff -= np.max(np.ma.masked_invalid(y_diff))
        y_bkg = self.chop_to_limits(np.array(my_data[4]), x_values, self.x_min[histogram_index - 1], self.x_max[histogram_index - 1])
        y_data = np.concatenate((y_obs, y_calc, y_diff, y_bkg))

        gsas_histogram = CreateWorkspace(
            OutputWorkspace=f"gsas_histogram_{histogram_index}", DataX=np.tile(my_data[0], 4), DataY=y_data, NSpec=4, EnableLogging=False
        )
        return gsas_histogram

    def load_gsas_reflections_per_histogram_for_plot(self, histogram_index: int) -> List[np.ndarray]:
        loaded_reflections: List[np.ndarray] = []
        for phase_name in self.phase_names_list:
            if not self.user_save_directory:
                raise ValueError("user_save_directory is None. Cannot construct the file path.")
            result_reflections_txt = os.path.join(
                self.user_save_directory, self.project_name + f"_reflections_{histogram_index}_{phase_name}.txt"
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
            if ADS.doesExist(f"{self.project_name}_GSASII_reflections"):
                DeleteWorkspace(f"{self.project_name}_GSASII_reflections", EnableLogging=False)
            return None
        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_reflections", EnableLogging=False)
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
        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_instrument_parameters", EnableLogging=False)
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
            loop_inst_parameters.append(float(self.x_min[bank_number_from_gsas2_histogram_name - 1]))
            loop_inst_parameters.append(float(self.x_max[bank_number_from_gsas2_histogram_name - 1]))
            table.addRow(loop_inst_parameters)
        if test:
            return table
        return None

    def create_lattice_parameter_table(self, test: bool = False) -> Optional[Union[None, object]]:
        LATTICE_TABLE_PARAMS: List[str] = ["length_a", "length_b", "length_c", "angle_alpha", "angle_beta", "angle_gamma", "volume"]

        table = CreateEmptyTableWorkspace(OutputWorkspace=f"{self.project_name}_GSASII_lattice_parameters", EnableLogging=False)
        table.addReadOnlyColumn("str", "Phase name")

        for param in LATTICE_TABLE_PARAMS:
            table.addReadOnlyColumn("double", param.split("_")[-1])
        table.addReadOnlyColumn("double", "Microstrain{}".format(" (Refined)" if self.refine_microstrain else ""))
        for phase_name in self.phase_names_list:
            if not self.user_save_directory:
                raise ValueError("user_save_directory is None. Cannot construct the file path.")
            parameters_txt: str = os.path.join(self.user_save_directory, self.project_name + f"_cell_parameters_{phase_name}.txt")
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
        if len(filenames) == 1 and "all_banks" in filenames[0]:
            filenames = [filenames[0].replace("all_banks", "bank_1"), filenames[0].replace("all_banks", "bank_2")]
        for filename in filenames:
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
        if self.data_files:
            if len(self.data_files) == 1:
                input_file_name = os.path.basename(self.data_files[0])
            else:
                input_file_name = os.path.basename(self.data_files[histogram_index - 1])
        return "GSAS-II Refinement " + input_file_name

    def _create_reflection_labels(self, reflection_positions: List[np.ndarray]) -> List[str]:
        """Create the labels used to identify different phase reflection lists
        The primary format, if the number of phase names equals the number of positions, is 'reflections_{phase_name}'
        The fallback format is 'reflections_phase_{index}' where index starts at 1
        """
        reflection_labels = [f"reflections_{phase_name}" for phase_name in self.phase_names_list]
        if len(reflection_labels) != len(reflection_positions):
            return [f"reflections_phase_{i}" for i in range(1, len(reflection_positions) + 1)]
        return reflection_labels
