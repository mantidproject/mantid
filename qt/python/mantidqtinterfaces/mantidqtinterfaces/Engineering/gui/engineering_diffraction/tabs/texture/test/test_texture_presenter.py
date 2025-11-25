# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.presenter import TexturePresenter
import numpy as np
import tempfile

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.presenter"


class TestTexturePresenter(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.model.all_wss_have_params.return_value = False
        self.model.at_least_one_param_assigned.return_value = False
        self.view = MagicMock()
        self.view.get_selected_workspaces.return_value = ([], [])
        self.presenter = TexturePresenter(self.model, self.view)

        self.pf_default_inputs = {
            "wss": ["test_ws"],
            "params": ["test_params"],
            "projection_method": "stereographic",
            "inc_scatter": True,
            "hkl": ["1", "1", "0"],
            "readout_col": "I",
            "ax_transform": np.eye(3),
            "ax_labels": ["RD", "ND", "TD"],
            "plot_exp": True,
            "contour_kernel": 2.0,
            "scat_vol_pos": (0.0, 0.0, 0.0),
            "chi2_thresh": 0.0,
            "peak_thresh": 0.0,
        }

    def _setup_default_pf_input(self):
        mock_get_setting = self.presenter._get_setting = MagicMock()
        mock_setting_data = {"plot_exp_pf": True, "contour_kernel": "2.0", "cost_func_thresh": 0.0, "peak_pos_thresh": 0.0}
        mock_get_setting.side_effect = lambda k, *a, **kw: mock_setting_data[k]

        # set all the parameters from the view onto the model
        self.view.get_selected_workspaces.return_value = (self.pf_default_inputs["wss"], self.pf_default_inputs["params"])
        self.view.get_projection_method.return_value = self.pf_default_inputs["projection_method"]
        self.view.get_inc_scatt_power.return_value = self.pf_default_inputs["inc_scatter"]
        self.view.get_hkl.return_value = self.pf_default_inputs["hkl"]
        self.view.get_readout_column.return_value = self.pf_default_inputs["readout_col"]
        self.presenter._get_ax_data = MagicMock(return_value=(self.pf_default_inputs["ax_transform"], self.pf_default_inputs["ax_labels"]))
        self.presenter.all_wss_have_params = MagicMock(return_value=True)
        self.presenter.at_least_one_param_assigned = MagicMock(return_value=True)
        self.presenter.model.check_param_ws_for_columns.return_value = (True, True)

    def _assert_pf_inputs_set_on_model(self):
        self.model.set_projection_method.assert_called_once_with(self.pf_default_inputs["projection_method"])
        self.model.set_inc_scatt.assert_called_once_with(self.pf_default_inputs["inc_scatter"])
        self.model.set_hkl.assert_called_once_with(self.pf_default_inputs["hkl"])
        self.model.set_readout_col.assert_called_once_with(self.pf_default_inputs["readout_col"])
        self.model.set_out_ws_and_grouping.assert_called_once_with(self.pf_default_inputs["wss"], self.pf_default_inputs["params"])
        self.model.set_ax_trans.assert_called_once_with(self.pf_default_inputs["ax_transform"])
        self.model.set_ax_labels.assert_called_once_with(self.pf_default_inputs["ax_labels"])
        self.model.set_plot_exp.assert_called_once_with(self.pf_default_inputs["plot_exp"])
        self.model.set_contour_kernel.assert_called_once_with(self.pf_default_inputs["contour_kernel"])
        self.model.set_scat_vol_pos.assert_called_once_with(self.pf_default_inputs["scat_vol_pos"])
        self.model.set_chi2_thresh.assert_called_once_with(self.pf_default_inputs["chi2_thresh"])
        self.model.set_peak_thresh.assert_called_once_with(self.pf_default_inputs["peak_thresh"])

    @patch(dir_path + ".TexturePresenter.redraw_table")
    def test_load_ws_files(self, mock_redraw_table):
        # set up file list
        files = ["path/to/existing_ws.nxs", "path/to/new_ws.nxs"]
        loaded_wss = ["existing_ws", "new_ws"]
        self.presenter.ws_names = ["existing_ws"]
        self.view.finder_texture_ws.getFilenames.return_value = files
        self.presenter.ws_assignments = {"existing_ws": "existing_param"}
        self.model.load_files.return_value = loaded_wss

        # run load files
        self.presenter.load_ws_files()

        self.model.load_files.assert_called_once_with(files)
        mock_redraw_table.assert_called_once()

        self.assertTrue(np.all((self.presenter.ws_names, ["existing_ws", "new_ws"])))
        self.assertIn("new_ws", self.presenter.unassigned_wss)
        self.assertNotIn("existing_ws", self.presenter.unassigned_wss)

    @patch(dir_path + ".TexturePresenter.redraw_table")
    def test_load_param_files(self, mock_redraw_table):
        # set up file list
        files = ["path/to/existing_param.nxs", "path/to/new_param.nxs"]
        loaded_wss = ["existing_param", "new_param"]
        self.view.finder_texture_tables.getFilenames.return_value = files
        self.presenter.param_assignments = {"existing_param": "existing_ws"}
        self.model.load_files.return_value = loaded_wss

        # run load files
        self.presenter.load_param_files()

        self.model.load_files.assert_called_once_with(files)
        mock_redraw_table.assert_called_once()

        self.assertIn("new_param", self.presenter.unassigned_params)
        self.assertNotIn("existing_param", self.presenter.unassigned_params)

    def setup_selection_testing(self):
        self.presenter.ws_names = ["ws1", "ws2"]
        self.presenter.ws_assignments = {"ws1": "param1", "ws2": "param2"}
        self.presenter.param_assignments = {"param1": "ws1", "param2": "ws2"}
        self.view.get_selected_workspaces.return_value = (["ws1"], ["param1"])

    @patch(dir_path + ".TexturePresenter.redraw_table")
    def test_delete_selected_files_and_param_files(self, mock_redraw_table):
        self.setup_selection_testing()
        self.presenter.delete_selected_files()
        self.assertNotIn("ws1", self.presenter.ws_names)
        self.assertNotIn("param1", self.presenter.get_assigned_params())
        self.assertTrue(np.all((self.presenter.ws_names, ["ws2"])))
        self.assertTrue(np.all((self.presenter.get_assigned_params(), ["param2"])))
        mock_redraw_table.assert_called_once()

    @patch(dir_path + ".TexturePresenter.redraw_table")
    def test_delete_selected_param_files_only(self, mock_redraw_table):
        self.setup_selection_testing()
        self.presenter.delete_selected_param_files()
        self.assertNotIn("param1", self.presenter.get_assigned_params())
        # check ws1 has been added back to the unassigned stack
        self.assertIn("ws1", self.presenter.unassigned_wss)
        mock_redraw_table.assert_called_once()

    def test_select_deselect_all(self):
        self.presenter.select_all()
        self.view.set_all_workspaces_selected.assert_called_with(True)
        self.presenter.deselect_all()
        self.view.set_all_workspaces_selected.assert_called_with(False)

    def test_set_crystal_calls_model_and_redraw(self):
        self.view.get_crystal_ws_prop.return_value = "ws1"
        self.presenter.redraw_table = MagicMock()
        self.presenter.on_set_crystal_clicked()
        self.model.set_ws_xtal.assert_called_once()
        self.presenter.redraw_table.assert_called_once()

    def test_set_all_crystal_calls_model_and_redraw(self):
        self.view.get_selected_workspaces.return_value = (["ws1", "ws2"], [])
        self.presenter.redraw_table = MagicMock()
        self.presenter.on_set_all_crystal_clicked()
        self.model.set_all_ws_xtal.assert_called_once()
        self.presenter.redraw_table.assert_called_once()

    def test_on_calc_pf_clicked_starts_worker(self):
        self.presenter.set_worker = MagicMock()
        mock_worker = MagicMock()
        self.presenter.get_worker = MagicMock(return_value=mock_worker)
        self._setup_default_pf_input()

        self.presenter.on_calc_pf_clicked()

        # assert the model values are set
        self._assert_pf_inputs_set_on_model()
        # Assert set_worker was called with an AsyncTask instance
        self.presenter.set_worker.assert_called_once()
        # Now check the worker.start was called
        mock_worker.start.assert_called_once()

    def test_on_worker_success_triggers_notifier(self):
        self.presenter.correction_notifier.notify_subscribers = MagicMock()
        self.presenter._on_worker_success()
        self.presenter.correction_notifier.notify_subscribers.assert_called_once_with("Pole Figure Created")

    @patch(dir_path + ".logger")
    def test_on_worker_error_logs(self, mock_logger):
        self.presenter._on_worker_error("Something went wrong")
        mock_logger.error.assert_called_once_with("Something went wrong")

    @patch(dir_path + ".output_settings.get_output_path")
    def test_calc_pf_executes_model_and_plot(self, mock_outpath):
        self.presenter.plot_pf = MagicMock()
        self.presenter.model.exec_make_pf_tables = MagicMock()
        wss = ["test_ws"]
        params = ["test_params"]
        save_dirs = ["dir/example"]
        root_dir, rb, group = "dir", "rb123", "GROUP"
        self.model.get_rb_num.return_value = rb
        self.model.get_grouping.return_value = group
        self.model.get_save_dirs.return_value = save_dirs
        mock_outpath.return_value = root_dir
        self.presenter.calc_pf(wss, params)

        self.model.get_save_dirs.assert_called_once_with(root_dir, "PoleFigureTables", rb, group)
        self.model.exec_make_pf_tables.assert_called_once_with(
            wss,
            params,
            save_dirs,
        )
        self.presenter.plot_pf.assert_called_once()

    def test_plot_pf_calls_model_and_draws(self):
        fig = MagicMock()
        canvas = MagicMock()
        self.view.get_plot_axis.return_value = (fig, canvas)
        self.presenter.model.exec_plot_pf = MagicMock()

        with tempfile.TemporaryDirectory() as d:
            self.presenter.plot_pf([d])

        self.presenter.model.exec_plot_pf.assert_called_once()
        canvas.draw.assert_called_once()

    def test_set_calibration_update(self):
        mock_cal = MagicMock()
        self.presenter.update_calibration(mock_cal)
        self.assertEqual(self.presenter.current_calibration, mock_cal)

    def test_set_instrument_override_ENGINX(self):
        instrument = 0
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with("ENGINX")
        self.assertEqual(self.presenter.instrument, "ENGINX")

    def test_set_instrument_override_IMAT(self):
        instrument = 1
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with("IMAT")
        self.assertEqual(self.presenter.instrument, "IMAT")

    def test_get_setting_returns_value(self):
        with patch(dir_path + ".get_setting", return_value="abc") as mock_get:
            val = self.presenter._get_setting("dummy")
            mock_get.assert_called_once()
            self.assertEqual(val, "abc")

    def test_ws_param_helpers(self):
        self.presenter.ws_assignments = {"ws1": "param1"}
        self.presenter.param_assignments = {"param1": "ws1"}
        self.assertTrue(self.presenter.ws_has_param("ws1"))
        self.assertTrue(self.presenter.param_has_ws("param1"))

    def test_update_readout_column_list_shows_column_ui_if_params_available(self):
        self.view.reset_mock()
        self.presenter.get_assigned_params = MagicMock(return_value=["param1"])
        self.presenter.all_wss_have_params = MagicMock(return_value=True)
        self.presenter.at_least_one_param_assigned = MagicMock(return_value=True)

        self.model.read_param_cols.return_value = (["I", "A", "B"], 0)
        self.model.has_at_least_one_col.return_value = True
        self.presenter.update_readout_column_list()

        self.view.populate_readout_column_list.assert_called_once_with(["I", "A", "B"], 0)
        self.view.update_col_select_visibility.assert_called_once_with(True)

    def test_set_crystal_inputs_enabled(self):
        # inputs:
        cif, lattice, spacegroup, basis, ws = "a", "b", "c", "d", "e"  # just give unique strings so we can check methods
        self.view.get_cif.return_value = cif
        self.view.get_lattice.return_value = lattice
        self.view.get_spacegroup.return_value = spacegroup
        self.view.get_basis.return_value = basis
        self.view.get_crystal_ws_prop.return_value = ws

        # model mocks
        has_cif, has_any_latt, has_xtal_and_ws, set_all_enabled = MagicMock(), MagicMock(), MagicMock(), MagicMock()
        self.model.has_cif.return_value = has_cif
        self.model.has_any_latt.return_value = has_any_latt
        self.model.has_xtal_and_ws.return_value = has_xtal_and_ws
        self.model.can_set_all_crystal.return_value = set_all_enabled

        # run method
        self.presenter.set_crystal_inputs_enabled()

        # assert func calls
        self.model.has_cif.assert_called_with(cif)
        self.model.has_any_latt.assert_called_with(lattice, spacegroup, basis)
        self.model.has_xtal_and_ws.assert_called_with(lattice, spacegroup, basis, cif, ws)

        # assert setEnabled calls
        self.view.finder_cif_file.setEnabled.assert_called_with(not has_any_latt)
        self.view.lattice_lineedit.setEnabled.assert_called_with(not has_cif)
        self.view.spacegroup_lineedit.setEnabled.assert_called_with(not has_cif)
        self.view.basis_lineedit.setEnabled.assert_called_with(not has_cif)
        self.view.btn_setCrystal.setEnabled.assert_called_with(has_xtal_and_ws)
        self.view.btn_setAllCrystal.setEnabled.assert_called_with(set_all_enabled)


if __name__ == "__main__":
    unittest.main()
