# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import os
import shutil
import socket
import sys
import tempfile
import time
import unittest

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService
from mantid.py3compat import mock
from workbench.projectrecovery.projectrecovery import ProjectRecovery, SAVING_TIME_KEY, NO_OF_CHECKPOINTS_KEY, \
    RECOVERY_ENABLED_KEY

if sys.version_info.major >= 3:
    unicode = str


def is_macOS():
    return sys.platform == "darwin"


class ProjectRecoveryTest(unittest.TestCase):
    def setUp(self):
        self.multifileinterpreter = mock.MagicMock()
        self.pr = ProjectRecovery(self.multifileinterpreter)
        self.working_directory = tempfile.mkdtemp()

    def tearDown(self):
        ADS.clear()
        if os.path.exists(self.pr.recovery_directory_hostname):
            shutil.rmtree(self.pr.recovery_directory_hostname)

        if os.path.exists(self.working_directory):
            shutil.rmtree(self.working_directory)

    def test_constructor_settings_are_set(self):
        # Test the paths set in the constructor that are generated.
        self.assertEqual(self.pr.recovery_directory,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery"))
        self.assertEqual(self.pr.recovery_directory_hostname,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", socket.gethostname()))
        self.assertEqual(self.pr.recovery_directory_pid,
                         os.path.join(ConfigService.getAppDataDirectory(), "workbench-recovery", socket.gethostname(),
                                      str(os.getpid())))
        self.assertEqual(self.pr.recovery_order_workspace_history_file,
                         os.path.join(ConfigService.getAppDataDirectory(), "ordered_recovery.py"))

        # Test config service values
        self.assertEqual(self.pr.time_between_saves, int(ConfigService[SAVING_TIME_KEY]))
        self.assertEqual(self.pr.maximum_num_checkpoints, int(ConfigService[NO_OF_CHECKPOINTS_KEY]))
        self.assertEqual(self.pr.recovery_enabled, ("true" == ConfigService[RECOVERY_ENABLED_KEY].lower()))

    def test_start_recovery_thread_if_thread_on_is_true(self):
        self.pr._timer_thread = mock.MagicMock()
        self.pr.thread_on = True
        self.pr.recovery_enabled = True

        self.pr.start_recovery_thread()

        self.assertEqual(self.pr._timer_thread.start.call_count, 0)

    def test_remove_empty_dir(self):
        if not os.path.exists(self.pr.recovery_directory):
            os.makedirs(self.pr.recovery_directory)

        empty = os.path.join(self.pr.recovery_directory, "empty")
        os.mkdir(os.path.join(self.pr.recovery_directory, "empty"))

        # Feed parent to temp directory here, on Linux = '/tmp'
        parent_path, _ = os.path.split(empty)
        self.pr._remove_empty_folders_from_dir(parent_path)

        self.assertTrue(not os.path.exists(empty))

    def test_remove_empty_dir_throws_outside_of_workbench_directory(self):
        os.mkdir(os.path.join(self.working_directory, "temp"))
        self.assertRaises(RuntimeError, self.pr._remove_empty_folders_from_dir, self.working_directory)

    def test_remove_all_folders_from_dir_raises_outside_of_mantid_dir(self):
        self.assertRaises(RuntimeError, self.pr._remove_directory_and_directory_trees, self.working_directory)

    def test_remove_all_folders_from_dir_doesnt_raise_inside_of_mantid_dir(self):
        temp_dir = os.path.join(self.pr.recovery_directory, "tempDir")
        os.mkdir(temp_dir)

        self.pr._remove_directory_and_directory_trees(temp_dir)

        self.assertTrue(not os.path.exists(temp_dir))

    @unittest.skipIf(is_macOS(), "Can be unreliable on macOS and is a test of logic not OS capability")
    def test_sort_paths_by_last_modified(self):
        # Make sure there is actually a different modified time on the files by using sleeps
        first = tempfile.mkdtemp()
        time.sleep(0.5)
        second = tempfile.mkdtemp()
        time.sleep(0.5)
        third = tempfile.mkdtemp()
        paths = [second, third, first]
        paths = self.pr.sort_by_last_modified(paths)

        self.assertListEqual(paths, [first, second, third])

    def test_get_pid_folder_to_be_used_to_load_a_checkpoint_from(self):
        self.pr._make_process_from_pid = mock.MagicMock()
        self.pr._is_mantid_workbench_process = mock.MagicMock(return_value=True)
        # Needs to be a high number outside of possible pids
        one = os.path.join(self.pr.recovery_directory_hostname, "10000000")
        two = os.path.join(self.pr.recovery_directory_hostname, "20000000")

        # Make the first pid folder, neither pid will be in use but it should select the oldest first
        os.makedirs(one)
        time.sleep(0.01)
        os.makedirs(two)

        result = self.pr.get_pid_folder_to_load_a_checkpoint_from()
        self.assertEqual(one, result)

    def test_list_dir_full_path(self):
        one = os.path.join(self.working_directory, "10000000")
        two = os.path.join(self.working_directory, "20000000")
        os.makedirs(one)
        os.makedirs(two)

        # There is no concern for list order in this equality assertion
        if sys.version_info.major < 3:
            # Python 2.7 way of doing it
            self.assertItemsEqual([one, two], self.pr.listdir_fullpath(self.working_directory))
        else:
            # Python 3.2+ way of doing it
            self.assertCountEqual([one, two], self.pr.listdir_fullpath(self.working_directory))

    def test_recovery_save_when_nothing_is_present(self):
        self.pr.saver._spin_off_another_time_thread = mock.MagicMock()
        self.assertIsNone(self.pr.recovery_save())

    def test_check_for_recovery_where_no_checkpoints_exist(self):
        self.assertTrue(not self.pr.check_for_recover_checkpoint())

    @staticmethod
    def create_checkpoints_at_directory(directory, number):
        for ii in range(0, number):
            os.makedirs(os.path.join(directory, (str(ii) + "00000000"), "spare_dir"))

    def test_check_for_recovery_when_2_instances_exist_with_2_checkpoints(self):
        # Doesn't do recovery so returns false

        # Create the 2 checkpoints
        self.create_checkpoints_at_directory(self.pr.recovery_directory_hostname, 2)
        self.pr._number_of_other_workbench_processes = mock.MagicMock(return_value=2)

        self.assertTrue(not self.pr.check_for_recover_checkpoint())

    def test_check_for_recovery_when_2_instances_exist_with_3_checkpoints(self):
        # Does recovery so returns true
        # Create the 3 checkpoints
        self.create_checkpoints_at_directory(self.pr.recovery_directory_hostname, 3)
        self.pr._number_of_other_workbench_processes = mock.MagicMock(return_value=2)

        self.assertTrue(self.pr.check_for_recover_checkpoint())

    def test_remove_oldest_checkpoints(self):
        self.pr._recovery_directory_pid = self.working_directory
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()

        for ii in range(0, self.pr.maximum_num_checkpoints+1):
            os.mkdir(os.path.join(self.working_directory, "dir"+str(ii)))
            time.sleep(0.01)

        self.pr.remove_oldest_checkpoints()

        # Now should have had a call made to delete working_directory + dir0
        self.pr._remove_directory_and_directory_trees.assert_called_with(os.path.join(self.working_directory, "dir0"))

    def test_clear_all_unused_checkpoints_called_with_none_and_only_one_user(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)

        self.pr.clear_all_unused_checkpoints()

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def test_clear_all_unused_checkpoints_called_with_none_and_multiple_users(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)
        user = os.path.join(self.pr.recovery_directory, "dimitar")
        if os.path.exists(user):
            shutil.rmtree(user)
        os.makedirs(user)

        self.pr.clear_all_unused_checkpoints()

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def test_clear_all_unused_checkpoints_called_with_not_none(self):
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        os.makedirs(self.pr.recovery_directory_hostname)

        self.pr.clear_all_unused_checkpoints(pid_dir=self.pr.recovery_directory_hostname)

        self.pr._remove_directory_and_directory_trees.assert_called_with(self.pr.recovery_directory_hostname)

    def _repair_checkpoint_checkpoints_setup(self, checkpoint1, checkpoint2, pid, pid2):
        if os.path.exists(pid):
            shutil.rmtree(pid)
        os.makedirs(pid)
        os.makedirs(checkpoint1)
        if os.path.exists(pid2):
            shutil.rmtree(pid2)
        os.makedirs(pid2)
        os.makedirs(checkpoint2)

        # Add a lock file to checkpoint 1
        open(os.path.join(checkpoint1, self.pr.lock_file_name), 'a').close()

        # Add one workspace to the checkpoint and change modified dates to older than a month
        os.utime(pid2, (1, 1))

    def _repair_checkpoints_assertions(self, checkpoint1, checkpoint2, pid, pid2):
        # None of the checkpoints should exist after the call. Thus the PID folder should be deleted and thus ignored.
        directory_removal_calls = [mock.call(os.path.join(self.pr.recovery_directory_hostname, '200000')),
                                   mock.call(os.path.join(self.pr.recovery_directory_hostname, "1000000", "check1"))]

        self.pr._remove_directory_and_directory_trees.assert_has_calls(directory_removal_calls)

        empty_file_calls = [mock.call(self.pr.recovery_directory_hostname)]
        self.pr._remove_empty_folders_from_dir.assert_has_calls(empty_file_calls)

        self.assertTrue(os.path.exists(checkpoint1))
        self.assertTrue(os.path.exists(pid2))
        self.assertTrue(os.path.exists(checkpoint2))
        self.assertTrue(os.path.exists(pid))

    def test_repair_checkpoints(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        checkpoint1 = os.path.join(pid, "check1")
        pid2 = os.path.join(self.pr.recovery_directory_hostname, "200000")
        checkpoint2 = os.path.join(pid, "check3")
        self._repair_checkpoint_checkpoints_setup(checkpoint1, checkpoint2, pid, pid2)
        self.pr._remove_directory_and_directory_trees = mock.MagicMock()
        self.pr._remove_empty_folders_from_dir = mock.MagicMock()

        self.pr.repair_checkpoints()

        self._repair_checkpoints_assertions(checkpoint1, checkpoint2, pid, pid2)

        self.pr._remove_empty_folders_from_dir(self.pr.recovery_directory_hostname)

    def test_find_checkpoints_older_than_a_month(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        if os.path.exists(pid):
            shutil.rmtree(pid)

        os.makedirs(pid)
        os.utime(pid, (1, 1))

        self.assertEqual([pid], self.pr._find_checkpoints_older_than_a_month([pid]))

    def test_find_checkpoints_which_are_locked(self):
        pid = os.path.join(self.pr.recovery_directory_hostname, "1000000")
        if os.path.exists(pid):
            shutil.rmtree(pid)
        checkpoint1 = os.path.join(pid, "check1")

        os.makedirs(pid)
        os.makedirs(checkpoint1)
        # Add a lock file to checkpoint 1
        open(os.path.join(checkpoint1, self.pr.lock_file_name), 'a').close()

        self.assertEqual([checkpoint1], self.pr._find_checkpoints_which_are_locked([pid]))
