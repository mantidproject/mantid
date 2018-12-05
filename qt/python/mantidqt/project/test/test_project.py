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
from time import sleep

from qtpy.QtWidgets import QMessageBox

from mantidqt.project.project import Project
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces, RenameWorkspace, UnGroupWorkspace, DeleteWorkspace
from mantid.api import AnalysisDataService as ADS

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
        self.project._get_directory_finder = mock.MagicMock(return_value=working_directory)
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save_as()

        self.assertEqual(self.project._get_directory_finder.call_count, 1)
        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_directory) + ".mtdproj" in file_list)
        self.assertTrue("ws1.nxs" in file_list)

    def test_load_calls_loads_successfully(self):
        working_directory = tempfile.mkdtemp()
        self.project._get_directory_finder = mock.MagicMock(return_value=working_directory)
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.save_as()

        ADS.clear()

        self.project.load()
        self.assertEqual(self.project._get_directory_finder.call_count, 2)
        self.assertEqual(["ws1"], ADS.getObjectNames())

    def test_offer_save_does_nothing_if_saved_is_true(self):
        self.project.saved = True

        self.assertEqual(self.project.offer_save(None), None)

    def test_offer_save_does_something_if_saved_is_false(self):
        self.project._offer_save_message_box = mock.MagicMock(return_value=QMessageBox.Yes)
        self.project.save = mock.MagicMock()
        self.project.saved = False

        self.assertEqual(self.project.offer_save(None), False)
        self.assertEqual(self.project.save.call_count, 1)
        self.assertEqual(self.project._offer_save_message_box.call_count, 1)

    def test_adding_to_ads_sets_saved_to_false(self):
        self.project.saved = True
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.assertTrue(not self.project.saved)

    def test_removing_from_ads_sets_saved_to_false(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.saved = True
        DeleteWorkspace("ws1")

        # DeleteWorkspace tries to complete on a separate thread and it is required to wait for it
        sleep(0.05)

        self.assertTrue(not self.project.saved)

    def test_grouping_in_ads_sets_saved_to_false(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")

        self.project.saved = True
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.assertTrue(not self.project.saved)

    def test_renaming_in_ads_sets_saved_to_false(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        self.project.saved = True
        RenameWorkspace(InputWorkspace="ws1", OutputWorkspace="ws2")

        self.assertTrue(not self.project.saved)

    def test_ungrouping_in_ads_sets_saved_to_false(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")

        self.project.saved = True
        UnGroupWorkspace(InputWorkspace="NewGroup")

        self.assertTrue(not self.project.saved)

    def test_group_updated_in_ads_sets_saved_to_false(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")
        GroupWorkspaces(InputWorkspaces="ws1,ws2", OutputWorkspace="NewGroup")
        CreateSampleWorkspace(OutputWorkspace="ws3")

        self.project.saved = True
        ADS.addToGroup("NewGroup", "ws3")

        # Adding a ws to a group takes time so wait for it
        sleep(0.05)

        self.assertTrue(not self.project.saved)

    def test_removing_unused_workspaces_operates_as_expected_from_save(self):
        working_directory = tempfile.mkdtemp()
        self.project.last_project_location = working_directory
        CreateSampleWorkspace(OutputWorkspace="ws1")

        self.project.save()
        ADS.clear()
        CreateSampleWorkspace(OutputWorkspace="ws2")
        self.project.save()

        self.assertTrue(os.path.isdir(working_directory))
        file_list = os.listdir(working_directory)
        self.assertTrue(os.path.basename(working_directory) + ".mtdproj" in file_list)
        self.assertTrue("ws2.nxs" in file_list)
        self.assertTrue("ws1.nxs" not in file_list)
