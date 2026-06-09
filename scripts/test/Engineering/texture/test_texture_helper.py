# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
import tempfile
import os
from unittest.mock import patch, MagicMock, call, mock_open
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateWorkspace, ConjoinWorkspaces
from Engineering.texture.texture_helper import (
    load_all_orientations,
    _read_xml,
    _validate_file,
    create_default_parameter_table_with_value,
    get_pole_figure_data,
    show_texture_sample_shape,
    get_gauge_vol_str,
    create_pole_figure_tables,
    plot_pole_figure,
    _retrieve_ws_object,
    get_debug_info,
    save_texture_ws_ascii,
    generous_rebin,
    ster_proj,
    azim_proj,
    ster_proj_xy,
    azim_proj_xy,
    get_alpha_beta_from_cart,
    ring,
    project_orientation,
    vec_string_to_norm_array,
    define_gauge_volume,
    get_scattering_centre,
)
import numpy as np
from os import path
from scipy.spatial.transform import Rotation

texture_utils_path = "Engineering.texture.texture_helper"


class BaseTextureTestClass(unittest.TestCase):
    def setUp(self):
        self.ws_name = "test_ws"
        # Mock workspace
        self.mock_ws = MagicMock()
        self.mock_ws.name.return_value = self.ws_name

    def tearDown(self):
        if ADS.doesExist(self.ws_name):
            ADS.remove(self.ws_name)


class TestHelperGaugeVolume(BaseTextureTestClass):
    @patch(texture_utils_path + ".get_cube_xml", return_value="example 4mm cube xml")
    def test_get_gauge_volume_string_with_4mm_cube_preset(self, mock_get_cube_xml):
        output_xml = get_gauge_vol_str(preset="4mmCube")
        mock_get_cube_xml.assert_called_once_with("some-gv", 0.004)
        self.assertEqual(output_xml, "example 4mm cube xml")

    @patch(texture_utils_path + "._read_xml", return_value="example xml")
    def test_get_gauge_volume_string_with_custom_xml(self, mock_read_xml):
        file = "example.xml"
        output_xml = get_gauge_vol_str(preset="Custom", custom_file=file)
        mock_read_xml.assert_called_once_with(file)
        self.assertEqual(output_xml, "example xml")

    def test_get_gauge_volume_string_with_no_gauge_volume(self):
        output_xml = get_gauge_vol_str(preset="No Gauge Volume")
        self.assertEqual(output_xml, None)

    @patch(texture_utils_path + "._read_xml", side_effect=RuntimeError("bad xml"))
    def test_get_gauge_vol_str_custom_runtimeerror_returns_none(self, mock_read_xml):
        out = get_gauge_vol_str(preset="Custom", custom_file="bad.xml")
        self.assertIsNone(out)
        mock_read_xml.assert_called_once_with("bad.xml")


class TestHelperFileHandling(BaseTextureTestClass):
    def test_validate_file_valid(self):
        valid_args = [("path.txt", ".txt"), ("long/path.txt", ".txt"), ("long/path.nxs", ".nxs")]

        for file, ext in valid_args:
            with self.subTest(file=file, ext=ext):
                self.assertTrue(_validate_file(file, ext))

    def test_validate_file_invalid(self):
        cases = [
            (None, ".txt"),
            ("", ".txt"),
            ("path.txt", ".xml"),
            ("path", ".txt"),
            ("path.TXT", ".txt"),
        ]
        for file, ext in cases:
            with self.subTest(file=file, ext=ext):
                self.assertFalse(_validate_file(file, ext))

    @patch("builtins.open", new_callable=mock_open, read_data="<xml>test</xml>")
    @patch(texture_utils_path + "._validate_file", return_value=True)
    def test_read_xml_reads_file_when_valid(self, mock_validate, mock_file):
        result = _read_xml("dummy.xml")

        self.assertEqual(result, "<xml>test</xml>")
        mock_validate.assert_called_once_with("dummy.xml", ".xml")
        mock_file.assert_called_once_with("dummy.xml", "r")

    @patch(texture_utils_path + "._validate_file", return_value=False)
    def test_read_xml_returns_none_when_invalid(self, mock_validate):
        self.assertIsNone(_read_xml("bad.xml"))
        mock_validate.assert_called_once_with("bad.xml", ".xml")

    @patch(texture_utils_path + ".ShowSampleModel")
    def test_show_texture_sample_no_gauge_volume(self, mock_model_constructor):
        mock_model = MagicMock()
        mock_model_constructor.return_value = mock_model

        ax_transform = np.eye(3)
        ax_labels = ("d1", "d2", "d3")
        show_texture_sample_shape(self.mock_ws, ax_transform, ax_labels, gauge_vol_preset=None)

        mock_model.set_ws_name.assert_called_once_with(self.ws_name)
        mock_model.set_fix_axes_to_sample.assert_called_once_with(True)
        mock_model.set_gauge_vol_str.assert_not_called()
        mock_model.show_shape_plot.assert_called_once_with(ax_transform, ax_labels)


