# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import json


class Gsas2InputParameters:
    """
    A container for GSAS-II input parameters used in the engineering diffraction interface.

    This class organizes and stores all the necessary parameters for GSAS-II refinement tasks.
    """

    def __init__(
        self,
        *,
        path_to_gsas2,
        temporary_save_directory,
        project_name,
        refinement_method,
        refine_background,
        refine_microstrain,
        refine_sigma_one,
        refine_gamma,
        refine_histogram_scale_factor,
        data_files,
        phase_files,
        instrument_files,
        limits,
        mantid_pawley_reflections,
        override_cell_lengths,
        refine_unit_cell,
        d_spacing_min,
        number_of_regions,
    ):
        self.path_to_gsas2 = path_to_gsas2
        self.temporary_save_directory = temporary_save_directory
        self.project_name = project_name
        self.refinement_settings = {
            "method": refinement_method,
            "background": refine_background,
            "microstrain": refine_microstrain,
            "sigma_one": refine_sigma_one,
            "gamma": refine_gamma,
            "histogram_scale_factor": refine_histogram_scale_factor,
            "unit_cell": refine_unit_cell,
        }
        self.file_paths = {
            "data_files": data_files,
            "phase_files": phase_files,
            "instrument_files": instrument_files,
        }
        self.limits = limits  # x_min and x_max
        self.mantid_pawley_reflections = mantid_pawley_reflections
        self.override_cell_lengths = override_cell_lengths
        self.refine_unit_cell = refine_unit_cell
        self.d_spacing_min = d_spacing_min
        self.number_of_regions = number_of_regions

    def matching_dict(self):
        match_dict = {
            "path_to_gsas2": self.path_to_gsas2,
            "temporary_save_directory": self.temporary_save_directory,
            "project_name": self.project_name,
            "refinement_settings": self.refinement_settings,
            "file_paths": self.file_paths,
            "limits": self.limits,
            "mantid_pawley_reflections": self.mantid_pawley_reflections,
            "override_cell_lengths": self.override_cell_lengths,
            "refine_unit_cell": self.refine_unit_cell,
            "d_spacing_min": self.d_spacing_min,
            "number_of_regions": self.number_of_regions,
        }
        return match_dict

    def validate_inputs(self):
        """
        Validates the input parameters to ensure they meet the expected criteria.
        Raises a ValueError if any parameter is invalid.
        """
        if not self.path_to_gsas2 or not isinstance(self.path_to_gsas2, str):
            raise ValueError("Invalid path_to_gsas2: must be a non-empty string.")
        if not self.file_paths["data_files"] or not isinstance(self.file_paths["data_files"], list):
            raise ValueError("Invalid data_files: must be a non-empty list.")


def Gsas2InputParameters_to_json(gsas2_inputs):
    return json.dumps(gsas2_inputs.matching_dict(), separators=(",", ":"))
