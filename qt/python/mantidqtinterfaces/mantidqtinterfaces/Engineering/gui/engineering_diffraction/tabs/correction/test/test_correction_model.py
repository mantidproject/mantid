# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright © 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.correction.model import (
    CorrectionModel,
)

model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.correction.model"


class TestCorrectionModel(unittest.TestCase):
    def setUp(self):
        self.model = CorrectionModel()
        # replace the setter with a mock so we can assert it
        self.model.set_reference_ws = MagicMock()

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_files_skips_existing_and_loads_new(self, mock_ads, mock_load, mock_logger):
        filenames = ["path/to/existing_ws.nxs", "path/to/new_ws.nxs"]

        def _does_exist(name):
            return name == "existing_ws"

        mock_ads.doesExist.side_effect = _does_exist

        ws_names = self.model.load_files(filenames)

        self.assertEqual(ws_names, ["existing_ws", "new_ws"])

        mock_logger.notice.assert_called_once_with(
            'A workspace "existing_ws" already exists, loading path/to/existing_ws.nxs has been skipped'
        )
        mock_load.assert_called_once_with(Filename="path/to/new_ws.nxs", OutputWorkspace="new_ws")
        mock_ads.doesExist.assert_has_calls([call("existing_ws"), call("new_ws")])

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_files_logs_warning_on_failure_and_continues(self, mock_ads, mock_load, mock_logger):
        filenames = ["/data/bad_ws.nxs", "/data/good_ws.nxs"]
        mock_ads.doesExist.return_value = False

        def load_side_effect(**kwargs):
            if kwargs["Filename"].endswith("bad_ws.nxs"):
                raise Exception("oops")
            return None

        mock_load.side_effect = load_side_effect

        ws_names = self.model.load_files(filenames)

        self.assertEqual(ws_names, ["bad_ws", "good_ws"])
        mock_logger.warning.assert_called_once()
        self.assertIn("Failed to load /data/bad_ws.nxs: oops", mock_logger.warning.call_args[0][0])
        self.assertEqual(mock_load.call_count, 2)

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_ref_no_path_does_nothing(self, mock_ads, mock_load, mock_logger):
        self.model.load_ref("")
        mock_ads.doesExist.assert_not_called()
        mock_load.assert_not_called()
        mock_logger.notice.assert_not_called()
        mock_logger.warning.assert_not_called()
        self.model.set_reference_ws.assert_not_called()

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_ref_skips_existing_and_sets_reference(self, mock_ads, mock_load, mock_logger):
        path = "some/dir/ref_ws.nxs"
        mock_ads.doesExist.return_value = True

        self.model.load_ref(path)

        mock_logger.notice.assert_called_once_with('A workspace "ref_ws" already exists, loading some/dir/ref_ws.nxs has been skipped')
        mock_load.assert_not_called()
        self.model.set_reference_ws.assert_called_once_with("ref_ws")

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_ref_loads_and_sets_reference_when_missing(self, mock_ads, mock_load, mock_logger):
        path = "/data/new_ref.nxs"
        mock_ads.doesExist.return_value = False

        self.model.load_ref(path)

        mock_load.assert_called_once_with(Filename="/data/new_ref.nxs", OutputWorkspace="new_ref")
        self.model.set_reference_ws.assert_called_once_with("new_ref")
        mock_logger.warning.assert_not_called()

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_ref_logs_warning_on_failure(self, mock_ads, mock_load, mock_logger):
        path = "/data/bad_ref.nxs"
        mock_ads.doesExist.return_value = False
        mock_load.side_effect = Exception("boom")

        self.model.load_ref(path)

        mock_load.assert_called_once_with(Filename="/data/bad_ref.nxs", OutputWorkspace="bad_ref")
        mock_logger.warning.assert_called_once()
        self.assertIn("Failed to load /data/bad_ref.nxs: boom", mock_logger.warning.call_args[0][0])
        self.model.set_reference_ws.assert_not_called()

    # --- try_convert_float -------------------------------------------------

    def test_try_convert_float_success(self):
        msg, val = self.model.try_convert_float("3.14", "Pi-ish")
        self.assertEqual(msg, "")
        self.assertIsInstance(val, float)
        self.assertAlmostEqual(val, 3.14)

    def test_try_convert_float_failure(self):
        msg, val = self.model.try_convert_float("not-a-number", "Field X")
        self.assertEqual(msg, "Field X must be interpretable as a float. \n")
        self.assertEqual(val, 0.0)

    # --- get_atten_args ----------------------------------------------------

    @patch(model_path + ".logger")
    def test_get_atten_args_success(self, mock_logger):
        out, err = self.model.get_atten_args(True, "1.5", "dSpacing")
        self.assertEqual(out, {"atten_val": 1.5, "atten_units": "dSpacing"})
        self.assertFalse(err)
        mock_logger.error.assert_not_called()

    @patch(model_path + ".logger")
    def test_get_atten_args_invalid_logs_and_returns_none(self, mock_logger):
        out, err = self.model.get_atten_args(True, "bad", "m")
        self.assertEqual(out, {})
        self.assertTrue(err)
        mock_logger.error.assert_called_once_with("Attenuation Evaluation Point must be interpretable as a float. \n")

    # --- get_div_args ------------------------------------------------------

    @patch(model_path + ".logger")
    def test_get_div_args_all_valid_returns_converted_values(self, mock_logger):
        out, err = self.model.get_div_args(True, "0.1", "0.2", "0.3")
        self.assertEqual(out["hoz"], 0.1)
        self.assertEqual(out["vert"], 0.2)
        self.assertEqual(out["det_hoz"], 0.3)
        self.assertFalse(err)
        mock_logger.error.assert_not_called()

    @patch(model_path + ".logger")
    def test_get_div_args_any_invalid_logs_and_returns_none(self, mock_logger):
        # Make div_vert invalid
        out, err = self.model.get_div_args(True, "1.0", "oops", "2.0")
        self.assertEqual(out, {})
        self.assertTrue(err)
        # Expect concatenated error messages to include only the failing param text
        args, _ = mock_logger.error.call_args
        self.assertIn("Vertical Divergence must be interpretable as a float.", args[0])


if __name__ == "__main__":
    unittest.main()
