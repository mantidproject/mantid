# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import re


class TomlValidationError(Exception):
    # A key error would be more appropriate, but there is special
    # handling on KeyError which escapes any new lines making it un-readable
    pass


class TomlSchemaV1Validator(object):
    # As of the current TOML release there is no way to validate a schema so
    # we must provide an implementation

    # Note : To future devs, if we have a V2 schema a lot of this class could
    # be split into a SchemaValidator and an inheriting V1 and V2 schema
    # would override the reference schema with the new one

    def __init__(self, dict_to_validate):
        self._expected_list = self._build_nested_keys(self._reference_schema())
        self._to_validate_list = self._build_nested_keys(dict_to_validate)

    def validate(self):
        self._to_validate_list = filter(lambda s: not s.startswith("metadata"), self._to_validate_list)
        unrecognised = set(self._to_validate_list).difference(self._expected_list)

        if not unrecognised:
            return

        # Build any with wildcards
        wildcard_matchers = [re.compile(s) for s in self._expected_list if "*" in s]
        # Remove anything which matches any the regex wildcards
        unrecognised = [s for s in unrecognised if not any(wild_matcher.match(s) for wild_matcher in wildcard_matchers)]

        if len(unrecognised) > 0:
            err = "The following keys were not recognised:\n"
            err += "".join("{0} \n".format(k) for k in unrecognised)
            raise TomlValidationError(err)

    @staticmethod
    def _reference_schema():
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

        polarization_keys = {
            "flipper_configuration",
            "spin_configuration",
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
            "polarization": polarization_keys,
        }

    @staticmethod
    def _build_nested_keys(d, path="", current_out=None):
        if not current_out:
            current_out = []

        def make_path(current_path, new_key):
            return current_path + "." + new_key if current_path else new_key

        for key, v in d.items():
            new_path = make_path(path, key)
            if isinstance(v, dict):
                # Recurse into dict
                current_out = TomlSchemaV1Validator._build_nested_keys(v, new_path, current_out)
            elif isinstance(v, set):
                # Pack all in from the set of names
                for name in v:
                    current_out.append(make_path(new_path, name))
            else:
                # This means its a value type with nothing special, so keep name
                current_out.append(new_path)

        return current_out
