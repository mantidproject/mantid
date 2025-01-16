# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import os
import tempfile
import unittest
import shutil
import warnings

from qtpy.QtWidgets import QMessageBox

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces, RenameWorkspace, UnGroupWorkspace
from unittest import mock
from mantid.plots.mantidaxes import MantidAxes
from mantidqt.project.project import Project
from mantidqt.utils.qt.testing import start_qapplication


class FakeGlobalFigureManager(object):
    def add_observer(self, *unused):
        pass


def fake_window_finding_function():
    return []


def _raise(exception):
    raise exception


@start_qapplication
class ProjectTest(unittest.TestCase):
    _folders_to_remove = set()

    def setUp(self):
        self.fgfm = FakeGlobalFigureManager()
        self.fgfm.figs = []
        self.project = Project(self.fgfm, fake_window_finding_function)

    def tearDown(self):
        ADS.clear()
        for folder in self._folders_to_remove:
            try:
                shutil.rmtree(folder)
            except OSError as exc:
                warnings.warn('Could not remove folder at "{}"\nError message:\n{}'.format(folder, exc))
        self._folders_to_remove.clear()

    def test_save_calls_save_as_when_last_location_is_not_none(self):
        self.project.open_project_save_dialog = mock.MagicMock()
        self.project.save()
        self.assertEqual(self.project.open_project_save_dialog.call_count, 1)

    def test_save_does_not_call_save_as_when_last_location_is_not_none(self):
        self.project.open_project_save_dialog = mock.MagicMock()
        self.project.last_project_location = "1"
        self.assertEqual(self.project.open_project_save_dialog.call_count, 0)

    def test_save_saves_project_successfully(self):
        temp_file_path = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_file_path)
        working_file = os.path.join(temp_file_path, "temp" + ".mtdproj")
        self.project.last_project_location = working_file
        self.project.remember_workspace_saving_option = True
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project._offer_overwriting_gui = mock.MagicMock(return_value=QMessageBox.Yes)

        self.project.save()

        self.assertTrue(os.path.isfile(working_file))
        file_list = os.listdir(os.path.dirname(working_file))
        self.assertTrue(os.path.basename(working_file) in file_list)
        self.assertTrue("ws1.nxs" in file_list)
        self.assertEqual(self.project._offer_overwriting_gui.call_count, 1)

    def test_save_as_saves_project_successfully(self):
        temp_file_path = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_file_path)
        working_file = os.path.join(temp_file_path, "temp" + ".mtdproj")
        working_directory = os.path.dirname(working_file)
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save_as(working_file)

        self.assertTrue(os.path.isfile(working_file))
        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_file) in file_list)
        self.assertTrue("ws1.nxs" in file_list)

    def test_load_calls_loads_successfully(self):
        working_directory = tempfile.mkdtemp()
        self._folders_to_remove.add(working_directory)
        return_value_for_load = os.path.join(working_directory, os.path.basename(working_directory) + ".mtdproj")
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.save_as(return_value_for_load)

        ADS.clear()

        self.project._load_file_dialog = mock.MagicMock(return_value=return_value_for_load)
        self.project.load()
        self.assertEqual(self.project._load_file_dialog.call_count, 1)
        self.assertEqual(["ws1"], ADS.getObjectNames())

    def test_offer_save_does_nothing_if_saved_is_true(self):
        self.assertEqual(self.project.offer_save(None), None)

    def test_offer_save_does_something_if_saved_is_false(self):
        self.project._offer_save_message_box = mock.MagicMock(return_value=QMessageBox.Yes)
        self.project.save = mock.MagicMock(return_value=None)

        # Add something to the ads so __saved is set to false
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.assertEqual(self.project.offer_save(None), False)
        self.assertEqual(self.project.save.call_count, 1)
        self.assertEqual(self.project._offer_save_message_box.call_count, 1)

    def test_offer_save_does_nothing_if_save_is_cancelled(self):
        self.project._offer_save_message_box = mock.MagicMock(return_value=QMessageBox.Yes)
        self.project.save = mock.MagicMock(return_value=True)

        # Add something to the ads so __saved is set to false
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.assertEqual(self.project.offer_save(None), True)

    def test_adding_to_ads_calls_any_change_handle(self):
        self.project.anyChangeHandle = mock.MagicMock()
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.assertEqual(1, self.project.anyChangeHandle.call_count)

    def test_removing_from_ads_calls_any_change_handle(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.anyChangeHandle = mock.MagicMock()
        ADS.remove("ws1")

        self.assertEqual(1, self.project.anyChangeHandle.call_count)

    def test_grouping_in_ads_calls_any_change_handle(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")

        self.project.anyChangeHandle = mock.MagicMock()
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        # Called twice because group is made and then added to the ADS
        self.assertEqual(2, self.project.anyChangeHandle.call_count)

    def test_renaming_in_ads_calls_any_change_handle(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.anyChangeHandle = mock.MagicMock()
        RenameWorkspace(InputWorkspace="ws1", OutputWorkspace="ws2")

        # Called twice because first workspace is removed and second is added
        self.assertEqual(2, self.project.anyChangeHandle.call_count)

    def test_ungrouping_in_ads_calls_any_change_handle(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.project.anyChangeHandle = mock.MagicMock()
        UnGroupWorkspace(InputWorkspace="NewGroup")

        # 1 for removing old group and 1 for something else but 2 seems right
        self.assertEqual(2, self.project.anyChangeHandle.call_count)

    def test_group_updated_in_ads_calls_any_change_handle(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")
        CreateSampleWorkspace(OutputWorkspace="ws3")

        self.project.anyChangeHandle = mock.MagicMock()
        ADS.addToGroup("NewGroup", "ws3")

        self.assertEqual(1, self.project.anyChangeHandle.call_count)

    def test_large_file_dialog_appears_for_large_file(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project._get_project_size = mock.MagicMock(return_value=int(ConfigService.getString("projectSaving.warningSize")) + 1)
        self.project._offer_large_size_confirmation = mock.MagicMock()
        self.project._save()
        self.assertEqual(self.project._offer_large_size_confirmation.call_count, 1)

    def test_large_file_dialog_does_not_appear_for_small_file(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project._get_project_size = mock.MagicMock(return_value=int(ConfigService.getString("projectSaving.warningSize")) - 1)
        self.project._offer_large_size_confirmation = mock.MagicMock()
        self.project._save()
        self.assertEqual(self.project._offer_large_size_confirmation.call_count, 0)

    def test_is_loading_is_False_after_error_thrown_during_load(self):
        with mock.patch.object(self.project, "_load_file_dialog", lambda: _raise(IOError)):
            try:
                self.project.load()
            except IOError:
                pass
        self.assertFalse(self.project.is_loading)

    def test_is_loading_is_False_after_None_returned_from_load_dialog(self):
        # None is returned from the load dialog when a user clicks Cancel
        with mock.patch.object(self.project, "_load_file_dialog", lambda: None):
            try:
                self.project.load()
            except IOError:
                pass
        self.assertFalse(self.project.is_loading)

    def test_is_saving_is_False_if_error_thrown_during_save(self):
        with mock.patch.object(self.project, "_get_project_size", lambda x: _raise(IOError)):
            try:
                self.project._save()
            except IOError:
                pass
        self.assertFalse(self.project.is_saving)

    @mock.patch("mantidqt.project.project.ProjectSaver.save_project")
    def test_workspace_groups_are_not_duplicated_when_saving(self, saver):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="newGroup")
        CreateSampleWorkspace(OutputWorkspace="ws3")
        self.project.plot_gfm.figs = "mocked_figs"
        self.project.interface_populating_function = mock.MagicMock(return_value="mocked_interfaces")

        self.project._save()
        saver.assert_called_with(
            file_name=self.project.last_project_location,
            workspace_to_save=["newGroup", "ws3"],
            plots_to_save="mocked_figs",
            interfaces_to_save="mocked_interfaces",
        )

    @staticmethod
    def create_altered_and_unaltered_mock_workspaces():
        # Create a mock unaltered workspace so it's history only contains Load.
        unaltered_workspace = mock.Mock()
        unaltered_workspace_history = mock.Mock()
        unaltered_workspace.getHistory.return_value = unaltered_workspace_history
        unaltered_workspace_history.size.return_value = 1
        unaltered_workspace_history.getAlgorithm(0).name.return_value = "Load"

        # Create a mock altered workspaces with history length > 1.
        altered_workspace = mock.Mock()
        altered_workspace.name.return_value = "altered_workspace"
        altered_workspace_history = mock.Mock()
        altered_workspace.getHistory.return_value = altered_workspace_history
        altered_workspace_history.size.return_value = 2

        return [altered_workspace, unaltered_workspace]

    @mock.patch("mantidqt.project.project.AnalysisDataService")
    def test_filter_unaltered_workspaces_function_removes_workspaces_that_have_only_been_loaded(self, mock_ads):
        workspaces = self.create_altered_and_unaltered_mock_workspaces()

        # When retrieveWorkspaces is called just return what is passed in.
        mock_ads.retrieveWorkspaces = lambda x: x

        altered_workspaces = self.project._filter_unaltered_workspaces(workspaces)

        self.assertEqual(len(altered_workspaces), 1)
        self.assertEqual(altered_workspaces[0], "altered_workspace")

    @mock.patch("mantidqt.project.project.AnalysisDataService")
    def test_filter_plots_removes_plots_that_use_unaltered_workspaces(self, mock_ads):
        workspaces = self.create_altered_and_unaltered_mock_workspaces()

        # When retrieveWorkspaces is called just return what is passed in.
        mock_ads.retrieveWorkspaces = lambda x: x

        # Create a plot for each workspace
        figure_managers = {}
        for i, ws in enumerate(workspaces):
            fig_manager = mock.Mock()
            mock_ax = mock.Mock(spec=MantidAxes)
            mock_ax.tracked_workspaces = [ws]
            fig_manager.canvas.figure.axes = [mock_ax]

            figure_managers[i] = fig_manager

        filtered_figure_managers = self.project._filter_plots_with_unaltered_workspaces(plots=figure_managers, workspaces=[workspaces[0]])

        self.assertEqual(len(filtered_figure_managers), 1)

    def test_saving_project_with_save_altered_workspaces_only_calls_filter_functions(self):
        self.project.save_altered_workspaces_only = True
        self.project._filter_plots_with_unaltered_workspaces = mock.Mock()
        self.project._filter_unaltered_workspaces = mock.Mock(return_value=[])

        self.project._save()

        self.project._filter_plots_with_unaltered_workspaces.assert_called_once()
        self.project._filter_unaltered_workspaces.assert_called_once()


if __name__ == "__main__":
    unittest.main()
