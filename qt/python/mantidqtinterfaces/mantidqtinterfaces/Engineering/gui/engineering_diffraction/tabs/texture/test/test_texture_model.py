# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.model import ProjectionModel
from numpy import eye, all
import tempfile


model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.model"


class TestProjectionModel_LoadFiles(unittest.TestCase):
    def setUp(self):
        self.model = ProjectionModel()

    @patch(model_path + ".logger")
    @patch(model_path + ".Load")
    @patch(model_path + ".ADS")
    def test_load_files_skips_existing_and_loads_new(self, mock_ads, mock_load, mock_logger):
        filenames = ["path/to/existing_ws.nxs", "path/to/new_ws.nxs"]

        def _does_exist(name):
            return name == "existing_ws"

        mock_ads.doesExist.side_effect = _does_exist

        ws_names = self.model.load_files(filenames)

        # returns both derived names in order
        self.assertEqual(ws_names, ["existing_ws", "new_ws"])

        # existing is skipped with a notice
        mock_logger.notice.assert_called_once_with(
            'A workspace "existing_ws" already exists, loading path/to/existing_ws.nxs has been skipped'
        )
        # new one is loaded
        mock_load.assert_called_once_with(Filename="path/to/new_ws.nxs", OutputWorkspace="new_ws")

        # ADS checked both
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
        # warning logged for the bad file
        mock_logger.warning.assert_called_once()
        self.assertIn("Failed to load /data/bad_ws.nxs: oops", mock_logger.warning.call_args[0][0])
        # still attempted both loads
        self.assertEqual(mock_load.call_count, 2)


class TestProjectionModel_ParamAndWSPairing(unittest.TestCase):
    def setUp(self):
        self.model = ProjectionModel()

    def test_has_selected_wss_and_at_least_one_param(self):
        self.assertTrue(self.model.has_selected_wss(["ws1"]))
        self.assertFalse(self.model.has_selected_wss([]))

        self.assertTrue(self.model.at_least_one_param_assigned(["p1"]))
        self.assertFalse(self.model.at_least_one_param_assigned([]))

    def test_all_wss_have_params_true(self):
        selected_wss = ["ws1", "ws2"]
        selected_params = ["p1", "p2"]
        self.assertTrue(self.model.all_wss_have_params(selected_wss, selected_params))

    def test_all_wss_have_params_false_when_not_set_present(self):
        selected_wss = ["ws1", "ws2"]
        selected_params = ["p1", "Not set"]
        self.assertFalse(self.model.all_wss_have_params(selected_wss, selected_params))

    def test_assign_unpaired_wss_and_params_pairs_correctly(self):
        unassigned_wss = ["ws1", "ws2"]
        unassigned_params = ["param1", "param2", "param3"]  # extra param should be dropped
        ws_assignments = {}
        param_assignments = {}

        u_wss, u_params, ws_map, p_map = self.model.assign_unpaired_wss_and_params(
            unassigned_wss, unassigned_params, ws_assignments, param_assignments
        )

        self.assertEqual(ws_map, {"ws1": "param1", "ws2": "param2"})
        self.assertEqual(p_map, {"param1": "ws1", "param2": "ws2"})
        self.assertEqual(u_wss, [])  # all used
        self.assertEqual(u_params, [])  # cleared per implementation

    def test_get_param_from_ws(self):
        ws_assignments = {"ws1": "p1"}
        self.assertEqual(self.model.get_param_from_ws("ws1", ws_assignments), "p1")
        self.assertEqual(self.model.get_param_from_ws("ws2", ws_assignments), "Not set")

    def test_has_at_least_one_col(self):
        self.assertTrue(self.model.has_at_least_one_col(["I"]))
        self.assertFalse(self.model.has_at_least_one_col([]))


class TestProjectionModel_XtalLogic(unittest.TestCase):
    def setUp(self):
        self.model = ProjectionModel()

    def test_lattice_checks(self):
        self.assertTrue(self.model.has_latt("a", "b", "c"))
        self.assertFalse(self.model.has_latt("", "b", "c"))
        self.assertTrue(self.model.has_any_latt("", "b", ""))
        self.assertFalse(self.model.has_any_latt("", "", ""))

    def test_cif_and_xtal_ws(self):
        self.assertTrue(self.model.has_cif("file.cif"))
        self.assertFalse(self.model.has_cif(""))

        # Either lattice triple OR cif, and ws must be non-empty
        self.assertTrue(self.model.has_xtal_and_ws("a", "b", "c", "", "ws1"))
        self.assertTrue(self.model.has_xtal_and_ws("", "", "", "file.cif", "ws1"))
        self.assertFalse(self.model.has_xtal_and_ws("", "", "", "", "ws1"))
        self.assertFalse(self.model.has_xtal_and_ws("a", "b", "c", "", ""))

    def test_can_set_all_crystal(self):
        self.assertTrue(self.model.can_set_all_crystal(True, True))
        self.assertFalse(self.model.can_set_all_crystal(False, True))
        self.assertFalse(self.model.can_set_all_crystal(True, False))


