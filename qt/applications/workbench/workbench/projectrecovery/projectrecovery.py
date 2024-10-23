# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os
import shutil
import socket
import time
from glob import glob

import psutil

from mantid.kernel import ConfigService, logger
from workbench.projectrecovery.projectrecoveryloader import ProjectRecoveryLoader
from workbench.projectrecovery.projectrecoverysaver import ProjectRecoverySaver

if os.name == "nt":  # Windows packaged and development
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
        """
        Project Recovery class is aimed at allowing you to recovery your workbench project should you crash for whatever
         reason
        :param multifileinterpreter: MultiPythonFileInterpreter; An object that is used in workbench to represent the
        python script editor
        :param main_window: A reference to the main window object to be used as a parent to the project recovery GUIs
        :param globalfiguremanager: Based on the globalfiguremanager object expects an object with a dictionary on
        cls/self.figs for the object passed here which contains all of the plots open/needed to be saved
        """
        self._recovery_directory = os.path.join(ConfigService.getAppDataDirectory(), self.recovery_workbench_recovery_name)
        self._recovery_directory_hostname = os.path.join(self.recovery_directory, socket.gethostname())
        self._recovery_directory_pid = os.path.join(self.recovery_directory_hostname, str(os.getpid()))

        self._recovery_order_workspace_history_file = os.path.join(
            ConfigService.getAppDataDirectory(), self.recovery_ordered_recovery_file_name
        )

        self.recovery_enabled = "true" == ConfigService[RECOVERY_ENABLED_KEY].lower()
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
        """
        Starts the recovery thread if it is not already running
        """
        self.saver.start_recovery_thread()

    def stop_recovery_thread(self):
        """
        Cancels the recovery thread if it is running
        """
        self.saver.stop_recovery_thread()

    def _remove_empty_folders_from_dir(self, directory: str):
        """
        Will remove all empty directories in the given directory
        :param directory: String; A path to a directory
        """
        folders = glob(os.path.join(directory, "*", ""))
        for folder in folders:
            if len(os.listdir(folder)) == 0:
                self._remove_directory_and_directory_trees(folder, ignore_errors=True)

    def _remove_directory_and_directory_trees(self, directory: str, *, ignore_errors: bool = False):
        """
        Will remove the file/directory passed as directory if somewhere below the self.recovery_directory directory
        :param directory: String; A path to a directory/file
        :param ignore_errors: If True all OSErrors raised are swallowed, else the exceptions are propagated to the
        caller
        :return: None
        """
        if directory is None or not os.path.exists(directory):
            return

        # Only allow deleting in subdirectories of workbench-recovery
        if self.recovery_directory not in directory:
            logger.warning("Project Recovery: Only allowed to delete trees in the recovery directory")
            return

        shutil.rmtree(directory, ignore_errors)

    @staticmethod
    def sort_by_last_modified(paths):
        """
        Sort the passed paths by the last modified time and return them
        :param paths: List of Strings; The paths to be sorted
        :return: List of Strings; The sorted list of paths
        """
        paths.sort(key=lambda x: os.path.getmtime(x))
        return paths

    def get_pid_folder_to_load_a_checkpoint_from(self):
        """
        Will find the pid that isn't used but is present in the workbench-recovery directory
        :return: String; The full path to the pid directory
        """
        # Get all pids and order them
        paths = self.listdir_fullpath(self.recovery_directory_hostname)
        paths = self.sort_by_last_modified(paths)

        # Get just the pids in a list
        pids = []
        for path in paths:
            pids.append(int(os.path.basename(path)))

        for pid in pids:
            return_current_pid = True
            if psutil.pid_exists(pid):
                try:
                    return_current_pid = not self._is_mantid_workbench_process(self._make_process_from_pid(pid=pid).cmdline())
                except psutil.AccessDenied:
                    # if we can't access the process properties then we assume it is not a mantid process
                    pass

            if return_current_pid:
                return os.path.join(self.recovery_directory_hostname, str(pid))

    @staticmethod
    def _make_process_from_pid(pid):
        return psutil.Process(pid=pid)

    @staticmethod
    def listdir_fullpath(directory):
        """
        A Utility function that will list all items in a directory but with their full path.
        :param directory: String; Path to directory
        :return: List of Strings; The paths to all items in that directory
        """
        try:
            return [os.path.join(directory, item) for item in os.listdir(directory)]
        except OSError:
            return []

    def remove_current_pid_folder(self, ignore_errors=False):
        """
        Removes the current open PID folder
        :param ignore_errors: If true then all errors are swallowed, else OSErrors
        will be raised
        """
        self._remove_directory_and_directory_trees(self.recovery_directory_pid, ignore_errors=ignore_errors)

    ######################################################
    #  Saving
    ######################################################

    def recovery_save(self):
        """
        The function to save a recovery checkpoint
        """
        self.saver.recovery_save()

    ######################################################
    #  Decision
    ######################################################

    def check_for_recover_checkpoint(self):
        """
        Should a recovery attempt/offer occur?
        :return: Boolean; True if recover else False
        """
        try:
            # Clean directory first
            self._remove_empty_folders_from_dir(self.recovery_directory_hostname)

            # One pid_checkpoint equals one mantid process. If the number of checkpoints is less than the number of
            # (other) mantid processes then a recovery should occur.
            pid_checkpoints = self.listdir_fullpath(self.recovery_directory_hostname)
            num_of_other_mantid_processes = self._number_of_other_workbench_processes()
            return len(pid_checkpoints) != 0 and len(pid_checkpoints) > num_of_other_mantid_processes
        except Exception as exc:
            if isinstance(exc, KeyboardInterrupt):
                raise

            # log and return no recovery possible
            logger.warning("Error checking if project can be recovered: {}".format(str(exc)))
            return False

    def _number_of_other_workbench_processes(self):
        """
        Finds the number of other workbench processes currently present on the pc
        :return: Int; The number of other workbench processes
        """
        total_mantids = 0
        for proc in psutil.process_iter():
            try:
                # Ignore processes that have another workbench as a child (e.g. the error-reporter process)
                if self._is_mantid_workbench_process(proc.cmdline()) and not self._has_child_workbench_process(proc):
                    total_mantids += 1
            except Exception:
                # if we can't access the process properties then we assume it is not a mantid process
                pass

        # One of these will be the mantid process running the information
        return total_mantids - 1

    @staticmethod
    def _is_mantid_workbench_process(cmdline):
        for line in cmdline:
            process_name = os.path.basename(os.path.normpath(line))
            if process_name in EXECUTABLE_NAMES:
                return True
        return False

    def _has_child_workbench_process(self, proc):
        for child in proc.children(recursive=True):
            if self._is_mantid_workbench_process(child.cmdline()):
                return True
        return False

    ######################################################
    #  Loading
    ######################################################

    def attempt_recovery(self):
        """
        Attempt to recover by launching the GUI relevant to project recovery and repeating with failure if failure
        occurs.
        """
        self.loader.attempt_recovery()

    def load_checkpoint(self, directory):
        """
        Load in a checkpoint that was saved by project recovery
        :param directory: The directory from which to recover
        :return: True, when recovery fails, False when recovery succeeds
        """
        self.loader.load_checkpoint(directory)

    def open_checkpoint_in_script_editor(self, checkpoint):
        """
        Open the passed checkpoint in the script editor
        :param checkpoint: String; path to the checkpoint file
        """
        self.loader.open_checkpoint_in_script_editor(checkpoint)

    ######################################################
    #  Checkpoint Repair
    ######################################################

    def remove_oldest_checkpoints(self):
        """
        Removes the oldest checkpoint in the currently running programs' directory if it has hit the max number of
        checkpoints
        """
        paths = self.listdir_fullpath(self.recovery_directory_pid)

        if len(paths) > self.maximum_num_checkpoints:
            # Order paths in reverse and remove the last folder
            paths.sort(reverse=True)
            for ii in range(self.maximum_num_checkpoints, len(paths)):
                try:
                    self._remove_directory_and_directory_trees(paths[ii])
                except OSError as exc:
                    logger.warning(f"Unable to remove project recovery checkpoint '{paths[ii]}': {str(exc)}")

    def clear_all_unused_checkpoints(self, pid_dir=None):
        """
        Will clear up after recovery attempt succeeds and clear to an appropriate level depending on if other pids to
        recover from/being written to are present in the directory at that time.
        :param pid_dir: String; If this path is passed then it will remove this path else will find the appropriate
        level to clear to
        """
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
            self._remove_directory_and_directory_trees(path, ignore_errors=True)

    def repair_checkpoints(self):
        """
        Will remove all unfinished (have a lock file), older than a month, and empty checkpoints
        """
        pid_dirs = self.listdir_fullpath(self.recovery_directory_hostname)

        dirs_to_delete = []

        dirs_to_delete += self._find_checkpoints_older_than_a_month(pid_dirs)
        dirs_to_delete += self._find_checkpoints_which_are_locked(pid_dirs)

        for directory in dirs_to_delete:
            self._remove_directory_and_directory_trees(directory, ignore_errors=True)

        # Now the checkpoints have been deleted we may have PID directories with no checkpoints present so delete them
        self._remove_empty_folders_from_dir(self.recovery_directory_hostname)

    def _find_checkpoints_older_than_a_month(self, pid_dirs):
        """
        Finds any checkpoint that is older than a month in the passed directory
        :param pid_dirs: String; Path to the directory to be checked
        :return: List of Strings; Paths to old directories
        """
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
        """
        Will check if any checkpoints in the passed PIDs are locked and if so remove them.
        :param pid_dirs: List of Strings; The pid directories to check for locked checkpoints in
        :return: List of Strings; The paths to the locked checkpoints
        """
        locked_checkpoints = []
        for pid_dir in pid_dirs:
            checkpoints = self.listdir_fullpath(pid_dir)
            for checkpoint in checkpoints:
                if os.path.isfile(os.path.join(checkpoint, self.lock_file_name)):
                    locked_checkpoints.append(checkpoint)
        return locked_checkpoints
