# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import unittest
import sys
import tempfile
import os

from qtpy.QtWidgets import QMessageBox

from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces, RenameWorkspace, UnGroupWorkspace
from mantid.api import AnalysisDataService as ADS
from mantidqt.project.project import Project

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class ProjectTest(unittest.TestCase):
    def setUp(self):
        self.project = Project()

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
        working_directory = tempfile.mkdtemp()
        self.project.last_project_location = working_directory
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save()

        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_directory) + ".mtdproj" in file_list)
        self.assertTrue("ws1.nxs" in file_list)

    def test_save_as_saves_project_successfully(self):
        working_directory = tempfile.mkdtemp()
        self.project._file_dialog = mock.MagicMock(return_value=working_directory)
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save_as()

        self.assertEqual(self.project._file_dialog.call_count, 1)
        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_directory) + ".mtdproj" in file_list)
        self.assertTrue("ws1.nxs" in file_list)

    def test_load_calls_loads_successfully(self):
        working_directory = tempfile.mkdtemp()
        return_value_for_load = os.path.join(working_directory, os.path.basename(working_directory) + ".mtdproj")
        self.project._file_dialog = mock.MagicMock(return_value=working_directory)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.save_as()
        
        self.assertEqual(self.project._file_dialog.call_count, 1)
        ADS.clear()

        self.project._file_dialog = mock.MagicMock(return_value=return_value_for_load)
        self.project.load()
        self.assertEqual(self.project._file_dialog.call_count, 1)
        self.assertEqual(["ws1"], ADS.getObjectNames())

    def test_offer_save_does_nothing_if_saved_is_true(self):
        self.assertEqual(self.project.offer_save(None), None)

    def test_offer_save_does_something_if_saved_is_false(self):
        self.project._offer_save_message_box = mock.MagicMock(return_value=QMessageBox.Yes)
        self.project.save = mock.MagicMock()

        # Add something to the ads so __saved is set to false
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.assertEqual(self.project.offer_save(None), False)
        self.assertEqual(self.project.save.call_count, 1)
        self.assertEqual(self.project._offer_save_message_box.call_count, 1)

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


if __name__ == "__main__":
    unittest.main()