class TestHelperShowSample(BaseTextureTestClass):
    @patch(texture_utils_path + ".get_gauge_vol_str", return_value="example xml")
    @patch(texture_utils_path + ".ShowSampleModel")
    def test_show_texture_sample_with_gauge_volume(self, mock_model_constructor, mock_gauge_vol_string):
        mock_model = MagicMock()
        mock_model_constructor.return_value = mock_model

        ax_transform = np.eye(3)
        ax_labels = ("d1", "d2", "d3")
        preset = "Custom"
        file = "custom.xml"
        show_texture_sample_shape(self.mock_ws, ax_transform, ax_labels, gauge_vol_preset=preset, custom_file=file)

        mock_model.set_ws_name.assert_called_once_with(self.ws_name)
        mock_model.set_fix_axes_to_sample.assert_called_once_with(True)
        mock_gauge_vol_string.assert_called_once_with(preset, file)
        mock_model.set_gauge_vol_str.assert_called_once_with("example xml")
        mock_model.show_shape_plot.assert_called_once_with(ax_transform, ax_labels)


class TestHelperSetOrientations(BaseTextureTestClass):
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

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + "._validate_file", return_value=True)
    def test_load_all_orientations_no_workspaces_warns(self, mock_validate, mock_logger):
        with patch("builtins.open", mock_open(read_data="30,45,60\n")):
            load_all_orientations([], "angles.txt", use_euler=True, euler_scheme=["x", "y", "z"], euler_sense="1,1,1")
        mock_logger.warning.assert_called_once()
        self.assertIn("No workspaces have been provided", mock_logger.warning.call_args[0][0])

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + "._validate_file", return_value=True)
    def test_load_all_orientations_fewer_workspaces_than_lines_warns(self, mock_validate, mock_logger):
        with patch("builtins.open", mock_open(read_data="30,45,60\n15,30,45\n")):
            load_all_orientations([self.mock_ws], "angles.txt", use_euler=True, euler_scheme=["x", "y", "z"], euler_sense="1,1,1")
        # warns about ignoring extra lines
        msg = mock_logger.warning.call_args_list[-1][0][0]
        self.assertIn("will be ignored", msg)


