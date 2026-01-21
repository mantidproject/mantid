# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock
import numpy as np
import tempfile
from Engineering.texture.polefigure.polefigure_model import TextureProjection

correction_model_path = "Engineering.texture.polefigure.polefigure_model"


class TextureProjectionTest(unittest.TestCase):
    def setUp(self):
        self.model = TextureProjection()
        self.ws_name = "test_ws"
        self.mock_ws = MagicMock()
        # Mock instrument
        mock_inst = MagicMock()
        mock_inst.getName.return_value = "instrument"
        # Mock Crystal
        self.mock_xtal = MagicMock()
        self.mock_xtal.getSpaceGroup().getHMSymbol.return_value = "P 1"
        self.mock_xtal.getScatterers.return_value = ["Fe 0.0 0.0 0.0"]

        # Mock run object to return the log
        mock_run = MagicMock()
        mock_run_number = MagicMock()
        mock_run_number.value = "123456"
        mock_grouping = MagicMock()
        mock_grouping.value = "Texture20"
        mock_log_data = {
            "run_number": mock_run_number,
            "Grouping": mock_grouping,
        }

        def get_log_data(key):
            return mock_log_data.get(key)

        mock_run.getLogData.side_effect = get_log_data

        self.mock_ws.sample().getCrystalStructure.return_value = self.mock_xtal
        self.mock_ws.sample().hasCrystalStructure.return_value = True
        self.mock_ws.getRun.return_value = mock_run
        self.mock_ws.getInstrument.return_value = mock_inst

    @patch(correction_model_path + ".ADS")
    @patch(correction_model_path + ".TextureProjection._has_no_valid_shape")
    def test_get_ws_info_with_crystal_structure(self, mock_has_no_valid_shape, mock_ads):
        mock_has_no_valid_shape.return_value = False
        mock_ads.retrieve.return_value = self.mock_ws
        result = self.model.get_ws_info("ws1", "param_file")
        self.assertEqual(result["fit_parameters"], "param_file")
        self.assertIn("Fe", result["crystal"])
        self.assertIn("P 1", result["crystal"])

    @patch(correction_model_path + ".ADS")
    def test_check_param_ws_for_columns_missing_columns(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getColumnNames.return_value = ["A", "B"]
        mock_ads.retrieve.return_value = mock_ws
        chi2, x0 = self.model.check_param_ws_for_columns(["ws1"])
        self.assertFalse(chi2)
        self.assertFalse(x0)

    @patch(correction_model_path + ".ADS")
    def test_get_pf_table_name_with_hkl(self, mock_ads):
        mock_ads.retrieve.return_value = self.mock_ws
        table_name, spectra_ws, _ = self.model.get_pf_output_names(["ws1", "ws2"], ["param_ws1", "param_ws2"], [1, 1, 1], "I")
        self.assertEqual("111_instrument_123456-123456_Texture20_pf_table_I", table_name)
        self.assertEqual("111_instrument_123456-123456_Texture20_spectra", spectra_ws)

    @patch(correction_model_path + ".ADS")
    def test_get_pf_table_name_with_no_hkl_and_no_params(self, mock_ads):
        mock_ads.retrieve.return_value = self.mock_ws
        table_name, spectra_ws, grouping = self.model.get_pf_output_names(["ws1", "ws2"], None, None, "I")
        self.assertEqual("instrument_123456-123456_Texture20_pf_table_I", table_name)
        self.assertEqual("instrument_123456-123456_Texture20_spectra", spectra_ws)

    @patch(correction_model_path + ".ADS")
    def test_get_pf_table_name_with_no_hkl_and_params(self, mock_ads):
        mock_param1, mock_param2 = MagicMock(), MagicMock()
        mock_param1.column.return_value = np.array((1, 2, 3))
        mock_param2.column.return_value = np.array((2, 2, 2))
        ads_wss = {"ws1": self.mock_ws, "ws2": self.mock_ws, "param_ws1": mock_param1, "param_ws2": mock_param2}

        def get_ads_ws(key):
            return ads_wss.get(key)

        mock_ads.retrieve.side_effect = get_ads_ws
        table_name, spectra_ws, grouping = self.model.get_pf_output_names(["ws1", "ws2"], ["param_ws1", "param_ws2"], None, "I")
        self.assertEqual("2.0_instrument_123456-123456_Texture20_pf_table_I", table_name)
        self.assertEqual("2.0_instrument_123456-123456_Texture20_spectra", spectra_ws)

    def test_parse_hkl_valid(self):
        hkl = self.model.parse_hkl("1", "2", "3")
        self.assertEqual(hkl, [1, 2, 3])

    def test_parse_hkl_invalid(self):
        hkl = self.model.parse_hkl("a", "b", "c")
        self.assertIsNone(hkl)

    @patch(correction_model_path + ".path.exists", return_value=False)
    @patch(correction_model_path + ".makedirs")
    def test_get_save_dirs_creates_directories(self, mock_makedirs, mock_exists):
        with tempfile.TemporaryDirectory() as d:
            dirs = self.model.get_save_dirs(d, "pf", "RB123", grouping="G")
        self.assertEqual(len(dirs), 2)
        mock_makedirs.assert_called()

    @patch(correction_model_path + ".SaveNexus")
    @patch(correction_model_path + ".SaveAscii")
    def test_save_files_calls_both_savers(self, mock_ascii, mock_nexus):
        with tempfile.TemporaryDirectory() as d:
            self.model._save_files("ws1", [d])
        mock_ascii.assert_called_once()
        mock_nexus.assert_called_once()

    @patch(correction_model_path + ".ADS")
    def test_read_param_cols_returns_names_and_index(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getColumnNames.return_value = ["bank", "I", "A", "B"]
        mock_ws.columnTypes.return_value = ["str", "double", "double", "double"]
        mock_ads.retrieve.return_value = mock_ws
        names, index = self.model.read_param_cols("ws1", "I")
        # the bank (string data) column should not be a valid option
        self.assertEqual(names, ["I", "A", "B"])
        self.assertEqual(index, 0)


if __name__ == "__main__":
    unittest.main()
