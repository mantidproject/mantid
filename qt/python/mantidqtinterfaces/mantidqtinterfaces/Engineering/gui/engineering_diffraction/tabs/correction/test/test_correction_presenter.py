# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
import numpy as np
from scipy.spatial.transform import Rotation
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
        # the setting is read from real QSettings otherwise, making the test depend on the machine
        self.presenter._get_setting = MagicMock(return_value=False)
        self.presenter._on_load_ref_clicked()
        self.model.load_ref.assert_called_once_with("ref_file.nxs")
        self.presenter.update_reference_info.assert_called_once()

    def test_load_ref_adopts_directions_when_the_setting_is_on(self):
        self.presenter.update_reference_info = MagicMock()
        self.presenter._adopt_reference_texture_directions = MagicMock()
        self.presenter._get_setting = MagicMock(return_value=True)

        self.presenter._on_load_ref_clicked()

        self.presenter._get_setting.assert_called_once_with("read_texture_dirs_from_ref", bool)
        self.presenter._adopt_reference_texture_directions.assert_called_once_with()

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


@patch(presenter_path + ".set_setting")
class TestAdoptReferenceTextureDirections(unittest.TestCase):
    """Loading a reference workspace re-frames the sample directions for the active experiment."""

    # RD=(0,1,0), ND=(0,0,1), TD=(1,0,0) stored as COLUMNS, so a transposed write would show up
    _AX_TRANSFORM = np.array([[0.0, 0.0, 1.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
    _DIR_NAMES = ("AD", "BD", "CD")

    def setUp(self):
        self.model = MagicMock()
        self.view = MagicMock()
        self.presenter = TextureCorrectionPresenter(self.model, self.view)
        self.presenter.set_rb_num("RB123")
        self.presenter.reference_frame_notifier = MagicMock()
        self.model.get_reference_ws.return_value = "ref_ws"
        # every direction setting currently holds the default frame
        self._stored = {"rd_dir": "1,0,0", "nd_dir": "0,1,0", "td_dir": "0,0,1", "rd_name": "RD", "nd_name": "ND", "td_name": "TD"}
        self.presenter._get_setting = MagicMock(side_effect=lambda name, *a: self._stored.get(name, ""))

    @staticmethod
    def _written(mock_set_setting):
        # {setting_name: (value, rb)} from the set_setting calls
        return {c.args[2]: (c.args[3], c.kwargs.get("rb")) for c in mock_set_setting.call_args_list}

    def test_directions_are_written_under_the_active_rb(self, mock_set_setting):
        self.model.get_reference_texture_directions.return_value = (self._AX_TRANSFORM, self._DIR_NAMES)

        self.presenter._adopt_reference_texture_directions()

        written = self._written(mock_set_setting)
        # columns of the matrix map onto RD/ND/TD in order
        self.assertEqual(written["rd_dir"], ("0.0,1.0,0.0", "RB123"))
        self.assertEqual(written["nd_dir"], ("0.0,0.0,1.0", "RB123"))
        self.assertEqual(written["td_dir"], ("1.0,0.0,0.0", "RB123"))
        self.assertEqual(written["rd_name"], ("AD", "RB123"))
        self.assertEqual(written["nd_name"], ("BD", "RB123"))
        self.assertEqual(written["td_name"], ("CD", "RB123"))

    @patch(presenter_path + ".logger")
    def test_a_reference_without_directions_leaves_the_settings_alone(self, mock_logger, mock_set_setting):
        # a reference built by Create Reference Workspace carries no direction logs; that says
        # nothing about the sample frame and must not wipe a configured one
        self.model.get_reference_texture_directions.return_value = None

        self.presenter._adopt_reference_texture_directions()

        mock_set_setting.assert_not_called()
        self.presenter.reference_frame_notifier.notify_subscribers.assert_not_called()
        mock_logger.notice.assert_called_once()

    @patch(presenter_path + ".logger")
    def test_the_change_is_logged_with_the_old_and_new_values(self, mock_logger, mock_set_setting):
        self.model.get_reference_texture_directions.return_value = (self._AX_TRANSFORM, self._DIR_NAMES)

        self.presenter._adopt_reference_texture_directions()

        message = mock_logger.notice.call_args.args[0]
        self.assertIn("RB123", message)
        self.assertIn("ref_ws", message)
        self.assertIn("rd_dir 1,0,0 -> 0.0,1.0,0.0", message)
        self.assertIn("rd_name RD -> AD", message)

    @patch(presenter_path + ".logger")
    def test_a_reference_matching_the_current_settings_is_not_reported(self, mock_logger, mock_set_setting):
        self.model.get_reference_texture_directions.return_value = (np.eye(3), ("RD", "ND", "TD"))
        self._stored.update({"rd_dir": "1.0,0.0,0.0", "nd_dir": "0.0,1.0,0.0", "td_dir": "0.0,0.0,1.0"})

        self.presenter._adopt_reference_texture_directions()

        mock_logger.notice.assert_not_called()
        self.presenter.reference_frame_notifier.notify_subscribers.assert_not_called()

    def test_the_settings_dialog_is_told_to_drop_its_cache(self, mock_set_setting):
        # otherwise the dialog's pre-load cache would be written back over these values on Apply
        self.model.get_reference_texture_directions.return_value = (self._AX_TRANSFORM, self._DIR_NAMES)

        self.presenter._adopt_reference_texture_directions()

        self.presenter.reference_frame_notifier.notify_subscribers.assert_called_once_with()

    def test_without_an_rb_the_directions_are_written_globally(self, mock_set_setting):
        self.presenter.set_rb_num(None)
        self.model.get_reference_texture_directions.return_value = (self._AX_TRANSFORM, self._DIR_NAMES)

        self.presenter._adopt_reference_texture_directions()

        self.assertEqual(self._written(mock_set_setting)["rd_dir"], ("0.0,1.0,0.0", None))

    def test_rotated_directions_are_written_at_a_usable_precision(self, mock_set_setting):
        rotated = Rotation.from_euler("z", 30, degrees=True).as_matrix()
        self.model.get_reference_texture_directions.return_value = (rotated, self._DIR_NAMES)

        self.presenter._adopt_reference_texture_directions()

        rd_value, _ = self._written(mock_set_setting)["rd_dir"]
        np.testing.assert_allclose([float(x) for x in rd_value.split(",")], rotated[:, 0], atol=1e-6)


if __name__ == "__main__":
    unittest.main()