class TestHelperPoleFigureTables(BaseTextureTestClass):
    @staticmethod
    def get_create_pf_kwargs():
        return {
            "out_ws": "out_ws",
            "combined_ws": "combined_ws",
            "hkl": [1, 1, 1],
            "inc_scatt_corr": True,
            "scat_vol_pos": [0, 0, 0],
            "chi2_thresh": 2.0,
            "peak_thresh": 0.1,
            "ax_transform": np.eye(3),
            "readout_col": "I",
            "include_spec_info": False,
        }

    @patch(texture_utils_path + ".ADS")
    @patch(texture_utils_path + ".ConjoinWorkspaces")
    @patch(texture_utils_path + ".RebinToWorkspace")
    @patch(texture_utils_path + ".CombineTableWorkspaces")
    @patch(texture_utils_path + ".CloneWorkspace")
    @patch(texture_utils_path + ".CreatePoleFigureTableWorkspace")
    def test_create_pole_figure_tables_with_peak_wss(self, mock_create_pf, mock_clone, mock_combine, mock_rebin, mock_conjoin, mock_ads):
        mock_ads.retrieve.return_value = MagicMock()  # returned output table
        out = create_pole_figure_tables(wss=["ws1", "ws2"], peak_wss=["p1", "p2"], **self.get_create_pf_kwargs())
        self.assertEqual(mock_create_pf.call_count, 2)
        clone_calls = [
            {"InputWorkspace": "_0_abi_table", "OutputWorkspace": "out_ws"},
            {"InputWorkspace": "_0_spec_ws", "OutputWorkspace": "combined_ws"},
        ]
        mock_clone.assert_has_calls([call(**target_call) for target_call in clone_calls])
        mock_combine.assert_called_once_with(LHSWorkspace="out_ws", RHSWorkspace="_1_abi_table", OutputWorkspace="out_ws")
        mock_rebin.assert_called_once_with(WorkspaceToRebin="_1_spec_ws", WorkspaceToMatch="_0_spec_ws", OutputWorkspace="_1_spec_ws")
        mock_conjoin.assert_called_once_with(
            InputWorkspace1="combined_ws", InputWorkspace2="_1_spec_ws", CheckOverlapping=False, CheckMatchingBins=False
        )
        mock_ads.retrieve.assert_called_once_with("out_ws")
        self.assertIsNotNone(out)

    @patch(texture_utils_path + ".create_default_parameter_table_with_value")
    @patch(texture_utils_path + ".ADS")
    @patch(texture_utils_path + ".ConjoinWorkspaces")
    @patch(texture_utils_path + ".RebinToWorkspace")
    @patch(texture_utils_path + ".CombineTableWorkspaces")
    @patch(texture_utils_path + ".CloneWorkspace")
    @patch(texture_utils_path + ".CreatePoleFigureTableWorkspace")
    def test_create_pole_figure_tables_without_peak_wss_uses_defaults(
        self, mock_create_pf, mock_clone, mock_combine, mock_rebin, mock_conjoin, mock_ads, mock_default
    ):
        mock_ads.retrieve.return_value = MagicMock()
        out = create_pole_figure_tables(wss=["ws1", "ws2", "ws3"], peak_wss=None, **self.get_create_pf_kwargs())
        # default table created once per workspace
        self.assertEqual(mock_default.call_count, 3)
        self.assertEqual(mock_create_pf.call_count, 3)
        clone_calls = [
            {"InputWorkspace": "_0_abi_table", "OutputWorkspace": "out_ws"},
            {"InputWorkspace": "_0_spec_ws", "OutputWorkspace": "combined_ws"},
        ]
        mock_clone.assert_has_calls([call(**target_call) for target_call in clone_calls])
        self.assertEqual(mock_combine.call_count, 2)
        self.assertEqual(mock_rebin.call_count, 2)
        self.assertEqual(mock_conjoin.call_count, 2)
        mock_ads.retrieve.assert_called_once_with("out_ws")
        self.assertIsNotNone(out)

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

        result = get_pole_figure_data(mock_ws, "stereographic")
        self.assertEqual(result.shape[1], 3)

    @patch(texture_utils_path + ".ADS")
    @patch(texture_utils_path + ".CloneWorkspace")
    @patch(texture_utils_path + ".CombineTableWorkspaces")
    @patch(texture_utils_path + ".CreatePoleFigureTableWorkspace")
    def test_include_spec_info_true_is_passed_through(self, mock_create, mock_combine, mock_clone, mock_ads):
        mock_ads.retrieve.return_value = MagicMock()
        create_pole_figure_tables(
            wss=["ws1"],
            peak_wss=["p1"],
            out_ws="out_ws",
            combined_ws=None,
            include_spec_info=True,
        )
        # Ensure IncludeSpectrumInfo=True passed to algorithm
        _, kwargs = mock_create.call_args
        self.assertIn("IncludeSpectrumInfo", kwargs)
        self.assertTrue(kwargs["IncludeSpectrumInfo"])

    @patch(texture_utils_path + ".create_default_parameter_table_with_value")
    @patch(texture_utils_path + ".ADS")
    @patch(texture_utils_path + ".CloneWorkspace")
    @patch(texture_utils_path + ".CombineTableWorkspaces")
    @patch(texture_utils_path + ".CreatePoleFigureTableWorkspace")
    def test_peak_wss_length_mismatch_falls_back_to_default_tables(self, mock_create, mock_combine, mock_clone, mock_ads, mock_default):
        mock_ads.retrieve.return_value = MagicMock()
        # if peak_wss provided but wrong length then the peak_ws should be ignored
        create_pole_figure_tables(
            wss=["ws1", "ws2"],
            peak_wss=["p1"],  # mismatch
            out_ws="out_ws",
            combined_ws=None,
        )
        mock_default.assert_has_calls([call("ws1", 1, "_default_param_table"), call("ws2", 2, "_default_param_table")])
        _, kwargs = mock_create.call_args
        self.assertEqual(kwargs["PeakParameterWorkspace"], "_default_param_table")


class TestHelperPoleFigurePlots(BaseTextureTestClass):
    @patch(texture_utils_path + "._retrieve_ws_object")
    @patch(texture_utils_path + ".get_pole_figure_data")
    @patch(texture_utils_path + ".get_debug_info", return_value=["row0", "row1"])
    @patch(texture_utils_path + ".plot_exp_pf")
    def test_plot_pole_figure_scatter_calls_plot_exp(self, mock_plot_exp, mock_get_debug, mock_get_data, mock_retrieve):
        test_pfi = np.ones((2, 3))
        mock_get_data.return_value = test_pfi
        ws_obj = MagicMock()
        ws_obj.name.return_value = "wsname"
        mock_retrieve.return_value = ws_obj
        mock_plot_exp.return_value = (MagicMock(), MagicMock())

        plot_pole_figure("ws", "stereographic", plot_exp=True, save_dirs=None, fig=None, readout_col="I")

        mock_get_debug.assert_called_once_with(ws_obj)
        mock_plot_exp.assert_called_once_with(test_pfi, ("Dir1", "Dir2"), "I", None, ["row0", "row1"])

    @patch(texture_utils_path + "._retrieve_ws_object")
    @patch(texture_utils_path + ".get_pole_figure_data")
    @patch(texture_utils_path + ".plot_contour_pf")
    def test_plot_pole_figure_contour_calls_plot_contour(self, mock_plot_contour, mock_get, mock_retrieve):
        test_pfi = np.ones((2, 3))
        mock_get.return_value = test_pfi
        mock_retrieve.return_value = self.mock_ws
        fig = MagicMock()
        ax = MagicMock()
        mock_plot_contour.return_value = (fig, ax)
        plot_pole_figure(self.mock_ws, "stereographic", plot_exp=False, save_dirs="outdir")
        mock_plot_contour.assert_called_once_with(test_pfi, ("Dir1", "Dir2"), "I", None, 2.0)
        fig.savefig.assert_called_once_with(str(path.join("outdir", "test_ws_contour_2.0.png")))

    @patch(texture_utils_path + "._retrieve_ws_object")
    @patch(texture_utils_path + ".get_pole_figure_data", return_value=np.array([[0.0, 0.0, 1.0]]))
    @patch(texture_utils_path + ".plot_exp_pf")
    def test_plot_pole_figure_save_dirs_string_only_saves_once(self, mock_plot_exp, mock_get, mock_retrieve):
        fig = MagicMock()
        ax = MagicMock()
        mock_plot_exp.return_value = (fig, ax)
        mock_retrieve.return_value = self.mock_ws
        plot_pole_figure(self.mock_ws, "stereographic", plot_exp=True, save_dirs="outdir")
        fig.savefig.assert_called_once_with(str(path.join("outdir", "test_ws_scatter.png")))

    @patch(texture_utils_path + "._retrieve_ws_object")
    @patch(texture_utils_path + ".get_pole_figure_data", return_value=np.ones((1, 3)))
    @patch(texture_utils_path + ".get_debug_info")
    @patch(texture_utils_path + ".plot_exp_pf", return_value=(MagicMock(), MagicMock()))
    def test_plot_pole_figure_display_debug_info_false_does_not_call_get_debug_info(
        self, mock_plot_exp, mock_get_debug, mock_get_data, mock_retrieve
    ):
        ws_obj = MagicMock()
        ws_obj.name.return_value = "wsname"
        mock_retrieve.return_value = ws_obj

        plot_pole_figure("ws", "stereographic", display_debug_info=False, plot_exp=True)

        mock_get_debug.assert_not_called()
        # debug_info arg should be None
        args, _ = mock_plot_exp.call_args
        self.assertIsNone(args[4])


