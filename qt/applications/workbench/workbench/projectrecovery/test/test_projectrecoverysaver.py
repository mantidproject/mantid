# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import json
import os
import shutil
import tempfile
import unittest

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces
from unittest import mock
from workbench.projectrecovery.projectrecovery import ProjectRecovery

unicode = str


class FakeEncoder(object):
    def encode(self, _, __):
        return {}

    def tags(self):
        return ["FakeEncoder"]


class ProjectRecoverySaverTest(unittest.TestCase):
    def setUp(self):
        self.working_directory = tempfile.mkdtemp()

        self.pr = ProjectRecovery(None)
        self.pr_saver = self.pr.saver

    def tearDown(self):
        ADS.clear()
        if os.path.exists(self.pr.recovery_directory_hostname):
            shutil.rmtree(self.pr.recovery_directory_hostname)

        if os.path.exists(self.working_directory):
            shutil.rmtree(self.working_directory)

    def test_add_lock_file(self):
        self.pr_saver._add_lock_file(self.working_directory)
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

    def test_remove_lock_file(self):
        self.pr_saver._add_lock_file(self.working_directory)
        self.assertTrue(os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

        self.pr_saver._remove_lock_file(self.working_directory)
        self.assertTrue(not os.path.exists(os.path.join(self.working_directory, self.pr.lock_file_name)))

    def test_recovery_save_when_nothing_is_present(self):
        self.pr.saver._spin_off_another_time_thread = mock.MagicMock()
        self.assertIsNone(self.pr.recovery_save())

    def test_recovery_save_with_just_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr_saver._spin_off_another_time_thread = mock.MagicMock()

        # Do save
        self.pr_saver.recovery_save()

        # Check 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(os.path.exists(os.path.join(checkpoint, "load_workspaces.py")))
        self.assertEqual(self.pr_saver._spin_off_another_time_thread.call_count, 1)

    @mock.patch("workbench.projectrecovery.projectrecoverysaver.find_all_windows_that_are_savable")
    def test_recovery_save_with_just_interfaces(self, windows_that_are_savable):
        CreateSampleWorkspace(OutputWorkspace="ws")
        windows_that_are_savable.return_value = [[FakeEncoder(), FakeEncoder()]]
        self.pr_saver._spin_off_another_time_thread = mock.MagicMock()
        ADS.clear()

        self.pr_saver.gfm = mock.MagicMock()
        self.pr_saver.gfm.figs = {}

        self.pr_saver.recovery_save()

        # Check no 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(not os.path.exists(os.path.join(checkpoint, "load_workspaces.py")))
        self.assertEqual(self.pr_saver._spin_off_another_time_thread.call_count, 1)

        # Read the .json file and check nothing is in workspace and something is in the interfaces dictionary
        with open(os.path.join(checkpoint, (os.path.basename(checkpoint) + self.pr.recovery_file_ext))) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 1)
            self.assertEqual(len(dictionary["workspaces"]), 0)

    def test_empty_group_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")
        CreateSampleWorkspace(OutputWorkspace="ws1")
        GroupWorkspaces(OutputWorkspace="Group", InputWorkspaces="ws,ws1")
        group_workspace = ADS.retrieve("Group")
        group_workspace.remove("ws")
        group_workspace.remove("ws1")

        self.assertTrue(self.pr_saver._empty_group_workspace(group_workspace))

    @mock.patch("workbench.projectrecovery.projectrecoverysaver.Timer")
    def test_spin_of_another_thread(self, timer):
        self.pr_saver._spin_off_another_time_thread()
        timer.assert_has_calls([mock.call().start()])

    def test_save_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws1")
        CreateSampleWorkspace(OutputWorkspace="ws2")

        self.pr_saver._save_workspaces(self.working_directory)

        load_workspaces_script = os.path.join(self.working_directory, "load_workspaces.py")
        self.assertTrue(os.path.exists(load_workspaces_script))
        with open(load_workspaces_script, "r") as reader:
            content = reader.read()
            self.assertIn("CreateSampleWorkspace(OutputWorkspace='ws1')", content)
            self.assertIn("CreateSampleWorkspace(OutputWorkspace='ws2')", content)

    def test_save_project(self):
        self.pr_saver.gfm = mock.MagicMock()
        self.pr_saver.gfm.figs = {}

        self.pr_saver._save_project(self.working_directory)

        project_file = os.path.join(self.working_directory, (os.path.basename(self.working_directory) + self.pr.recovery_file_ext))
        self.assertTrue(os.path.exists(project_file))
        with open(project_file) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 0)
            self.assertEqual(len(dictionary["workspaces"]), 0)

    @mock.patch("workbench.projectrecovery.projectrecoverysaver.find_all_windows_that_are_savable")
    def test_recovery_save_with_both_workspace_and_interfaces(self, windows_that_are_savable):
        CreateSampleWorkspace(OutputWorkspace="ws")
        windows_that_are_savable.return_value = [[FakeEncoder(), FakeEncoder()]]
        self.pr_saver._spin_off_another_time_thread = mock.MagicMock()

        self.pr_saver.gfm = mock.MagicMock()
        self.pr_saver.gfm.figs = {}

        self.pr_saver.recovery_save()

        # Check 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(os.path.exists(os.path.join(checkpoint, "load_workspaces.py")))
        self.assertEqual(self.pr_saver._spin_off_another_time_thread.call_count, 1)

        # Read the .json file and check nothing is in workspace and something is in the interfaces dictionary
        with open(os.path.join(checkpoint, (os.path.basename(checkpoint) + self.pr.recovery_file_ext))) as f:
            dictionary = json.load(f)
            self.assertEqual(len(dictionary["interfaces"]), 1)
            self.assertEqual(len(dictionary["workspaces"]), 1)

    def test_start_recovery_thread_if_thread_on_is_false(self):
        self.pr_saver._timer_thread = mock.MagicMock()
        self.pr_saver.thread_on = False
        self.pr_saver.pr.recovery_enabled = True

        self.pr_saver.start_recovery_thread()

        self.assertEqual(self.pr_saver._timer_thread.start.call_count, 1)

    def test_stop_recovery_thread(self):
        self.pr_saver._timer_thread = mock.MagicMock()

        self.pr_saver.stop_recovery_thread()

        self.assertEqual(self.pr_saver._timer_thread.cancel.call_count, 1)
