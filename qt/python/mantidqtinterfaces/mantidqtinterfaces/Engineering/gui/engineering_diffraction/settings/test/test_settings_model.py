# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_model import SettingsModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_presenter import DEFAULT_SETTINGS, ALL_PEAKS

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings."


class SettingsModelTest(unittest.TestCase):
    def setUp(self):
        self.model = SettingsModel()

    @patch(dir_path + "settings_model.set_setting")
    def test_set_setting(self, set_setting_mock):
        self.model.set_setting("name", "value")
        set_setting_mock.assert_called_with("CustomInterfaces", "EngineeringDiffraction2/", "name", "value")

    @patch(dir_path + "settings_model.get_setting")
    def test_get_setting(self, get_setting_mock):
        self.model.get_setting("name")
        get_setting_mock.assert_called_with("CustomInterfaces", "EngineeringDiffraction2/", "name", return_type=str)

    @patch(dir_path + "settings_model.set_setting")
    def test_set_settings_dict(self, set_setting_mock):
        self.model.set_settings_dict({"name": "value", "namebool": False, "namenum": 10})
        self.assertEqual(set_setting_mock.call_count, 3)
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name", "value")
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "namebool", False)
        set_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "namenum", 10)

    @patch(dir_path + "settings_model.get_setting")
    def test_get_settings_dict(self, get_setting_mock):
        get_setting_mock.return_value = "value"
        self.assertEqual(self.model.get_settings_dict({"name1": str, "name2": str}), {"name1": "value", "name2": "value"})
        self.assertEqual(get_setting_mock.call_count, 2)
        get_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name1", return_type=str)
        get_setting_mock.assert_any_call("CustomInterfaces", "EngineeringDiffraction2/", "name2", return_type=str)

    def test_load_invalid_settings(self):
        bad_settings = {"foo": "dud", "bar": "result"}
        output_settings = self.model.validate_settings(bad_settings, DEFAULT_SETTINGS, ALL_PEAKS)

        self.assertEqual(output_settings, DEFAULT_SETTINGS)

    def test_load_invalid_settings_correct_keys(self):
        input_settings = DEFAULT_SETTINGS.copy()
        input_settings["full_calibration"] = ""  # invalid value

        output_settings = self.model.validate_settings(input_settings, DEFAULT_SETTINGS, ALL_PEAKS)

        self.assertEqual(output_settings, DEFAULT_SETTINGS)

    def test_validate_euler_sense_string(self):
        good_inputs = ["1,1,1", "1,-1,1", "1", "-1,1,-1,-1,-1"]
        expected_senses = [
            [1, 1, 1],
            [1, -1, 1],
            [
                1,
            ],
            [-1, 1, -1, -1, -1],
        ]

        bad_inputs = ["CW, CW", "0,0,0", "1 1 1", (1, -1, 1)]

        for i, good_input in enumerate(good_inputs):
            valid, sense, msg = self.model._validate_euler_sense_string(good_input)
            self.assertTrue(valid)
            self.assertEqual(sense, expected_senses[i])
            self.assertEqual(msg, "")

        for i, bad_input in enumerate(bad_inputs):
            valid, sense, msg = self.model._validate_euler_sense_string(bad_input)
            self.assertFalse(valid)

    @patch(dir_path + "settings_model.logger")
    def test_validate_euler_settings(self, mock_logger):
        good_inputs = (("XYX", "1,1,1"), ("xyz", "1,1,1"), ("x", "1"))
        bad_inputs = (("x,y,z", "1,1,1"), ("abc", "1,1,1"), ("xyz", "1"))

        for good_input in good_inputs:
            mock_logger.reset_mock()
            settings = dict(zip(("euler_angles_scheme", "euler_angles_sense"), good_input))
            self.model.validate_euler_settings(settings, True)
            mock_logger.error.assert_not_called()

        for bad_input in bad_inputs:
            mock_logger.reset_mock()
            settings = dict(zip(("euler_angles_scheme", "euler_angles_sense"), bad_input))
            self.model.validate_euler_settings(settings, True)
            mock_logger.error.assert_called_once()

    @patch(dir_path + "settings_model.logger")
    def test_validate_reference_frame(self, mock_logger):
        r2o2 = (2**0.5) / 2
        good_inputs = (["1,0,0", "0,1,0", "0,0,1"], ["0,1,0", "1,0,0", "0,0,1"], [f"{r2o2},{r2o2},0", f"{r2o2},{r2o2},0", "0,0,1"])
        bad_inputs = (["100", "010", "001"], ["a,b,c", "a,b,c", "a,b,c"])

        for good_input in good_inputs:
            mock_logger.reset_mock()
            settings = dict(zip(("rd_dir", "nd_dir", "td_dir"), good_input))
            self.model.validate_reference_frame(settings)
            mock_logger.error.assert_not_called()

        for bad_input in bad_inputs:
            mock_logger.reset_mock()
            settings = dict(zip(("rd_dir", "nd_dir", "td_dir"), bad_input))
            self.model.validate_reference_frame(settings)
            mock_logger.error.assert_called_once()


if __name__ == "__main__":
    unittest.main()