class TestRetrieveWSObject(BaseTextureTestClass):
    @patch(texture_utils_path + ".ADS")
    def test_retrieve_ws_object_str_uses_ads(self, mock_ads):
        mock_ads.retrieve.return_value = self.mock_ws
        out = _retrieve_ws_object(self.ws_name)
        self.assertIs(out, self.mock_ws)
        mock_ads.retrieve.assert_called_once_with(self.ws_name)

    def test_retrieve_ws_object_non_str_returns_input(self):
        self.assertIs(_retrieve_ws_object(self.mock_ws), self.mock_ws)


class MockTable:
    def __init__(self, rows):
        self._rows = rows

    def rowCount(self):
        return len(self._rows)

    def row(self, i):
        return self._rows[i]


class TestDebugInfo(unittest.TestCase):
    def test_get_debug_info_formats_rows(self):
        ws = MockTable([{"Alpha": 1.0, "Beta": 2.0, "I": 1.0}, {"Alpha": 1.0, "Beta": 0.0, "I": 2.0}])
        labels = get_debug_info(ws)
        self.assertEqual(len(labels), 2)
        self.assertEqual("Alpha: 1.0, Beta: 2.0, I: 1.0", labels[0])
        self.assertEqual("Alpha: 1.0, Beta: 0.0, I: 2.0", labels[1])


class TestSaveTextureWsAscii(unittest.TestCase):
    def setUp(self):
        self.ws_name = "test_ws"

    @patch(texture_utils_path + ".SaveAscii")
    def test_save_calls_save_ascii_with_correct_args(self, mock_save_ascii):
        save_texture_ws_ascii(self.ws_name, "/some/dir")
        mock_save_ascii.assert_called_once_with(
            InputWorkspace=self.ws_name,
            Filename=path.join("/some/dir", self.ws_name + ".txt"),
            Separator="Tab",
        )

    @patch(texture_utils_path + ".SaveAscii")
    def test_save_does_not_rebin_on_success(self, mock_save_ascii):
        with patch(texture_utils_path + ".generous_rebin") as mock_rebin:
            save_texture_ws_ascii(self.ws_name, "/some/dir")
            mock_rebin.assert_not_called()

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".generous_rebin")
    @patch(texture_utils_path + ".SaveAscii")
    def test_save_calls_generous_rebin_and_retries_on_runtime_error(self, mock_save_ascii, mock_rebin, mock_logger):
        mock_save_ascii.side_effect = [RuntimeError("bin mismatch"), None]
        mock_ascii_ws = MagicMock()
        mock_rebin.return_value = mock_ascii_ws

        save_texture_ws_ascii(self.ws_name, "/some/dir")

        mock_rebin.assert_called_once_with(self.ws_name, "ascii_ws", False)
        self.assertEqual(mock_save_ascii.call_count, 2)
        mock_save_ascii.assert_called_with(
            InputWorkspace=mock_ascii_ws,
            Filename=path.join("/some/dir", self.ws_name + ".txt"),
            Separator="Tab",
        )

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".generous_rebin", return_value=MagicMock())
    @patch(texture_utils_path + ".SaveAscii", side_effect=[RuntimeError("bin mismatch"), None])
    def test_save_passes_store_in_ads_to_generous_rebin(self, mock_save_ascii, mock_rebin, mock_logger):
        save_texture_ws_ascii(self.ws_name, "/some/dir", StoreInADS=True)
        mock_rebin.assert_called_once_with(self.ws_name, "ascii_ws", True)

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".generous_rebin", return_value=MagicMock())
    @patch(texture_utils_path + ".SaveAscii", side_effect=[RuntimeError("bin mismatch"), None])
    def test_save_logs_notice_on_runtime_error(self, mock_save_ascii, mock_rebin, mock_logger):
        save_texture_ws_ascii(self.ws_name, "/some/dir")
        mock_logger.notice.assert_called_once()
        self.assertIn(self.ws_name, mock_logger.notice.call_args[0][0])


