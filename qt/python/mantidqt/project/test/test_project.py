# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import os
import tempfile
import unittest

from qtpy.QtWidgets import QMessageBox

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces, RenameWorkspace, UnGroupWorkspace
from mantid.py3compat import mock
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
    def setUp(self):
        self.fgfm = FakeGlobalFigureManager()
        self.fgfm.figs = []
        self.project = Project(self.fgfm, fake_window_finding_function)

    def tearDown(self):
        ADS.clear()

    def test_save_calls_save_as_when_last_location_is_not_none(self):
        self.project.save_as = mock.MagicMock()
        self.project.save()
        self.assertEqual(self.project.save_as.call_count, 1)

    def test_save_does_not_call_save_as_when_last_location_is_not_none(self):
        self.project.save_as = mock.MagicMock()
        self.project.last_project_location = "1"
        self.assertEqual(self.project.save_as.call_count, 0)

    def test_save_saves_project_successfully(self):
        working_file = os.path.join(tempfile.mkdtemp(), "temp" + ".mtdproj")
        self.project.last_project_location = working_file
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project._offer_overwriting_gui = mock.MagicMock(return_value=QMessageBox.Yes)

        self.project.save()

        self.assertTrue(os.path.isfile(working_file))
        file_list = os.listdir(os.path.dirname(working_file))
        self.assertTrue(os.path.basename(working_file) in file_list)
        self.assertTrue("ws1.nxs" in file_list)
        self.assertEqual(self.project._offer_overwriting_gui.call_count, 1)

    def test_save_as_saves_project_successfully(self):
        working_file = os.path.join(tempfile.mkdtemp(), "temp" + ".mtdproj")
        working_directory = os.path.dirname(working_file)
        self.project._save_file_dialog = mock.MagicMock(return_value=working_file)
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save_as()

        self.assertEqual(self.project._save_file_dialog.call_count, 1)
        self.assertTrue(os.path.isfile(working_file))
        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_file) in file_list)
        self.assertTrue("ws1.nxs" in file_list)

    def test_load_calls_loads_successfully(self):
        working_directory = tempfile.mkdtemp()
        return_value_for_load = os.path.join(working_directory, os.path.basename(working_directory) + ".mtdproj")
        self.project._save_file_dialog = mock.MagicMock(return_value=return_value_for_load)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.save_as()

        self.assertEqual(self.project._save_file_dialog.call_count, 1)
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
        self.project._get_project_size = mock.MagicMock(return_value=
                                                        int(ConfigService.getString("projectSaving.warningSize")) + 1)
        self.project._offer_large_size_confirmation = mock.MagicMock()
        self.project._save()
        self.assertEqual(self.project._offer_large_size_confirmation.call_count, 1)

    def test_large_file_dialog_does_not_appear_for_small_file(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project._get_project_size = mock.MagicMock(return_value=
                                                        int(ConfigService.getString("projectSaving.warningSize")) - 1)
        self.project._offer_large_size_confirmation = mock.MagicMock()
        self.project._save()
        self.assertEqual(self.project._offer_large_size_confirmation.call_count, 0)

    def test_is_loading_is_False_after_error_thrown_during_load(self):
        with mock.patch.object(self.project, '_load_file_dialog', lambda: _raise(IOError)):
            try:
                self.project.load()
            except IOError:
                pass
        self.assertFalse(self.project.is_loading)

    def test_is_loading_is_False_after_None_returned_from_load_dialog(self):
        # None is returned from the load dialog when a user clicks Cancel
        with mock.patch.object(self.project, '_load_file_dialog', lambda: None):
            try:
                self.project.load()
            except IOError:
                pass
        self.assertFalse(self.project.is_loading)

    def test_is_saving_is_False_if_error_thrown_during_save(self):
        with mock.patch.object(self.project, '_get_project_size', lambda x: _raise(IOError)):
            try:
                self.project._save()
            except IOError:
                pass
        self.assertFalse(self.project.is_saving)

    @mock.patch('mantidqt.project.project.ProjectSaver.save_project')
    def test_workspace_groups_are_not_duplicated_when_saving(self, saver):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="newGroup")
        CreateSampleWorkspace(OutputWorkspace="ws3")
        self.project.plot_gfm.figs = "mocked_figs"
        self.project.interface_populating_function = mock.MagicMock(return_value="mocked_interfaces")

        self.project._save()
        saver.assert_called_with(file_name=self.project.last_project_location,
                                 workspace_to_save=['newGroup', 'ws3'],
                                 plots_to_save="mocked_figs",
                                 interfaces_to_save="mocked_interfaces")


if __name__ == "__main__":
    unittest.main()
