# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os
import shutil
import tempfile
import unittest

from mantid.api import AnalysisDataService as ADS
from unittest import mock
from mantid.simpleapi import CreateSampleWorkspace

from mantidqt.utils.testing.strict_mock import StrictContextManagerMock
from workbench.projectrecovery.projectrecovery import ProjectRecovery

unicode = str


def add_main_window_mock(loader):
    loader.main_window = mock.Mock()
    loader.main_window.algorithm_selector = mock.Mock()
    loader.main_window.algorithm_selector.block_progress_widget_updates = StrictContextManagerMock()


class ProjectRecoveryLoaderTest(unittest.TestCase):
    def setUp(self):
        self.working_directory = tempfile.mkdtemp()
        mock_mfi = mock.MagicMock()

        self.pr = ProjectRecovery(mock_mfi)
        self.pr_loader = self.pr.loader

    def tearDown(self):
        ADS.clear()
        if os.path.exists(self.pr.recovery_directory_hostname):
            shutil.rmtree(self.pr.recovery_directory_hostname)

        if os.path.exists(self.working_directory):
            shutil.rmtree(self.working_directory)

    @mock.patch("workbench.projectrecovery.projectrecoveryloader.ProjectRecoveryPresenter")
    def test_attempt_recovery_and_recovery_passes(self, presenter):
        presenter.return_value.start_recovery_view.return_value = True
        presenter.return_value.start_recovery_failure.return_value = True
        add_main_window_mock(self.pr.loader)
        self.pr.clear_all_unused_checkpoints = mock.MagicMock()
        self.pr.start_recovery_thread = mock.MagicMock()

        self.pr.attempt_recovery()

        self.assertEqual(presenter.return_value.start_recovery_view.call_count, 1)
        self.assertEqual(presenter.return_value.start_recovery_failure.call_count, 0)
        self.assertEqual(self.pr.clear_all_unused_checkpoints.call_count, 1)
        self.assertEqual(self.pr.start_recovery_thread.call_count, 1)
        self.pr.loader.main_window.algorithm_selector.block_progress_widget_updates.assert_context_triggered()

    @mock.patch("workbench.projectrecovery.projectrecoveryloader.ProjectRecoveryPresenter")
    def test_attempt_recovery_and_recovery_fails_first_time_but_is_successful_on_failure_view(self, presenter):
        presenter.return_value.start_recovery_view.return_value = False
        presenter.return_value.start_recovery_failure.return_value = True
        add_main_window_mock(self.pr.loader)
        self.pr.clear_all_unused_checkpoints = mock.MagicMock()
        self.pr.start_recovery_thread = mock.MagicMock()

        self.pr.attempt_recovery()

        self.assertEqual(presenter.return_value.start_recovery_view.call_count, 1)
        self.assertEqual(presenter.return_value.start_recovery_failure.call_count, 1)
        self.assertEqual(self.pr.clear_all_unused_checkpoints.call_count, 1)
        self.assertEqual(self.pr.start_recovery_thread.call_count, 1)
        self.pr.loader.main_window.algorithm_selector.block_progress_widget_updates.assert_context_triggered()

    @mock.patch("workbench.projectrecovery.projectrecoveryloader.ProjectLoader")
    def test_load_project_interfaces_call(self, loader):
        loader.return_value.load_project.return_value = True

        self.pr_loader._load_project_interfaces("")

        self.assertEqual(loader.return_value.load_project.call_args, mock.call(file_name=self.pr.recovery_file_ext, load_workspaces=False))

    def test_copy_in_recovery_script(self):
        # make sure to clear out the script if it exists
        if os.path.exists(self.pr.recovery_order_workspace_history_file):
            os.remove(self.pr.recovery_order_workspace_history_file)

        # Create checkpoint
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr.saver._spin_off_another_time_thread = mock.MagicMock()
        self.pr.recovery_save()

        # Find the checkpoint
        checkpoints = os.listdir(self.pr.recovery_directory_pid)
        checkpoint = os.path.join(self.pr.recovery_directory_pid, checkpoints[0])

        self.pr_loader._copy_in_recovery_script(checkpoint)

        self.assertTrue(os.path.exists(self.pr.recovery_order_workspace_history_file))

        # Confirm contents is correct
        with open(self.pr.recovery_order_workspace_history_file, "r") as f:
            actual_file_contents = f.read()

        file_contents = ""
        # Strip out the time
        for ii in actual_file_contents:
            if ii == "#":
                break
            file_contents += ii

        self.assertEqual(file_contents, "from mantid.simpleapi import *\n\nCreateSampleWorkspace(OutputWorkspace='ws') ")

    def test_load_checkpoint(self):
        # Create the checkpoint
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.pr.saver._spin_off_another_time_thread = mock.MagicMock()
        self.pr.recovery_save()

        # Mock out excess function calls
        self.pr_loader.recovery_presenter = mock.MagicMock()
        self.pr_loader._open_script_in_editor_call = mock.MagicMock()
        self.pr_loader._run_script_in_open_editor = mock.MagicMock()
        self.pr_loader._load_project_interfaces = mock.MagicMock()

        # Find the checkpoint
        checkpoints = os.listdir(self.pr.recovery_directory_pid)
        checkpoint = os.path.join(self.pr.recovery_directory_pid, checkpoints[0])

        self.pr_loader.load_checkpoint(checkpoint)

        # Test the calls are made properly
        self.assertEqual(self.pr_loader._open_script_in_editor_call.call_count, 1)
        self.assertEqual(self.pr_loader._run_script_in_open_editor.call_count, 1)
        self.assertEqual(self.pr_loader._load_project_interfaces.call_count, 1)

    def test_open_script_in_editor(self):
        self.pr_loader.recovery_presenter = mock.MagicMock()
        self.pr_loader._open_script_in_editor_call = mock.MagicMock()

        # Ensure a script file exists
        script = os.path.join(self.working_directory, "script")
        open(script, "a").close()

        self.pr_loader._open_script_in_editor(script)

        self.assertEqual(self.pr_loader._open_script_in_editor_call.call_count, 1)
        self.assertEqual(self.pr_loader.recovery_presenter.set_up_progress_bar.call_count, 1)
        self.assertEqual(self.pr_loader.recovery_presenter.set_up_progress_bar.call_args, mock.call(0))