class TestGenerousRebin(unittest.TestCase):
    def _make_mock_ws(self, x_arrays, instrument_name="ENGIN-X"):
        mock_ws = MagicMock()
        mock_ws.getNumberHistograms.return_value = len(x_arrays)
        mock_ws.readX.side_effect = lambda i: np.array(x_arrays[i])
        mock_ws.getInstrument.return_value.getName.return_value = instrument_name
        return mock_ws

    @patch(texture_utils_path + ".Rebin")
    @patch(texture_utils_path + "._retrieve_ws_object")
    def test_generous_rebin_calls_rebin_with_correct_params(self, mock_retrieve, mock_rebin):
        x_arrays = [[1.0, 2.0, 4.0], [1.5, 2.5, 3.5]]
        mock_ws = self._make_mock_ws(x_arrays)
        mock_retrieve.return_value = mock_ws
        mock_rebin.return_value = MagicMock()

        generous_rebin(mock_ws, "out_ws")

        expected_min = 1.0
        expected_max = 4.0
        expected_diff = 1.0
        mock_rebin.assert_called_once_with(
            InputWorkspace=mock_ws,
            Params=(expected_min, expected_diff, expected_max),
            OutputWorkspace="out_ws",
            StoreInADS=True,
        )

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".Rebin", return_value=MagicMock())
    @patch(texture_utils_path + "._retrieve_ws_object")
    def test_generous_rebin_warns_for_non_enginx_imat_instrument(self, mock_retrieve, mock_rebin, mock_logger):
        mock_ws = self._make_mock_ws([[1.0, 2.0, 3.0]], instrument_name="WISH")
        mock_retrieve.return_value = mock_ws

        generous_rebin(mock_ws, "out_ws")

        mock_logger.warning.assert_called_once()

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".Rebin", return_value=MagicMock())
    @patch(texture_utils_path + "._retrieve_ws_object")
    def test_generous_rebin_does_not_warn_for_enginx(self, mock_retrieve, mock_rebin, mock_logger):
        mock_ws = self._make_mock_ws([[1.0, 2.0, 3.0]], instrument_name="ENGIN-X")
        mock_retrieve.return_value = mock_ws

        generous_rebin(mock_ws, "out_ws")

        mock_logger.warning.assert_not_called()

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".Rebin", return_value=MagicMock())
    @patch(texture_utils_path + "._retrieve_ws_object")
    def test_generous_rebin_does_not_warn_for_imat(self, mock_retrieve, mock_rebin, mock_logger):
        mock_ws = self._make_mock_ws([[1.0, 2.0, 3.0]], instrument_name="IMAT")
        mock_retrieve.return_value = mock_ws

        generous_rebin(mock_ws, "out_ws")

        mock_logger.warning.assert_not_called()

    @patch(texture_utils_path + ".Rebin")
    @patch(texture_utils_path + "._retrieve_ws_object")
    def test_generous_rebin_passes_store_in_ads(self, mock_retrieve, mock_rebin):
        mock_ws = self._make_mock_ws([[1.0, 2.0, 3.0]])
        mock_retrieve.return_value = mock_ws
        mock_rebin.return_value = MagicMock()

        generous_rebin(mock_ws, "out_ws", StoreInADS=False)

        _, kwargs = mock_rebin.call_args
        self.assertFalse(kwargs["StoreInADS"])


class TestGenerousRebinSystem(unittest.TestCase):
    def tearDown(self):
        for name in ["__spec_a", "__spec_b", "rebinned_ws"]:
            if ADS.doesExist(name):
                ADS.remove(name)

    def test_generous_rebin_produces_uniform_bins_spanning_full_x_range(self):
        # spec 0: X=[1, 2, 3]      -> min_diff=1.0, range=[1,3]
        # spec 1: X=[1.5, 2.0, 2.5, 3.0] -> min_diff=0.5, range=[1.5,3.0]
        # expected: step=0.5, range=[1.0, 3.0] -> bins [1.0,1.5,2.0,2.5,3.0]
        CreateWorkspace(DataX=[1.0, 2.0, 3.0], DataY=[1.0, 2.0], NSpec=1, OutputWorkspace="__spec_a")
        CreateWorkspace(DataX=[1.5, 2.0, 2.5, 3.0], DataY=[1.0, 2.0, 3.0], NSpec=1, OutputWorkspace="__spec_b")
        ConjoinWorkspaces(
            InputWorkspace1="__spec_a",
            InputWorkspace2="__spec_b",
            CheckOverlapping=False,
            CheckMatchingBins=False,
        )
        ws_in = ADS.retrieve("__spec_a")

        result = generous_rebin(ws_in, "rebinned_ws", StoreInADS=True)

        self.assertEqual(result.getNumberHistograms(), 2)
        x0 = result.readX(0)
        x1 = result.readX(1)
        np.testing.assert_array_equal(x0, x1)
        self.assertAlmostEqual(x0[0], 1.0)
        self.assertAlmostEqual(x0[-1], 3.0)
        np.testing.assert_allclose(np.diff(x0), 0.5, rtol=1e-6)


