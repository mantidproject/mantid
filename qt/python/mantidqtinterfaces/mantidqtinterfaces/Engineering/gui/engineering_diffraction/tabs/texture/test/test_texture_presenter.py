# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.presenter import TexturePresenter
import numpy as np

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.texture.presenter"


class TestTexturePresenter(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.view = MagicMock()
        self.view.get_selected_workspaces.return_value = ([], [])
        self.presenter = TexturePresenter(self.model, self.view)

    @patch(dir_path + ".TexturePresenter.redraw_table")
    @patch(dir_path + ".logger")
    @patch(dir_path + ".ADS")
    @patch(dir_path + ".Load")
    def test_load_ws_files(self, mock_load, mock_ads, mock_logger, mock_redraw_table):
        # set up file list
        self.view.finder_texture_ws.getFilenames.return_value = ["path/to/existing_ws.nxs", "path/to/new_ws.nxs"]
        mock_ads.doesExist.side_effect = lambda ws: ws == "existing_ws"
        self.presenter.ws_assignments = {"existing_ws": "existing_param"}

        # run load files
        self.presenter.load_ws_files()
        # existing file should give notice that it will not be reloaded
        mock_logger.notice.assert_called_once_with(
            'A workspace "existing_ws" already exists, loading path/to/existing_ws.nxs has been skipped'
        )
        # new file should be loaded
        mock_load.called_once_with("path/to/new_ws.nxs", "new_ws")

        mock_redraw_table.assert_called_once()

        self.assertTrue(np.all((self.presenter.ws_names, ["existing_ws", "new_ws"])))
        self.assertIn("new_ws", self.presenter.unassigned_wss)
        self.assertNotIn("existing_ws", self.presenter.unassigned_wss)

    @patch(dir_path + ".TexturePresenter.redraw_table")
    @patch(dir_path + ".logger")
    @patch(dir_path + ".ADS")
    @patch(dir_path + ".Load")
    def test_load_param_files(self, mock_load, mock_ads, mock_logger, mock_redraw_table):
        # set up file list
        self.view.finder_texture_tables.getFilenames.return_value = ["path/to/existing_param.nxs", "path/to/new_param.nxs"]
        self.presenter.param_assignments = {"existing_param": "existing_ws"}

        # load the parameter files
        self.presenter.load_param_files()

        # no warnings
        mock_logger.warning.assert_not_called()

        # new file should be loaded
        expected_calls = [
            call(Filename="path/to/existing_param.nxs", OutputWorkspace="existing_param"),
            call(Filename="path/to/new_param.nxs", OutputWorkspace="new_param"),
        ]

        mock_load.assert_has_calls(expected_calls)

        mock_redraw_table.assert_called_once()

        self.assertIn("new_param", self.presenter.unassigned_params)
        self.assertNotIn("existing_param", self.presenter.unassigned_params)

    @patch(dir_path + ".TexturePresenter.update_readout_column_list")
    @patch(dir_path + ".TexturePresenter.update_ws_info")
    @patch(dir_path + ".logger")
    @patch(dir_path + ".ADS")
    @patch(dir_path + ".Load")
    def test_loading_new_param_adds_it_to_an_unassigned_ws_but_extras_are_removed(
        self, mock_load, mock_ads, mock_logger, mock_update_ws_info, mock_read_col
    ):
        # set up file list
        self.view.finder_texture_tables.getFilenames.return_value = ["path/to/new_param.nxs", "path/to/extra_param.nxs"]
        self.presenter.param_assignments = {"param1": "ws1"}
        self.presenter.ws_assignments = {"ws1": "param1"}
        self.presenter.unassigned_wss = ["ws2"]

        # load the parameter files
        self.presenter.load_param_files()

        # no warnings
        mock_logger.warning.assert_not_called()

        # new file should be loaded
        expected_calls = [
            call(Filename="path/to/new_param.nxs", OutputWorkspace="new_param"),
            call(Filename="path/to/extra_param.nxs", OutputWorkspace="extra_param"),
        ]

        mock_load.assert_has_calls(expected_calls)

        mock_update_ws_info.assert_called_once()
        mock_read_col.assert_called_once()

        self.assertTrue(len(self.presenter.unassigned_wss) == 0)
        self.assertTrue(len(self.presenter.unassigned_params) == 0)  # don't want extra param to be waiting
        self.assertTrue(self.presenter.ws_assignments["ws2"] == "new_param")
        self.assertTrue(self.presenter.param_assignments["new_param"] == "ws2")

    def test_assign_unpaired_wss_and_params_pairs_correctly(self):
        self.presenter.unassigned_wss = ["ws1", "ws2"]
        self.presenter.unassigned_params = ["param1", "param2", "param3"]  # extra param
        self.presenter.assign_unpaired_wss_and_params()

        self.assertEqual(self.presenter.ws_assignments, {"ws1": "param1", "ws2": "param2"})
        self.assertEqual(self.presenter.param_assignments, {"param1": "ws1", "param2": "ws2"})
        self.assertEqual(self.presenter.unassigned_params, [])  # cleared after pairing
        self.assertEqual(self.presenter.unassigned_wss, [])  # all used

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

    @patch(dir_path + ".output_settings.get_texture_axes_transform")
    @patch(dir_path + ".TexturePresenter._get_setting")
    @patch(dir_path + ".TexturePresenter.calc_pf")
    def test_on_calc_pf_clicked_starts_worker(self, mock_calc_pf, mock_get_setting, mock_get_tex_ax):
        self.view.get_selected_workspaces.return_value = (["ws1"], ["param1"])
        self.view.get_projection_method.return_value = "Stereographic"
        self.view.get_inc_scatt_power.return_value = True
        self.presenter.model.parse_hkl.return_value = [1, 1, 1]
        self.view.get_hkl.return_value = (1, 1, 1)
        self.view.get_readout_column.return_value = "I"
        self.presenter.model.get_pf_table_name.return_value = ("output_ws", "GROUP")
        mock_get_tex_ax.return_value = (np.eye(3), ("d1", "d2"))

        mock_setting_data = {"plot_exp_pf": True, "contour_kernel": "2.0", "cost_func_thresh": 0.1, "peak_pos_thresh": 0.1}
        mock_get_setting.side_effect = lambda k, *a, **kw: mock_setting_data[k]

        self.presenter.all_wss_have_params = MagicMock(return_value=True)
        self.presenter.at_least_one_param_assigned = MagicMock(return_value=True)
        self.presenter.model.check_param_ws_for_columns.return_value = (True, True)

        # Setup mocks for instance methods
        mock_worker = MagicMock()
        self.presenter.set_worker = MagicMock()
        self.presenter.get_worker = MagicMock(return_value=mock_worker)

        self.presenter.on_calc_pf_clicked()

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

    @patch(dir_path + ".output_settings.get_output_path", return_value="/tmp")
    def test_calc_pf_executes_model_and_plot(self, mock_outpath):
        self.presenter.plot_pf = MagicMock()
        self.presenter.model.make_pole_figure_tables = MagicMock()
        self.presenter.calc_pf(
            ["ws1"],
            ["param1"],
            "out_ws",
            [1, 1, 1],
            "stereographic",
            True,
            (0, 0, 0),
            0.1,
            0.1,
            "RB123",
            [[1, 0, 0], [0, 1, 0], [0, 0, 1]],
            ("A", "B"),
            "I",
            "GROUP",
            True,
            2.0,
        )
        self.presenter.model.make_pole_figure_tables.assert_called_once()
        self.presenter.plot_pf.assert_called_once()

    def test_plot_pf_calls_model_and_draws(self):
        fig = MagicMock()
        canvas = MagicMock()
        self.view.get_plot_axis.return_value = (fig, canvas)
        self.presenter.model.plot_pole_figure = MagicMock()

        self.presenter.plot_pf("ws", "proj", "", ["/tmp"], True, ("A", "B"), 2.0)

        self.presenter.model.plot_pole_figure.assert_called_once()
        canvas.draw.assert_called_once()

    def test_set_rb_num_and_calibration_update(self):
        self.presenter.set_rb_num("RB1234")
        self.assertEqual(self.presenter.rb_num, "RB1234")
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

    def test_all_wss_have_params_and_at_least_one(self):
        self.presenter.ws_names = ["ws1", "ws2"]
        self.presenter.param_assignments = {"p1": "ws1", "p2": "ws2"}
        self.view.get_selected_workspaces.return_value = (["ws1", "ws2"], ["p1", "p2"])
        self.assertTrue(self.presenter.all_wss_have_params())
        self.assertTrue(self.presenter.at_least_one_param_assigned())

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
        self.presenter.update_readout_column_list()

        self.view.populate_readout_column_list.assert_called_once_with(["I", "A", "B"], 0)
        self.view.update_col_select_visibility.assert_called_once_with(True)


class TestTextureXtalUIElementEnabling(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.view = MagicMock()
        self.view.get_selected_workspaces.return_value = ([], [])
        self.view.get_crystal_ws_prop.return_value = ""

        # inputs
        self.view.finder_cif_file = MagicMock()
        self.view.lattice_lineedit = MagicMock()
        self.view.spacegroup_lineedit = MagicMock()
        self.view.basis_lineedit = MagicMock()
        self.inputs = (self.view.finder_cif_file, self.view.lattice_lineedit, self.view.spacegroup_lineedit, self.view.basis_lineedit)

        # buttons
        self.view.btn_setCrystal = MagicMock()
        self.view.btn_setAllCrystal = MagicMock()
        self.buttons = (self.view.btn_setCrystal, self.view.btn_setAllCrystal)

        self.presenter = TexturePresenter(self.model, self.view)
        self.presenter._has_selected_wss = MagicMock()

    # ------ utility funcs ------------

    def set_getter_return_values(self, cif="", lattice="", spacegroup="", basis=""):
        self.view.get_cif.return_value = cif
        self.view.get_lattice.return_value = lattice
        self.view.get_spacegroup.return_value = spacegroup
        self.view.get_basis.return_value = basis

    def assert_expected_states(self, expected_states_inputs, expected_states_buttons):
        for i, inp in enumerate(self.inputs):
            inp.setEnabled.assert_called_with(expected_states_inputs[i])
        for i, button in enumerate(self.buttons):
            button.setEnabled.assert_called_with(expected_states_buttons[i])

    # -------- tests ------------

    def test_no_ws_no_inputs_all_inputs_enabled_all_buttons_disabled(self):
        # set view state
        self.set_getter_return_values()

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [True, True, True, True]  # cif, lattice, space group, basis
        expected_states_buttons = [False, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_cif_set_disables_lattice_no_wss_still_have_buttons_off(self):
        # set view state
        self.presenter._has_selected_wss.return_value = False
        self.set_getter_return_values(cif="test.cif")

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [True, False, False, False]  # cif, lattice, space group, basis
        expected_states_buttons = [False, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_just_lattice_set_disables_cif_no_wss_still_have_buttons_off(self):
        # set view state
        self.presenter._has_selected_wss.return_value = False
        self.set_getter_return_values(lattice="1.0    1.0    1.0")

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [False, True, True, True]  # cif, lattice, space group, basis
        expected_states_buttons = [False, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_cif_set_disables_lattice_but_ref_ws_and_no_selected_wss_only_enables_set_xtal_not_set_xtal_to_all(self):
        # set view state
        self.presenter._has_selected_wss.return_value = False
        self.set_getter_return_values(cif="test.cif")
        self.view.get_crystal_ws_prop.return_value = "ref_ws"

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [True, False, False, False]  # cif, lattice, space group, basis
        expected_states_buttons = [True, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_only_lattice_set_disables_cif_but_ref_ws_and_no_selected_wss_doesnt_enable_set_xtal_nor_set_xtal_to_all(self):
        # set view state
        self.presenter._has_selected_wss.return_value = False
        self.set_getter_return_values(lattice="1.0    1.0    1.0")
        self.view.get_crystal_ws_prop.return_value = "ref_ws"

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [False, True, True, True]  # cif, lattice, space group, basis
        expected_states_buttons = [False, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_all_lattice_inps_set_disables_cif_but_ref_ws_and_no_selected_wss_only_enables_set_xtal_not_set_xtal_to_all(self):
        # set view state
        self.presenter._has_selected_wss.return_value = False
        self.set_getter_return_values(lattice="1.0    1.0    1.0", spacegroup="P1", basis="Fe 0 0 0 1 1")
        self.view.get_crystal_ws_prop.return_value = "ref_ws"

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [False, True, True, True]  # cif, lattice, space group, basis
        expected_states_buttons = [True, False]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_all_lattice_inps_set_disables_cif_with_ref_ws_and_selected_wss_enables_set_xtal_and_set_xtal_to_all(self):
        # set view state
        self.presenter._has_selected_wss.return_value = True
        self.set_getter_return_values(lattice="1.0    1.0    1.0", spacegroup="P1", basis="Fe 0 0 0 1 1")
        self.view.get_crystal_ws_prop.return_value = "ref_ws"

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [False, True, True, True]  # cif, lattice, space group, basis
        expected_states_buttons = [True, True]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)

    def test_cif_set_disables_lattice_with_ref_ws_and_selected_wss_enables_set_xtal_and_set_xtal_to_all(self):
        # set view state
        self.presenter._has_selected_wss.return_value = True
        self.set_getter_return_values(cif="test.cif")
        self.view.get_crystal_ws_prop.return_value = "ref_ws"

        # call the method to set what is enabled
        self.presenter.set_crystal_inputs_enabled()

        # check element setter calls
        expected_states_inputs = [True, False, False, False]  # cif, lattice, space group, basis
        expected_states_buttons = [True, True]  # Set Crystal, Set Crystal to All
        self.assert_expected_states(expected_states_inputs, expected_states_buttons)


if __name__ == "__main__":
    unittest.main()