class TestProjectionModel_SettersGetters(unittest.TestCase):
    def setUp(self):
        self.model = ProjectionModel()

    def test_set_hkl_respects_inc_scatt(self):
        mock_parse = MagicMock(return_value=[1, 1, 1])
        self.model.parse_hkl = mock_parse

        # when inc_scatt is True, it parses and stores
        self.model.set_inc_scatt(True)
        self.model.set_hkl(("1", "1", "1"))
        self.assertEqual(self.model.get_hkl(), [1, 1, 1])
        mock_parse.assert_called_once_with("1", "1", "1")

        # when inc_scatt is False, hkl becomes None (no parse)
        mock_parse.reset_mock()
        self.model.set_inc_scatt(False)
        self.model.set_hkl((2, 0, 0))
        self.assertIsNone(self.model.get_hkl())
        mock_parse.assert_not_called()

    def test_set_out_ws_and_grouping_calls_pf_table_name(self):
        out_ws, combined_ws, grouping = "out_ws", "combined_ws", "group"
        mock_get_pf = MagicMock(return_value=(out_ws, combined_ws, grouping))
        self.model.get_pf_output_names = mock_get_pf

        self.model.set_readout_col("I")
        self.model.hkl = [0, 0, 1]
        wss = ["ws"]
        params = ["param"]

        self.model.set_out_ws_and_grouping(wss, params)

        mock_get_pf.assert_called_once_with(wss, params, self.model.hkl, "I")
        self.assertEqual(self.model.get_out_ws(), out_ws)
        self.assertEqual(self.model.get_combined_ws(), combined_ws)
        self.assertEqual(self.model.get_grouping(), grouping)

    def test_simple_setters_and_getters(self):
        # defaults under test
        defaults = {
            "projection_method": "stereographic",
            "inc_scatter": True,
            "readout_col": "I",
            "ax_transform": eye(3),
            "ax_labels": ["RD", "ND", "TD"],
            "plot_exp": True,
            "contour_kernel": 2.0,
            "scat_vol_pos": (0.0, 0.0, 0.0),
            "chi2_thresh": 0.0,
            "peak_thresh": 0.0,
        }

        # set
        self.model.set_projection_method(defaults["projection_method"])
        self.model.set_inc_scatt(defaults["inc_scatter"])
        self.model.set_scat_vol_pos(defaults["scat_vol_pos"])
        self.model.set_chi2_thresh(defaults["chi2_thresh"])
        self.model.set_peak_thresh(defaults["peak_thresh"])
        self.model.set_ax_trans(defaults["ax_transform"])
        self.model.set_ax_labels(defaults["ax_labels"])
        self.model.set_readout_col(defaults["readout_col"])
        self.model.set_plot_exp(defaults["plot_exp"])
        self.model.set_contour_kernel(defaults["contour_kernel"])

        # get / assert
        self.assertEqual(self.model.get_projection_method(), "stereographic")
        self.assertTrue(self.model.get_inc_scatt())
        self.assertEqual(self.model.get_scat_vol_pos(), (0.0, 0.0, 0.0))
        self.assertEqual(self.model.get_chi2_thresh(), 0.0)
        self.assertEqual(self.model.get_peak_thresh(), 0.0)
        self.assertTrue(all(self.model.get_ax_trans() == eye(3)))
        self.assertEqual(self.model.get_ax_labels(), ["RD", "ND", "TD"])
        self.assertEqual(self.model.get_readout_col(), "I")
        self.assertTrue(self.model.get_plot_exp())
        self.assertEqual(self.model.get_contour_kernel(), 2.0)


class TestProjectionModel_ExecMakePFTables(unittest.TestCase):
    def setUp(self):
        self.model = ProjectionModel()

    @patch(model_path + ".ProjectionModel.make_pole_figure_tables")
    def test_exec_make_pf_tables_passes_through_values(self, mock_make_pf):
        # preset values read by getters
        self.model.out_ws = "out_ws"
        self.model.combined_ws = "combined_ws"
        self.model.hkl = [1, 0, 0]
        self.model.set_inc_scatt(True)
        self.model.set_scat_vol_pos((0.0, 0.0, 0.0))
        self.model.set_chi2_thresh(0.5)
        self.model.set_peak_thresh(0.9)
        self.model.set_ax_trans(None)  # set as None to save asserting np.all
        self.model.set_readout_col("I")

        wss = ["ws1", "ws2"]
        params = ["p1", "p2"]
        with tempfile.TemporaryDirectory() as d:
            save_dirs = [d]
            self.model.exec_make_pf_tables(wss, params, save_dirs)

        mock_make_pf.assert_called_once_with(
            wss,
            params,
            "out_ws",
            "combined_ws",
            [1, 0, 0],
            True,
            (0.0, 0.0, 0.0),
            0.5,
            0.9,
            save_dirs,
            None,
            "I",
        )


if __name__ == "__main__":
    unittest.main()