class TestSaveTextureWsAsciiSystem(unittest.TestCase):
    def setUp(self):
        self.save_dir = tempfile.mkdtemp()
        self.ws_name = "test_ws"

    def tearDown(self):
        for name in [self.ws_name, "ascii_ws", "__spec_a", "__spec_b"]:
            if ADS.doesExist(name):
                ADS.remove(name)
        for f in os.listdir(self.save_dir):
            os.remove(os.path.join(self.save_dir, f))
        os.rmdir(self.save_dir)

    def test_save_uniform_bins_creates_file(self):
        # Workspace with matching X bins on all spectra — SaveAscii succeeds directly.
        x = [1.0, 2.0, 3.0, 1.0, 2.0, 3.0]
        y = [1.0, 2.0, 1.0, 2.0]
        CreateWorkspace(DataX=x, DataY=y, NSpec=2, OutputWorkspace=self.ws_name)

        save_texture_ws_ascii(self.ws_name, self.save_dir)

        expected = os.path.join(self.save_dir, self.ws_name + ".txt")
        self.assertTrue(os.path.isfile(expected), f"Expected output file not found: {expected}")

    def test_save_ragged_bins_creates_file_via_rebin_fallback(self):
        # Workspace with mismatched bin counts triggers generous_rebin
        CreateWorkspace(DataX=[1.0, 2.0, 3.0], DataY=[1.0, 2.0], NSpec=1, OutputWorkspace="__spec_a")
        CreateWorkspace(DataX=[1.0, 2.0, 3.0, 4.0], DataY=[1.0, 2.0, 3.0], NSpec=1, OutputWorkspace="__spec_b")
        ConjoinWorkspaces(
            InputWorkspace1="__spec_a",
            InputWorkspace2="__spec_b",
            CheckOverlapping=False,
            CheckMatchingBins=False,
        )

        save_texture_ws_ascii("__spec_a", self.save_dir)

        expected = os.path.join(self.save_dir, "__spec_a.txt")
        self.assertTrue(os.path.isfile(expected), f"Expected output file not found: {expected}")


class TestProjections(unittest.TestCase):
    def test_ster_proj_equator_maps_to_unit_circle(self):
        # beta = pi/2 (equator) should map to radius 1
        alphas = np.array([0.0, np.pi / 2])
        betas = np.array([np.pi / 2, np.pi / 2])
        i = np.array([5.0, 7.0])

        out = ster_proj(alphas, betas, i)

        self.assertEqual(out.shape, (2, 3))
        # alpha=0 -> (1, 0), alpha=pi/2 -> (0, 1)
        np.testing.assert_allclose(out[0, :2], [1.0, 0.0], atol=1e-12)
        np.testing.assert_allclose(out[1, :2], [0.0, 1.0], atol=1e-12)
        # intensity column is carried through unchanged
        np.testing.assert_array_equal(out[:, 2], i)

    def test_ster_proj_north_pole_maps_to_origin(self):
        # beta = 0 maps to the centre of the projection
        out = ster_proj(np.array([0.0]), np.array([0.0]), np.array([1.0]))
        np.testing.assert_allclose(out[0, :2], [0.0, 0.0], atol=1e-12)

    def test_azim_proj_equator_maps_to_unit_circle(self):
        alphas = np.array([0.0, np.pi / 2])
        betas = np.array([np.pi / 2, np.pi / 2])
        i = np.array([5.0, 7.0])

        out = azim_proj(alphas, betas, i)

        self.assertEqual(out.shape, (2, 3))
        np.testing.assert_allclose(out[0, :2], [1.0, 0.0], atol=1e-12)
        np.testing.assert_allclose(out[1, :2], [0.0, 1.0], atol=1e-12)
        np.testing.assert_array_equal(out[:, 2], i)

    def test_azim_proj_origin_maps_to_centre(self):
        out = azim_proj(np.array([0.0]), np.array([0.0]), np.array([3.0]))
        np.testing.assert_allclose(out[0, :2], [0.0, 0.0], atol=1e-12)

    def test_ster_proj_xy_matches_ster_proj_without_intensity(self):
        alphas = np.array([0.3, 1.2, 2.5])
        betas = np.array([0.4, 0.9, 1.4])

        out_xy = ster_proj_xy(alphas, betas)
        out_full = ster_proj(alphas, betas, np.zeros_like(alphas))

        self.assertEqual(out_xy.shape, (3, 2))
        np.testing.assert_allclose(out_xy, out_full[:, :2], atol=1e-12)

    def test_azim_proj_xy_matches_azim_proj_without_intensity(self):
        alphas = np.array([0.3, 1.2, 2.5])
        betas = np.array([0.4, 0.9, 1.4])

        out_xy = azim_proj_xy(alphas, betas)
        out_full = azim_proj(alphas, betas, np.zeros_like(alphas))

        self.assertEqual(out_xy.shape, (3, 2))
        np.testing.assert_allclose(out_xy, out_full[:, :2], atol=1e-12)


