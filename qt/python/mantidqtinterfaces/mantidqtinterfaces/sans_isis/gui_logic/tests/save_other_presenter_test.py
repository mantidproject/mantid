# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid import ConfigService
from unittest import mock
from mantidqtinterfaces.sans_isis.gui_logic.presenter.save_other_presenter import SaveOtherPresenter
from os.path import join


class SaveOtherPresenterTest(unittest.TestCase):
    def setUp(self):
        self.mock_view = mock.MagicMock()
        self.presenter = SaveOtherPresenter()
        self.presenter.set_view(view=self.mock_view)
        self.current_directory = ConfigService["defaultsave.directory"]

    def test_that_sets_initialises_and_subscribes_to_view(self):
        self.assertEqual(self.presenter._view, self.mock_view)
        self.mock_view.subscribe.assert_called_once_with(self.presenter)

    def test_on_cancel_clicked_window_closes(self):
        self.presenter.on_cancel_clicked()

        self.mock_view.done.assert_called_once_with(0)

    def test_on_browse_clicked_launches_browse_window(self):
        self.presenter.on_browse_clicked()

        self.mock_view.launch_file_browser.assert_called_once_with(self.current_directory)

    def test_on_file_name_changed_updates_filename(self):
        self.presenter.on_file_name_changed("test_file")

        self.assertEqual(self.presenter.filename, "test_file")

    def test_on_item_selection_changed_changes_label_to_suffix_on_multi_selection(self):
        self.mock_view.get_selected_workspaces.return_value = ["workspace_1", "workspace_2"]
        self.presenter.on_item_selection_changed()

        self.mock_view.rename_filebox.assert_called_once_with("Suffix")

    def test_on_item_selection_changed_enables_filename_if_single_selection(self):
        self.mock_view.get_selected_workspaces.return_value = ["workspace_1"]
        self.presenter.on_item_selection_changed()

        self.mock_view.rename_filebox.assert_called_once_with("Filename")

    def test_on_directory_changed_updates_current_directory(self):
        self.presenter.on_directory_changed("new_dir")

        self.assertEqual(self.presenter.current_directory, "new_dir")

    def test_get_filenames_returns_full_path_list_for_multi_selection(self):
        self.presenter.on_directory_changed("base_dir/")
        returned_list = self.presenter.get_filenames(["workspace_1", "workspace_2"], "filename_to_save")
        expected_list = ["base_dir/workspace_1_filename_to_save", "base_dir/workspace_2_filename_to_save"]

        self.assertEqual(expected_list, returned_list)

    def test_get_filenames_returns_specified_name_for_single_selection(self):
        self.presenter.on_directory_changed("base_dir/")
        returned_list = self.presenter.get_filenames(["workspace_1"], "filename_to_save")
        expected_list = ["base_dir/filename_to_save"]

        self.assertEqual(expected_list, returned_list)

    def test_get_filenames_returns_workspace_name_for_no_specified_name(self):
        self.presenter.on_directory_changed("base_dir/")
        returned_list = self.presenter.get_filenames(["workspace_1"], "")
        expected_list = ["base_dir/workspace_1"]

        self.assertEqual(expected_list, returned_list)

    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.save_other_presenter.AnalysisDataService")
    def test_on_save_clicked_calls_save_workspace_to_file(self, mock_ads):
        # This test is to make sure that any positional argument changes are caught.
        self.mock_view.get_selected_workspaces.return_value = ["a"]
        self.mock_view.get_filenames.return_value = ["a.nxs"]
        self.presenter.on_save_clicked()

    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.save_other_presenter.save_workspace_to_file")
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.save_other_presenter.AnalysisDataService")
    def test_on_save_clicked_calls_save_workspace_to_file_with_correct_params(self, _, mock_save):
        # This one is to ensure on_save is passing the correct params. Split, else the mocking hides bugs.
        workspace_names = ["aworkspace"]
        file_name = "afile"
        file_types = ["Nexus"]
        self.mock_view.get_selected_workspaces.return_value = workspace_names
        self.presenter.filename = file_name
        self.mock_view.get_save_options.return_value = file_types
        self.presenter.on_save_clicked()
        mock_save.assert_called_with(workspace_names[0], file_types, join(self.presenter.current_directory, file_name), {}, {})


if __name__ == "__main__":
    unittest.main()
