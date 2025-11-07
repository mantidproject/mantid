# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from os import path
from mantid.kernel import logger
from numpy import all, array, concatenate, abs, eye
from numpy.linalg import det, norm
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_presenter import SETTINGS_DICT


class SettingsModel(object):
    def get_settings_dict(self, names_and_types):
        settings = {}
        for setting_name in names_and_types.keys():
            settings[setting_name] = self.get_setting(setting_name, return_type=names_and_types[setting_name])
        return settings

    def set_settings_dict(self, settings):
        for key in settings:
            self.set_setting(key, settings[key])

    @staticmethod
    def get_setting(name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name, return_type=return_type)

    @staticmethod
    def set_setting(name, value):
        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name, value)

    # validation methods

    def validate_gsas2_path(self, gsas2_path):
        # FileFinderWidget doesn't validate that the directory exists if isForDirectory=true (probably to support save
        # locations where directory gets created on save). So check the GSAS2 path is valid here
        msg = "Path does not exists" if not self.validate_path_empty_or_valid(gsas2_path) else ""
        return msg == "", msg

    @staticmethod
    def validate_reference_frame(settings):
        try:
            rd = array([float(x) for x in settings["rd_dir"].split(",")])[:, None]
            nd = array([float(x) for x in settings["nd_dir"].split(",")])[:, None]
            td = array([float(x) for x in settings["td_dir"].split(",")])[:, None]
            trans_mat = concatenate([rd, nd, td], axis=1)
            _ = abs(det(trans_mat))  # ensure a determinant can be calculated
            if not is_approx_orthonormal(trans_mat):
                logger.warning("Currently only orthonormal sample axes are properly supported")
        except Exception as e:
            logger.error("Invalid Reference Axes, values must all be able to be converted to floats. " + str(e))

    def validate_euler_settings(self, settings, use_euler_angles):
        if use_euler_angles:
            error_msg = ""
            euler_scheme = settings["euler_angles_scheme"]
            euler_sense = settings["euler_angles_sense"]
            # check sense is comma separated
            euler_sense_valid, sense_vals, sense_msg = self._validate_euler_sense_string(euler_sense)
            if not euler_sense_valid:
                error_msg += sense_msg

            # validate euler_scheme
            euler_scheme_valid = all([v in ("x", "y", "z") for v in euler_scheme.lower()])
            if not euler_scheme_valid:
                error_msg += "Euler Scheme should be defined in terms of X, Y and Z (eg. XYZ or YZY). "

            # check euler_scheme has correct number of corresponding senses
            sense_scheme_valid = len(euler_scheme) == len(sense_vals)
            if not sense_scheme_valid:
                error_msg += "Should be an equal number of Rotation Axes defined as euler senses. "
            valid = euler_sense_valid and euler_scheme_valid and sense_scheme_valid
            if not valid:
                logger.error(error_msg)

    @staticmethod
    def _validate_euler_sense_string(euler_sense):
        try:
            sense_vals = [int(x) for x in euler_sense.split(",")]
            valid = all([v in (1, -1) for v in sense_vals])
            error_msg = ""
        except Exception as e:
            error_msg = "Euler Senses should be comma separated +/-1s. " + str(e)
            sense_vals = []
            valid = False
        return valid, sense_vals, error_msg

    @staticmethod
    def _validate_convert_to_float(settings, setting_name):
        val = ""
        try:
            val = settings[setting_name]
            float(val)
        except ValueError:
            logger.error(f"Could not convert {setting_name} value of {val} to a float")

    @staticmethod
    def validate_path_empty_or_valid(path_to_check):
        if path_to_check:
            if not path.exists(path_to_check):
                return False
        return True

    @staticmethod
    def _check_and_populate_with_default(name, settings, default_settings):
        if name not in settings or settings[name] == "":
            settings[name] = default_settings[name]
        return settings

    def validate_settings(self, settings, default_settings, all_peaks, set_nullables_to_default=True):
        # set to class attributes to save duplicating arguments and passing settings dict around too much

        for key in list(settings):
            if key not in default_settings.keys():
                del settings[key]

        # check and populate calls
        settings_to_check = list(SETTINGS_DICT.keys())
        if not set_nullables_to_default:
            settings_to_check.pop(settings_to_check.index("primary_log"))
        for name in settings_to_check:
            settings = self._check_and_populate_with_default(name, settings, default_settings)

        # value validations
        self.validate_reference_frame(settings)
        if settings["default_peak"] not in all_peaks:
            settings["default_peak"] = default_settings["default_peak"]
        if not path.isfile(settings["full_calibration"]):
            settings["full_calibration"] = default_settings["full_calibration"]
        self._validate_convert_to_float(settings, "timeout")
        self._validate_convert_to_float(settings, "dSpacing_min")
        self._validate_convert_to_float(settings, "cost_func_thresh")
        self._validate_convert_to_float(settings, "peak_pos_thresh")
        self._validate_convert_to_float(settings, "contour_kernel")
        return settings


def is_approx_orthonormal(mat, tol=1e-5):
    n = mat.shape[1]
    identity = eye(n)
    error = norm(mat.T @ mat - identity, ord="fro")
    return error < tol