class TestGetAlphaBetaFromCart(unittest.TestCase):
    def test_known_unit_vectors(self):
        # columns: +x, +y, +z
        q = np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])
        out = get_alpha_beta_from_cart(q)

        self.assertEqual(out.shape, (3, 2))
        # +x: alpha=atan2(0,1)=0,    beta=acos(0)=pi/2
        np.testing.assert_allclose(out[0], [0.0, np.pi / 2], atol=1e-12)
        # +y: alpha=atan2(0,0)=0,    beta=acos(1)=0
        np.testing.assert_allclose(out[1], [0.0, 0.0], atol=1e-12)
        # +z: alpha=atan2(1,0)=pi/2, beta=acos(0)=pi/2
        np.testing.assert_allclose(out[2], [np.pi / 2, np.pi / 2], atol=1e-12)

    def test_southern_points_are_inverted_to_northern_hemisphere(self):
        # a southern point (y < 0) is flipped through the origin, so it must give
        # the same (alpha, beta) as its northern mirror image
        north = np.array([[0.3], [0.8], [0.5]])
        north = north / np.linalg.norm(north)
        south = -north
        np.testing.assert_allclose(get_alpha_beta_from_cart(south), get_alpha_beta_from_cart(north), atol=1e-12)

    def test_values_outside_unit_range_are_clipped(self):
        # y = 1.5 would give nan from arccos without the clip to [-1, 1]
        q = np.array([[0.0], [1.5], [0.0]])
        out = get_alpha_beta_from_cart(q)
        self.assertFalse(np.any(np.isnan(out)))
        np.testing.assert_allclose(out[0, 1], 0.0, atol=1e-12)

    def test_does_not_mutate_input(self):
        q = np.array([[0.0], [1.5], [0.0]])
        q_copy = q.copy()
        get_alpha_beta_from_cart(q)
        np.testing.assert_array_equal(q, q_copy)

    def test_invalid_shape_raises_value_error(self):
        with self.assertRaises(ValueError):
            get_alpha_beta_from_cart(np.zeros((2, 5)))
        with self.assertRaises(ValueError):
            get_alpha_beta_from_cart(np.zeros(3))


class TestRing(unittest.TestCase):
    def test_ring_shape(self):
        out = ring(np.array((0.0, 0.0, 1.0)), r=1, res=50)
        self.assertEqual(out.shape, (3, 50))

    def test_ring_about_z_lies_in_xy_plane_at_radius(self):
        r = 2.0
        out = ring(np.array((0.0, 0.0, 1.0)), r=r, res=100)
        # all z components approx 0
        np.testing.assert_allclose(out[2], 0.0, atol=1e-12)
        # all points at radius r from the origin in the xy plane
        radii = np.sqrt(out[0] ** 2 + out[1] ** 2)
        np.testing.assert_allclose(radii, r, atol=1e-12)

    def test_ring_is_perpendicular_to_axis(self):
        axis = np.array((1.0, 0.0, 0.0))
        out = ring(axis, r=1.5, res=80)
        # every point should be perpendicular to the axis (dot product ~ 0)
        dots = axis @ out
        np.testing.assert_allclose(dots, 0.0, atol=1e-12)

    def test_ring_offset_translates_ring(self):
        # offsetting should rigidly translate every point by the offset (about z the
        # rotation is the identity, so the shift is applied exactly)
        axis = np.array((0.0, 0.0, 1.0))
        offset = (1.0, 2.0, 3.0)
        base = ring(axis, r=1, res=60)
        shifted = ring(axis, r=1, res=60, offset=offset)
        diff = shifted - base
        expected = np.broadcast_to(np.array(offset)[:, None], diff.shape)
        np.testing.assert_allclose(diff, expected, atol=1e-9)


