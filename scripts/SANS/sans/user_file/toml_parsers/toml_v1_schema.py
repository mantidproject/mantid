# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.user_file.toml_parsers.toml_base_schema import TomlSchemaValidator


class TomlSchemaV1Validator(TomlSchemaValidator):
    # As of the current TOML release there is no way to validate a schema so
    # we must provide an implementation

    def __init__(self, dict_to_validate):
        super(TomlSchemaV1Validator, self).__init__(dict_to_validate)

    @staticmethod
    def reference_schema():
        """
        Returns a dictionary layout of all supported keys
        :return: Dictionary containing all keys, and values set to None
        """
        instrument_keys = {
            "name": None,
            "configuration": {
                "collimation_length",
                "gravity_enabled",
                "gravity_extra_length",
                "norm_monitor",
                "sample_aperture_diameter",
                "sample_offset",
                "trans_monitor",
            },
        }

        detector_keys = {
            "configuration": {
                "selected_detector": None,
                "rear_scale": None,
                "all_centre": {"x", "y"},
                "front_centre": {"x", "y"},
                "rear_centre": {"x", "y"},
            },
            "correction": {
                "direct": {"front_file", "rear_file"},
                "flat": {"front_file", "rear_file"},
                "tube": {"file"},
                "position": {
                    "front_x",
                    "front_y",
                    "front_z",
                    "front_rot",
                    "front_radius",
                    "front_side",
                    "front_x_tilt",
                    "front_y_tilt",
                    "front_z_tilt",
                    "rear_x",
                    "rear_y",
                    "rear_z",
                    "rear_rot",
                    "rear_radius",
                    "rear_side",
                    "rear_x_tilt",
                    "rear_y_tilt",
                    "rear_z_tilt",
                },
            },
            "radius_limit": {"min", "max"},
        }

        binning_keys = {
            "wavelength": {"start", "step", "stop", "type", "binning"},
            "1d_reduction": {"binning", "radius_cut", "wavelength_cut"},
            "2d_reduction": {"step", "stop", "type"},
        }

        reduction_keys = {
            "merged": {
                "rescale": {"min", "max", "use_fit"},
                "shift": {"min", "max", "use_fit", "factor"},
                "merge_range": {"min", "max", "use_fit"},
            },
            "events": {"binning", "type"},
        }

        q_resolution_keys = {"enabled", "moderator_file", "source_aperture", "delta_r", "h1", "h2", "w1", "w2"}

        transmission_keys = {
            "monitor": {
                "*": {"spectrum_number", "background", "shift", "use_own_background", "use_different_norm_monitor", "trans_norm_monitor"}
            },
            "ROI": {"file"},
            "fitting": {"enabled": None, "function": None, "polynomial_order": None, "parameters": {"lambda_min", "lambda_max"}},
        }

        normalisation_keys = {"monitor": {"*": {"spectrum_number", "background"}}, "all_monitors": {"background", "enabled"}}

        mask_keys = {
            "prompt_peak": {"start", "stop"},
            "mask_files": None,
            "phi": {"mirror", "start", "stop"},
            "time": {"tof"},
            "spatial": {
                "rear": {"detector_columns", "detector_rows", "detector_column_ranges", "detector_row_ranges"},
                "front": {"detector_columns", "detector_rows", "detector_column_ranges", "detector_row_ranges"},
                "beamstop_shadow": {"angle", "width", "x_pos", "y_pos"},
                "mask_pixels": None,
            },
        }

        return {
            "toml_file_version": None,
            "binning": binning_keys,
            "metadata": ".*",
            "detector": detector_keys,
            "mask": mask_keys,
            "instrument": instrument_keys,
            "normalisation": normalisation_keys,
            "normalization": normalisation_keys,  # Accept both forms
            "q_resolution": q_resolution_keys,
            "reduction": reduction_keys,
            "transmission": transmission_keys,
        }
