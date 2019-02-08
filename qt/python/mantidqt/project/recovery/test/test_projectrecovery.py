# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest
import mock
import os
import getpass
import tempfile
import time
import shutil
from pathlib2 import Path

from mantidqt.project.recovery.projectrecovery import ProjectRecovery, SAVING_TIME_KEY, NO_OF_CHECKPOINTS_KEY, \
                                                      RECOVERY_ENABLED_KEY
from mantid.kernel import ConfigService
from mantid.simpleapi import CreateSampleWorkspace


class ProjectRecoveryTest(unittest.TestCase):
    def setUp(self):
        self.globalfigmanager = mock.MagicMock()
        self.window_finder = mock.MagicMock()
        self.multifileinterpreter = mock.MagicMock()
        self.pr = ProjectRecovery(self.globalfigmanager, self.window_finder, self.multifileinterpreter)

    def tearDown(self):
        if os.path.exists(self.pr.recovery_directory_hostname):
            shutil.rmtree(self.pr.recovery_directory_hostname)

    def test_constructor_settings_are_set(self):
        # Test the paths set in the constructor that are generated.
        self.assertEqual(self.pr.recovery_directory,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery"))
        self.assertEqual(self.pr.recovery_directory_hostname,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", getpass.getuser()))
        self.assertEqual(self.pr.recovery_directory_pid,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", getpass.getuser(),
                                      str(os.getpid())))
        self.assertEqual(self.pr.recovery_order_workspace_history_file,
                         os.path.join(ConfigService.getAppDataDirectory(), "ordered_recovery.py"))

        # Test config service values
        self.assertEqual(self.pr.time_between_saves, ConfigService[SAVING_TIME_KEY])
        self.assertEqual(self.pr.maximum_num_checkpoints, ConfigService[NO_OF_CHECKPOINTS_KEY])
        self.assertEqual(self.pr.recovery_enabled, ConfigService[RECOVERY_ENABLED_KEY])

    def test_start_recovery_thread_if_thread_on_is_true(self):
        self.pr._timer_thread = mock.MagicMock()
        self.pr.thread_on = True
        self.pr.recovery_enabled = True

        self.pr.start_recovery_thread()

        self.assertEqual(self.pr._timer_thread.start.call_count, 0)

    def test_start_recovery_thread_if_thread_on_is_false(self):
        self.pr._timer_thread = mock.MagicMock()
        self.pr.thread_on = False
        self.pr.recovery_enabled = True

        self.pr.start_recovery_thread()

        self.assertEqual(self.pr._timer_thread.start.call_count, 1)

    def test_stop_recovery_thread(self):
        self.pr._timer_thread = mock.MagicMock()

        self.pr.stop_recovery_thread()

        self.assertEqual(self.pr._timer_thread.cancel.call_count, 1)

    def test_remove_empty_dir(self):
        working_directory = tempfile.mkdtemp()

        self.assertTrue(os.path.exists(working_directory))

        # Feed parent to temp directory here, on Linux = '/tmp'
        self.pr._remove_empty_folders_from_dir(str(Path(working_directory).parent))

        self.assertTrue(not os.path.exists(working_directory))

    def test_remove_all_folders_from_dir_raises_outside_of_mantid_dir(self):
        working_directory = tempfile.mkdtemp()

        self.assertRaises(RuntimeError, self.pr._remove_all_folders_from_dir, working_directory)

    def test_remove_all_folders_from_dir_doesnt_raise_inside_of_mantid_dir(self):
        temp_dir = os.path.join(self.pr.recovery_directory, "tempDir")
        os.mkdir(temp_dir)

        self.pr._remove_all_folders_from_dir(temp_dir)

        self.assertTrue(not os.path.exists(temp_dir))

    def test_sort_paths_by_last_modified(self):
        # Make sure there is actually a different modified time on the files by using sleeps
        first = tempfile.mkdtemp()
        time.sleep(0.01)
        second = tempfile.mkdtemp()
        time.sleep(0.01)
        third = tempfile.mkdtemp()
        paths = [second, third, first]
        paths = self.pr.sort_by_last_modified(paths)

        self.assertListEqual(paths, [first, second, third])

    def test_get_pid_folder_to_be_used_to_load_a_checkpoint_from(self):
        # Needs to be a high number outside of possible pids
        one = os.path.join(self.pr.recovery_directory_hostname, "10000000")
        two = os.path.join(self.pr.recovery_directory_hostname, "20000000")

        # Make the first pid folder, neither pid will be in use but it should select the oldest first
        os.makedirs(one)
        time.sleep(0.01)
        os.makedirs(two)

        result = self.pr.get_pid_folder_to_be_used_to_load_a_checkpoint_from()
        self.assertEqual(one, result)

    def test_list_dir_full_path(self):
        working_directory = tempfile.mkdtemp()
        one = os.path.join(working_directory, "10000000")
        two = os.path.join(working_directory, "20000000")
        os.makedirs(one)
        os.makedirs(two)

        # There is no concern for list order in this equality assertion
        self.assertItemsEqual([one, two], self.pr.listdir_fullpath(working_directory))

    def test_recovery_save_when_nothing_is_present(self):
        self.assertIsNone(self.pr.recovery_save())

    def test_recovery_save_with_just_workspaces(self):
        CreateSampleWorkspace(OutputWorkspace="ws")

        # Do save
        self.pr.recovery_save()

        # Check 0.py was made
        checkpoint = self.pr.listdir_fullpath(self.pr.recovery_directory_pid)[0]
        self.assertTrue(os.path.exists(os.path.join(checkpoint, "0.py")))

    # todo: continue test production
