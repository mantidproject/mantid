# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#

from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Union
import os
import fnmatch
import json


@dataclass
class SaveDirectories:
    """
    A dataclass to store save directory params for temporary files and project-related data.
    """

    temporary_save_directory: str
    project_name: str


@dataclass
class RefinementSettings:
    """
    A dataclass representing the settings for refinement in the GSAS-II engineering diffraction interface.

    Attributes:
        method: The refinement method to be used.
        background: Indicates whether background refinement is enabled.
        microstrain: Indicates whether microstrain refinement is enabled.
        sigma_one: Indicates whether sigma one refinement is enabled.
        gamma: Indicates whether gamma refinement is enabled.
        histogram_scale_factor: Indicates whether histogram scale factor refinement is enabled.
        unit_cell: Indicates whether unit cell refinement is enabled.
    """

    method: str
    background: bool
    microstrain: bool
    sigma_one: bool
    gamma: bool
    histogram_scale_factor: bool
    unit_cell: bool


@dataclass
class FilePaths:
    """
    A dataclass to store file path lists related to engineering diffraction.

    Attributes:
        data_files: A list of paths to data files.
        phase_filepaths: A list of paths to phase files.
        instrument_files: A list of paths to instrument files.
    """

    data_files: List[str] = field(default_factory=list)
    phase_filepaths: List[str] = field(default_factory=list)
    instrument_files: List[str] = field(default_factory=list)
    gsas2_save_dirs: List[str] = field(default_factory=list)


@dataclass
class GSAS2Config:
    """
    Configuration dataclass for GSAS-II integration in the engineering diffraction interface.

    Attributes:
        limits: A list of limits for the configuration.
        mantid_pawley_reflections: A nested list containing Mantid Pawley reflections data.
        override_cell_lengths: A list of cell length overrides.
        d_spacing_min: The minimum d-spacing value.
        number_of_regions: The number of regions to configure.
    """

    limits: Optional[List[Union[int, float]]] = field(default_factory=list)
    mantid_pawley_reflections: Optional[List[Union[str, int]]] = None
    override_cell_lengths: Optional[List[List[float]]] = None
    d_spacing_min: float = 1.0
    number_of_regions: int = 1


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
            (self.file_paths.phase_filepaths, "phase_filepaths"),
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
