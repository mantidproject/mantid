# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import os
import shutil
import socket
import time
from glob import glob

import psutil

from mantid.kernel import ConfigService, logger
from workbench.projectrecovery.projectrecoveryloader import ProjectRecoveryLoader
from workbench.projectrecovery.projectrecoverysaver import ProjectRecoverySaver

if os.name == 'nt':  # Windows packaged and development
    EXECUTABLE_NAMES = ["launch_workbench.pyw", "workbench-script.pyw"]
else:  # Confirmed on Ubuntu 18.04 Dev and MacOS
    EXECUTABLE_NAMES = ["workbench", "workbench-script"]

THIRTY_DAYS_IN_SECONDS = 2592000

SAVING_TIME_KEY = "projectRecovery.secondsBetween"
NO_OF_CHECKPOINTS_KEY = "projectRecovery.numberOfCheckpoints"
RECOVERY_ENABLED_KEY = "projectRecovery.enabled"


class ProjectRecovery(object):
    recovery_ordered_recovery_file_name = "ordered_recovery.py"
    recovery_workbench_recovery_name = "workbench-recovery"
    recovery_file_ext = ".recfile"
    lock_file_name = "projectrecovery.lock"

    def __init__(self, multifileinterpreter, main_window=None, globalfiguremanager=None):
        self._recovery_directory = os.path.join(ConfigService.getAppDataDirectory(),
                                                self.recovery_workbench_recovery_name)
        self._recovery_directory_hostname = os.path.join(self.recovery_directory, socket.gethostname())
        self._recovery_directory_pid = os.path.join(self.recovery_directory_hostname, str(os.getpid()))

        self._recovery_order_workspace_history_file = os.path.join(ConfigService.getAppDataDirectory(),
                                                                   self.recovery_ordered_recovery_file_name)

        self.recovery_enabled = ("true" == ConfigService[RECOVERY_ENABLED_KEY].lower())
        self.maximum_num_checkpoints = int(ConfigService[NO_OF_CHECKPOINTS_KEY])
        self.time_between_saves = int(ConfigService[SAVING_TIME_KEY])  # seconds

        # The recovery GUI's presenter is set when needed
        self.recovery_presenter = None

        self.thread_on = False

        # Set to true by workbench on close to kill the thread on completion of project save
        self.closing_workbench = False

        # Recovery loader and saver
        self.loader = ProjectRecoveryLoader(self, multi_file_interpreter=multifileinterpreter, main_window=main_window)
        self.saver = ProjectRecoverySaver(self, globalfiguremanager)

    ######################################################
    #  'Read-only' properties
    ######################################################

    @property
    def recovery_directory(self):
        return self._recovery_directory

    @property
    def recovery_directory_hostname(self):
        return self._recovery_directory_hostname

    @property
    def recovery_directory_pid(self):
        return self._recovery_directory_pid

    @property
    def recovery_order_workspace_history_file(self):
        return self._recovery_order_workspace_history_file

    ######################################################
    #  Utility
    ######################################################

    def start_recovery_thread(self):
        self.saver.start_recovery_thread()

    def stop_recovery_thread(self):
        self.saver.stop_recovery_thread()

    def _remove_empty_folders_from_dir(self, directory):
        folders = glob(os.path.join(directory, "*", ""))
        for folder in folders:
            try:
                if len(os.listdir(folder)) == 0:
                    self._remove_directory_and_directory_trees(folder)
            except OSError:
                # Fail silently as expected for all folders
                pass

    def _remove_directory_and_directory_trees(self, directory):
        if directory is None or not os.path.exists(directory):
            return

        # Only allow deleting in subdirectories of workbench-recovery
        if self.recovery_directory not in directory:
            raise RuntimeError("Project Recovery: Only allowed to delete trees in the recovery directory")

        shutil.rmtree(directory)

    @staticmethod
    def sort_by_last_modified(paths):
        paths.sort(key=lambda x: os.path.getmtime(x))
        return paths

    def get_pid_folder_to_load_a_checkpoint_from(self):
        # Get all pids and order them
        paths = self.listdir_fullpath(self.recovery_directory_hostname)
        paths = self.sort_by_last_modified(paths)

        # Get just the pids in a list
        pids = []
        for path in paths:
            pids.append(int(os.path.basename(path)))

        for pid in pids:
            if not psutil.pid_exists(pid):
                return os.path.join(self.recovery_directory_hostname, str(pid))

    @staticmethod
    def listdir_fullpath(directory):
        try:
            return [os.path.join(directory, item) for item in os.listdir(directory)]
        except OSError:
            return []

    def remove_current_pid_folder(self):
        self._remove_directory_and_directory_trees(self.recovery_directory_pid)

    ######################################################
    #  Saving
    ######################################################

    def recovery_save(self):
        self.saver.recovery_save()

    ######################################################
    #  Decision
    ######################################################

    def check_for_recover_checkpoint(self):
        try:
            # Clean directory first
            self._remove_empty_folders_from_dir(self.recovery_directory_hostname)

            # One pid_checkpoint equals one mantid process. If the number of checkpoints is less than the number of
            # (other) mantid processes then a recovery should occur.
            pid_checkpoints = self.listdir_fullpath(self.recovery_directory_hostname)
            num_of_other_mantid_processes = self._number_of_other_workbench_processes()
            return len(pid_checkpoints) != 0 and len(pid_checkpoints) > num_of_other_mantid_processes
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # fail silently and return false
            return False

    @staticmethod
    def _number_of_other_workbench_processes():
        total_mantids = 0
        for proc in psutil.process_iter():
            try:
                for line in proc.cmdline():
                    process_name = os.path.basename(os.path.normpath(line))
                    if process_name in EXECUTABLE_NAMES:
                        total_mantids += 1
            except IndexError:
                # Ignore these errors as it's checking the cmdline which causes this on process with no args
                pass

        # One of these will be the mantid process running the information
        return total_mantids - 1

    ######################################################
    #  Loading
    ######################################################

    def attempt_recovery(self):
        self.loader.attempt_recovery()

    def load_checkpoint(self, directory):
        """
        Load in a checkpoint that was saved by project recovery
        :param directory: The directory from which to recover
        :return: True, when recovery fails, False when recovery succeeds
        """
        self.loader.load_checkpoint(directory)

    def open_checkpoint_in_script_editor(self, checkpoint):
        self.loader.open_checkpoint_in_script_editor(checkpoint)

    ######################################################
    #  Checkpoint Repair
    ######################################################

    def remove_oldest_checkpoints(self):
        paths = self.listdir_fullpath(self.recovery_directory_pid)

        if len(paths) > self.maximum_num_checkpoints:
            # Order paths in reverse and remove the last folder
            paths.sort(reverse=True)
            for ii in range(self.maximum_num_checkpoints, len(paths)):
                self._remove_directory_and_directory_trees(paths[ii])

    def clear_all_unused_checkpoints(self, pid_dir=None):
        if pid_dir is None:
            # If no pid directory given then remove user level folder if multiple users else remove workbench-recovery
            # folder
            if len(os.listdir(self.recovery_directory)) == 1 and len(os.listdir(self.recovery_directory_hostname)) == 0:
                path = self.recovery_directory
            else:
                path = self.recovery_directory_hostname
        else:
            path = pid_dir
        if os.path.exists(path):
            try:
                self._remove_directory_and_directory_trees(path)
            except Exception as e:
                if isinstance(e, KeyboardInterrupt):
                    raise
                else:
                    # Fail silently because it is just over diligent calls to clear checkpoints
                    pass

    def repair_checkpoints(self):
        """
        Will remove all locked, older than a month, and empty checkpoints
        """
        pid_dirs = self.listdir_fullpath(self.recovery_directory_hostname)

        dirs_to_delete = []

        dirs_to_delete += self._find_checkpoints_older_than_a_month(pid_dirs)
        dirs_to_delete += self._find_checkpoints_which_are_locked(pid_dirs)

        for directory in dirs_to_delete:
            self._remove_directory_and_directory_trees(directory)

        # Now the checkpoints have been deleted we may have PID directories with no checkpoints present so delete them
        self._remove_empty_folders_from_dir(self.recovery_directory_hostname)

    def _find_checkpoints_older_than_a_month(self, pid_dirs):
        old_pids = []
        for pid_dir in pid_dirs:
            # If pid folder hasn't been touched in a month delete it
            if self._is_file_older_than_month(pid_dir):
                old_pids.append(pid_dir)
        return old_pids

    @staticmethod
    def _is_file_older_than_month(filepath):
        last_modified = os.path.getmtime(filepath)
        return last_modified < time.time() - THIRTY_DAYS_IN_SECONDS

    def _find_checkpoints_which_are_locked(self, pid_dirs):
        locked_checkpoints = []
        for pid_dir in pid_dirs:
            checkpoints = self.listdir_fullpath(pid_dir)
            for checkpoint in checkpoints:
                if os.path.isfile(os.path.join(checkpoint, self.lock_file_name)):
                    locked_checkpoints.append(checkpoint)
        return locked_checkpoints
