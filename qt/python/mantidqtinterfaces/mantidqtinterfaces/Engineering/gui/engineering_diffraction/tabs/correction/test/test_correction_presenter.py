# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import MagicMock, patch, call
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.correction.presenter import TextureCorrectionPresenter

presenter_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.correction.presenter"


class TestTextureCorrectionPresenter(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.view = MagicMock()
        self.presenter = TextureCorrectionPresenter(self.model, self.view)

    def test_select_and_deselect_all(self):
        self.presenter.select_all()
        self.view.set_all_workspaces_selected.assert_called_with(True)
        self.presenter.deselect_all()
        self.view.set_all_workspaces_selected.assert_called_with(False)

    @patch(presenter_path + ".TextureCorrectionPresenter.redraw_table")
    def test_load_ws_files(self, mock_redraw_table):
        # set up file list
        files = ["path/to/existing_ws.nxs", "path/to/new_ws.nxs"]
        loaded_wss = ["existing_ws", "new_ws"]
        self.presenter.ws_names = ["existing_ws"]
        self.view.finder_corr.getFilenames.return_value = files
        self.model.load_files.return_value = loaded_wss

        # run load files
        self.presenter.load_files_into_table()

        self.model.load_files.assert_called_once_with(files)
        mock_redraw_table.assert_called_once()

        self.assertEqual(self.presenter.ws_names, ["existing_ws", "new_ws"])

    def test_delete_selected_files(self):
        self.presenter.ws_names = ["ws1", "ws2"]
        self.view.get_selected_workspaces.return_value = ["ws1"]
        self.presenter.delete_selected_files()
        self.assertNotIn("ws1", self.presenter.ws_names)

    def test_update_ws_info_populates_dict(self):
        self.presenter.ws_names = ["ws1"]
        self.view.get_selected_workspaces.return_value = ["ws1"]
        self.model.get_ws_info.return_value = {"info": "mock"}
        self.presenter.update_ws_info()
        self.assertIn("ws1", self.presenter.ws_info)

    def test_set_rb_num_and_calibration(self):
        self.presenter.set_rb_num("RB123")
        self.assertEqual(self.presenter.rb_num, "RB123")
        cal = MagicMock()
        self.presenter.update_calibration(cal)
        self.assertEqual(self.presenter.current_calibration, cal)

    def test_on_create_ref_sample_clicked_calls_model_and_updates_view(self):
        self.presenter.rb_num = "RB123"
        self.presenter.instrument = "ENGINX"
        self.presenter.update_reference_info = MagicMock()
        self.presenter.on_create_ref_sample_clicked()
        self.model.create_reference_ws.assert_called_once_with("RB123", "ENGINX")
        self.presenter.update_reference_info.assert_called_once()

    def test_on_save_ref_clicked_calls_model_save_reference_file(self):
        self.presenter.rb_num = "RB123"
        self.presenter._on_save_ref_clicked()
        self.model.save_reference_file.assert_called_once()

    def test_load_ref(self):
        self.view.get_reference_file.return_value = "ref_file.nxs"
        self.model.load_ref = MagicMock()
        self.presenter.update_reference_info = MagicMock()
        self.presenter._on_load_ref_clicked()
        self.model.load_ref.assert_called_once_with("ref_file.nxs")
        self.presenter.update_reference_info.assert_called_once()

    def test_update_reference_info_calls_view_update(self):
        self.view.reset_mock()
        self.model.get_reference_info.return_value = ("ref_ws", True, "Fe")
        self.presenter.update_reference_info()
        self.view.update_reference_info_section.assert_has_calls([call("ref_ws", True, "Fe")])

    def test_on_copy_sample_calls_model_and_redraw(self):
        self.presenter.redraw_table = MagicMock()
        self.view.get_sample_reference_ws.return_value = "ref"
        self.view.get_selected_workspaces.return_value = ["ws1"]
        self.presenter._copy_sample_to_all_selected()
        self.model.copy_sample_info.assert_called_once()
        self.presenter.redraw_table.assert_called_once()

    def test_on_copy_ref_sample_to_all_selected(self):
        self.presenter.model.reference_ws = "ref_ws"
        self.view.get_selected_workspaces.return_value = ["ws1"]
        self.presenter.redraw_table = MagicMock()
        self.presenter._copy_ref_sample_to_all_selected()
        self.model.copy_sample_info.assert_called_once_with("ref_ws", ["ws1"], True)
        self.presenter.redraw_table.assert_called_once()

    def test_update_custom_shape_finder_vis(self):
        self.view.get_shape_method.return_value = "Custom Shape"
        self.presenter.update_custom_shape_finder_vis()
        self.view.set_finder_gauge_vol_visible.assert_called_with(True)
        self.view.get_shape_method.return_value = "4mmCube"
        self.presenter.update_custom_shape_finder_vis()
        self.view.set_finder_gauge_vol_visible.assert_called_with(False)

    def test_load_all_orientations_calls_model_and_redraws(self):
        self.view.get_selected_workspaces.return_value = ["ws1", "ws2"]
        self.view.get_orientation_file.return_value = "orient.txt"
        self.presenter._get_setting = MagicMock(side_effect=[True, "XYZ", "1,1,1"])
        self.presenter.redraw_table = MagicMock()
        self.presenter.load_all_orientations()
        self.model.load_all_orientations.assert_called_with(["ws1", "ws2"], "orient.txt", True, "XYZ", "1,1,1")
        self.presenter.redraw_table.assert_called_once()

    def test_on_apply_clicked_sets_up_and_starts_worker(self):
        self.view.get_selected_workspaces.return_value = ["ws1"]
        self.presenter._apply_all_corrections = MagicMock()
        self.presenter._on_worker_success = MagicMock()
        self.presenter._on_worker_error = MagicMock()
        self.presenter.on_apply_clicked()
        self.assertTrue(self.presenter.worker is not None)

    def test_on_worker_error_logs_error(self):
        with patch(presenter_path + ".logger.error") as mock_log:
            self.presenter._on_worker_error("Error occurred")
            mock_log.assert_called_once_with("Error occurred")

    def test_set_instrument_override_ENGINX(self):
        instrument = 0
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with("ENGINX")
        self.assertEqual(self.presenter.instrument, "ENGINX")

    def test_open_dialog_triggers_exec(self):
        mock_dialog = MagicMock()
        manager_mock = MagicMock()
        manager_mock.createDialogFromName.return_value = mock_dialog
        with patch(presenter_path + ".InterfaceManager", return_value=manager_mock):
            self.presenter._open_alg_dialog("SetGoniometer")
            mock_dialog.show.assert_called_once()

    @patch(presenter_path + ".get_setting", return_value="default")
    def test_get_setting_returns_value(self, mock_get):
        val = self.presenter._get_setting("key")
        self.assertEqual(val, "default")
        mock_get.assert_called_once()


if __name__ == "__main__":
    unittest.main()