class TestProjectOrientation(unittest.TestCase):
    def test_identity_rotation_matches_direct_projection(self):
        detQs_lab = np.array([[1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])
        ax_transform = np.eye(3)
        R = Rotation.identity()

        out = project_orientation(R, detQs_lab, ax_transform, "ster")

        cart = get_alpha_beta_from_cart(detQs_lab.T)
        expected = ster_proj_xy(*cart.T)
        self.assertEqual(out.shape, (2, 2))
        np.testing.assert_allclose(out, expected, atol=1e-12)

    def test_azimuthal_branch_used_when_not_ster(self):
        detQs_lab = np.array([[1.0, 0.0, 0.0], [0.0, 0.0, 1.0]])
        ax_transform = np.eye(3)
        R = Rotation.identity()

        out = project_orientation(R, detQs_lab, ax_transform, "azim")

        cart = get_alpha_beta_from_cart(detQs_lab.T)
        expected = azim_proj_xy(*cart.T)
        np.testing.assert_allclose(out, expected, atol=1e-12)


class TestVecStringToNormArray(unittest.TestCase):
    def test_normalises_simple_vectors(self):
        np.testing.assert_allclose(vec_string_to_norm_array("3,0,0"), [1.0, 0.0, 0.0])
        np.testing.assert_allclose(vec_string_to_norm_array("0,0,5"), [0.0, 0.0, 1.0])

    def test_normalises_diagonal_vector(self):
        out = vec_string_to_norm_array("1,1,1")
        np.testing.assert_allclose(out, np.full(3, 1 / np.sqrt(3)))
        np.testing.assert_allclose(np.linalg.norm(out), 1.0)

    @patch(texture_utils_path + ".logger")
    def test_wrong_length_logs_error_and_returns_x_axis(self, mock_logger):
        out = vec_string_to_norm_array("1,2")
        np.testing.assert_array_equal(out, [1, 0, 0])
        mock_logger.error.assert_called_once()

    @patch(texture_utils_path + ".logger")
    def test_unparsable_values_log_error_and_return_x_axis(self, mock_logger):
        out = vec_string_to_norm_array("a,b,c")
        np.testing.assert_array_equal(out, [1, 0, 0])
        mock_logger.error.assert_called_once()


class TestDefineGaugeVolume(unittest.TestCase):
    @patch(texture_utils_path + ".DefineGaugeVolume")
    def test_applies_gauge_volume_when_string_given(self, mock_define):
        ws = MagicMock()
        define_gauge_volume(ws, "<xml/>")
        mock_define.assert_called_once_with(ws, "<xml/>")

    @patch(texture_utils_path + ".DefineGaugeVolume")
    def test_does_nothing_when_string_is_none(self, mock_define):
        define_gauge_volume(MagicMock(), None)
        mock_define.assert_not_called()

    @patch(texture_utils_path + ".DefineGaugeVolume")
    def test_does_nothing_when_string_is_empty(self, mock_define):
        define_gauge_volume(MagicMock(), "")
        mock_define.assert_not_called()


class TestGetScatteringCentre(unittest.TestCase):
    @staticmethod
    def _make_ws(has_gauge_vol, extents=(0.01, 0.01, 0.01)):
        ws = MagicMock()
        ws.run.return_value.hasProperty.return_value = has_gauge_vol
        ws.sample.return_value.getShape.return_value.getBoundingBox.return_value.width.return_value = list(extents)
        return ws

    @patch(texture_utils_path + ".EstimateScatteringVolumeCentreOfMass", return_value=[0.1, 0.2, 0.3])
    def test_with_gauge_volume_uses_1mm_elements(self, mock_estimate):
        ws = self._make_ws(has_gauge_vol=True)
        out = get_scattering_centre(ws)

        np.testing.assert_array_equal(out, [0.1, 0.2, 0.3])
        _, kwargs = mock_estimate.call_args
        self.assertEqual(kwargs["ElementSize"], 1)
        self.assertEqual(kwargs["ElementUnits"], "mm")

    @patch(texture_utils_path + ".EstimateScatteringVolumeCentreOfMass", return_value=[0.0, 0.0, 0.0])
    def test_cubic_shape_without_gauge_volume_uses_factor_of_5(self, mock_estimate):
        ws = self._make_ws(has_gauge_vol=False, extents=(0.01, 0.01, 0.01))
        get_scattering_centre(ws)

        _, kwargs = mock_estimate.call_args
        self.assertAlmostEqual(kwargs["ElementSize"], 0.01 / 5)
        self.assertEqual(kwargs["ElementUnits"], "m")

    @patch(texture_utils_path + ".EstimateScatteringVolumeCentreOfMass", return_value=[0.0, 0.0, 0.0])
    def test_elongated_shape_without_gauge_volume_uses_factor_of_2(self, mock_estimate):
        # longest / shortest >= 5 (a wire-like shape) -> only 2 elements across the short side
        ws = self._make_ws(has_gauge_vol=False, extents=(0.001, 0.001, 0.1))
        get_scattering_centre(ws)

        _, kwargs = mock_estimate.call_args
        self.assertAlmostEqual(kwargs["ElementSize"], 0.001 / 2)

    @patch(texture_utils_path + ".logger")
    @patch(texture_utils_path + ".EstimateScatteringVolumeCentreOfMass", side_effect=RuntimeError("outside gauge volume"))
    def test_falls_back_to_origin_on_runtime_error(self, mock_estimate, mock_logger):
        ws = self._make_ws(has_gauge_vol=True)
        out = get_scattering_centre(ws)

        np.testing.assert_array_equal(out, [0.0, 0.0, 0.0])
        mock_logger.warning.assert_called_once()


if __name__ == "__main__":
    unittest.main()
