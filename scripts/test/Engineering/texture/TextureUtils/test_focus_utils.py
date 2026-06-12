# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock
from Engineering.texture.TextureUtils.focus_utils import run_focus_script, _get_instrument_from_ws_list

texture_utils_path = "Engineering.texture.TextureUtils.focus_utils"


class TestGetInstrumentFromWsList(unittest.TestCase):
    @patch(f"{texture_utils_path}.ADS")
    def test_single_ws_in_ads_returns_instrument_name(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = mock_ws

        result = _get_instrument_from_ws_list(["ws1"])

        mock_ads.doesExist.assert_called_once_with("ws1")
        mock_ads.retrieve.assert_called_once_with("ws1")
        self.assertEqual(result, "ENGINX")

    @patch(f"{texture_utils_path}.ADS")
    def test_multiple_ws_same_instrument_in_ads_returns_instrument_name(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = mock_ws

        result = _get_instrument_from_ws_list(["ws1", "ws2", "ws3"])

        self.assertEqual(result, "ENGINX")
        self.assertEqual(mock_ads.retrieve.call_count, 3)

    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_ws_not_in_ads_loads_from_file(self, mock_ads, mock_load):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "IMAT"
        mock_ads.doesExist.return_value = False
        mock_load.return_value = mock_ws

        result = _get_instrument_from_ws_list(["path/to/file.nxs"])

        mock_load.assert_called_once_with(Filename="path/to/file.nxs")
        self.assertEqual(result, "IMAT")

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_ws_not_loadable_logs_error_and_returns_none(self, mock_ads, mock_load, mock_logger):
        mock_ads.doesExist.return_value = False
        mock_load.side_effect = RuntimeError("Cannot load file")

        result = _get_instrument_from_ws_list(["nonexistent.nxs"])

        mock_logger.error.assert_called_once()
        self.assertIn("nonexistent.nxs", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.ADS")
    def test_multiple_instruments_logs_error_and_returns_none(self, mock_ads, mock_logger):
        mock_ws1 = MagicMock()
        mock_ws1.getInstrument().getName.return_value = "ENGINX"
        mock_ws2 = MagicMock()
        mock_ws2.getInstrument().getName.return_value = "IMAT"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.side_effect = [mock_ws1, mock_ws2]

        result = _get_instrument_from_ws_list(["ws1", "ws2"])

        mock_logger.error.assert_called_once()
        self.assertIn("multiple different instruments", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    def test_empty_list_logs_error_and_returns_none(self, mock_logger):
        result = _get_instrument_from_ws_list([])

        mock_logger.error.assert_called_once()
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_load_failure_stops_processing_remaining_ws(self, mock_ads, mock_load, mock_logger):
        # If the second ws fails to load, processing stops early and returns None.
        mock_ws1 = MagicMock()
        mock_ws1.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.side_effect = [True, False]
        mock_ads.retrieve.return_value = mock_ws1
        mock_load.side_effect = RuntimeError("Cannot load")

        result = _get_instrument_from_ws_list(["ws1", "bad_file.nxs"])

        mock_logger.error.assert_called_once()
        self.assertIn("bad_file.nxs", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)


class TextureUtilsFocusTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.IMAT")
    @patch(f"{texture_utils_path}.EnginX")
    @patch(f"{texture_utils_path}._get_instrument_from_ws_list", return_value="ENGINX")
    def test_run_focus_script_instantiates_ENGINX_model_and_calls_main(self, mock_get_instr, mock_enginx, mock_imat, mock_mk):
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_enginx.return_value.main.assert_called_once()
        mock_imat.return_value.main.assert_not_called()
        mock_mk.assert_called_once_with("focus")

    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.IMAT")
    @patch(f"{texture_utils_path}.EnginX")
    @patch(f"{texture_utils_path}._get_instrument_from_ws_list", return_value="IMAT")
    def test_run_focus_script_instantiates_IMAT_model_and_calls_main(self, mock_get_instr, mock_enginx, mock_imat, mock_mk):
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_enginx.return_value.main.assert_not_called()
        mock_imat.return_value.main.assert_called_once()
        mock_mk.assert_called_once_with("focus")


if __name__ == "__main__":
    unittest.main()
