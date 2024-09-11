# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import json


class Gsas2Inputs:
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
        self.refinement_method = refinement_method
        self.refine_background = refine_background
        self.refine_microstrain = refine_microstrain
        self.refine_sigma_one = refine_sigma_one
        self.refine_gamma = refine_gamma
        self.refine_histogram_scale_factor = refine_histogram_scale_factor
        self.data_files = data_files
        self.phase_files = phase_files
        self.instrument_files = instrument_files
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
            "refinement_method": self.refinement_method,
            "refine_background": self.refine_background,
            "refine_microstrain": self.refine_microstrain,
            "refine_sigma_one": self.refine_sigma_one,
            "refine_gamma": self.refine_gamma,
            "refine_histogram_scale_factor": self.refine_histogram_scale_factor,
            "data_files": self.data_files,
            "phase_files": self.phase_files,
            "instrument_files": self.instrument_files,
            "limits": self.limits,
            "mantid_pawley_reflections": self.mantid_pawley_reflections,
            "override_cell_lengths": self.override_cell_lengths,
            "refine_unit_cell": self.refine_unit_cell,
            "d_spacing_min": self.d_spacing_min,
            "number_of_regions": self.number_of_regions,
        }
        return match_dict


def Gsas2Inputs_to_json(gsas2_inputs):
    return json.dumps(gsas2_inputs.matching_dict(), separators=(",", ":"))
