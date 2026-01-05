# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call, mock_open
from mantid.api import AnalysisDataService as ADS
from Engineering.texture.TextureUtils import load_all_orientations
from Engineering.texture.texture_helper import _read_xml, create_default_parameter_table_with_value, get_pole_figure_data
import numpy as np

texture_utils_path = "Engineering.texture.texture_helper"


class TestTextureHelperSetGoniometer(unittest.TestCase):
    def setUp(self):
        self.ws_name = "test_ws"
        # Mock workspace
        self.mock_ws = MagicMock()

    def tearDown(self):
        if ADS.doesExist(self.ws_name):
            ADS.remove(self.ws_name)

    def run_euler_gonio_test(self, test_wss, txt_data, euler_scheme, euler_sense, target_calls, mock_set_gonio):
        mock_set_gonio.reset_mock()
        with patch("builtins.open", mock_open(read_data=txt_data)):
            load_all_orientations(test_wss, "angles.txt", use_euler=True, euler_scheme=euler_scheme, euler_sense=euler_sense)

        self.assertEqual(mock_set_gonio.call_count, 2)
        expected_calls = [call(self.mock_ws, **target_call) for target_call in target_calls]
        mock_set_gonio.assert_has_calls(expected_calls)

    @patch(texture_utils_path + ".SetGoniometer")
    def test_load_all_orientations_using_euler_angles(self, mock_set_gonio):
        test_wss = [self.mock_ws, self.mock_ws]
        txt_data = "30,45,60\n15,30,45"
        euler_schemes = [["x", "y", "z"], ["y", "x", "y"]]  # try two different rot schemes
        euler_senses = ["1,1,-1", "-1,-1,1"]  # and two sets of sense
        target_calls = [
            (
                {"Axis0": "30,1,0,0,1", "Axis1": "45,0,1,0,1", "Axis2": "60,0,0,1,-1"},
                {"Axis0": "15,1,0,0,1", "Axis1": "30,0,1,0,1", "Axis2": "45,0,0,1,-1"},
            ),
            (
                {"Axis0": "30,0,1,0,-1", "Axis1": "45,1,0,0,-1", "Axis2": "60,0,1,0,1"},
                {"Axis0": "15,0,1,0,-1", "Axis1": "30,1,0,0,-1", "Axis2": "45,0,1,0,1"},
            ),
        ]  # expected rot axes
        for i in range(2):
            self.run_euler_gonio_test(test_wss, txt_data, euler_schemes[i], euler_senses[i], target_calls[i], mock_set_gonio)

    @patch(texture_utils_path + ".SetGoniometer")
    def test_load_all_orientations_using_rot_mat(self, mock_set_gonio):
        test_wss = [self.mock_ws, self.mock_ws]
        txt_data = "1,0,0,0,1,0,0,0,1,2,2,2\n0,1,0,0,0,1,1,0,0,2,2,2"

        target_calls = (
            {"GoniometerMatrix": [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]},
            {"GoniometerMatrix": [0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0]},
        )

        with patch("builtins.open", mock_open(read_data=txt_data)):
            load_all_orientations(test_wss, "rot_mats.txt", use_euler=False)

        self.assertEqual(mock_set_gonio.call_count, 2)
        expected_calls = [call(self.mock_ws, **target_call) for target_call in target_calls]
        mock_set_gonio.assert_has_calls(expected_calls)

    @patch(texture_utils_path + ".logger")
    def test_load_all_orientations_error_handling_when_matrix_given_to_euler(self, mock_logger):
        test_wss = [self.mock_ws, self.mock_ws]
        txt_data = "1,0,0,0,1,0,0,0,1,2,2,2\n0,1,0,0,0,1,1,0,0,2,2,2"

        with patch("builtins.open", mock_open(read_data=txt_data)):
            load_all_orientations(test_wss, "rot_mats.txt", use_euler=True, euler_scheme=["x", "y", "z"], euler_sense="1,1,1")

        mock_logger.error.assert_called_once_with(
            "list index out of range. Failed to set goniometer, are your settings for `use_euler_angles` correct? Currently: True"
        )

    @patch(texture_utils_path + ".logger")
    def test_load_all_orientations_error_handling_when_euler_given_as_matrix(self, mock_logger):
        test_wss = [self.mock_ws, self.mock_ws]
        txt_data = "30,45,60\n15,30,45"

        with patch("builtins.open", mock_open(read_data=txt_data)):
            load_all_orientations(test_wss, "rot_mats.txt", use_euler=False, euler_scheme=["x", "y", "z"], euler_sense="1,1,1")

        mock_logger.error.assert_called_once_with(
            'Problem setting "GoniometerMatrix" in SetGoniometer-v1: '
            'When converting parameter "GoniometerMatrix": '
            'When setting value of property "GoniometerMatrix": '
            "Incorrect size. Failed to set goniometer, "
            "are your settings for `use_euler_angles` correct? "
            "Currently: False"
        )

    @patch("builtins.open", new_callable=mock_open, read_data="<xml>test</xml>")
    @patch(texture_utils_path + "._validate_file", return_value=True)
    def test_read_xml_reads_file_when_valid(self, mock_validate, mock_file):
        result = _read_xml("dummy.xml")

        self.assertEqual(result, "<xml>test</xml>")
        mock_validate.assert_called_once_with("dummy.xml", ".xml")
        mock_file.assert_called_once_with("dummy.xml", "r")

    @patch(texture_utils_path + ".ADS")
    def test_create_default_parameter_table_with_value(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getNumberHistograms.return_value = 2
        mock_ads.retrieve.return_value = mock_ws
        with patch(texture_utils_path + ".CreateEmptyTableWorkspace") as mock_create:
            mock_table = MagicMock()
            mock_create.return_value = mock_table
            create_default_parameter_table_with_value("ws", 5.0, "out_ws")
            self.assertEqual(mock_table.addRow.call_count, 2)

    @patch(texture_utils_path + ".ADS")
    def test_get_pole_figure_data_stereographic_projection(self, mock_ads):
        mock_ws = MagicMock()

        col_data = {"Alpha": np.array([0.1, 0.2]), "Beta": np.array([0.1, 0.2]), "I": np.array([0.1, 0.2])}

        def get_column(col):
            return col_data.get(col)

        mock_ws.column.side_effect = get_column
        mock_ads.retrieve.return_value = mock_ws

        result = get_pole_figure_data("ws", "stereographic")
        self.assertEqual(result.shape[1], 3)
