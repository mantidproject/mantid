# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os
import shutil
import time

from mantid.kernel import ConfigService
from mantid.api import AnalysisDataService as ADS
from mantidqt.project.recovery.recoverythread import RecoveryThread

from qtpy.QtCore import QThread
from qtpy.QtWidgets import QApplication


def replace_space_with_t(string):
    string.replace(" ", "T")
    return string


def replace_t_with_space(string):
    string.replace("T", " ")
    return string


class ProjectRecoveryModel(object):
    def __init__(self, project_recovery, presenter):
        self.presenter = presenter
        self.project_recovery = project_recovery

        self.recovery_running = False
        self.failed_run = False

        self.rows = []

        self._fill_first_row()

    def find_number_of_workspaces_in_directory(self, path):
        if not os.path.exists(path):
            raise AttributeError("Project Recovery Model: Path is not valid")
        files = self.project_recovery.listdir_fullpath(path)
        total_counted = 0
        for file in files:
            _, file_ext = os.path.splitext(file)
            if file_ext == ".py":
                total_counted += 1

        return total_counted

    def get_row(self, checkpoint):
        # if it is a string
        if isinstance(checkpoint, str):
            for index in self.rows:
                if index[0] == checkpoint:
                    return index
            return ["", "", "0"]
        # if the checkpoint is a number
        if isinstance(checkpoint, int):
            return self.rows[checkpoint]

    def start_mantid_normally(self):
        self.project_recovery.clear_all_unused_checkpoints()
        self.project_recovery.start_recovery_thread()
        self.presenter.close_view()

    def recover_selected_checkpoint(self, selected):
        self.recovery_running = True
        self.presenter.change_mantid_to_cancel_label()

        ADS.clear()

        # Recover given the checkpoint selected
        selected = replace_space_with_t(selected)
        checkpoint = os.path.join(self.project_recovery.pid_to_load_checkpoint_from(), selected)

        self.project_recovery.load_checkpoint(checkpoint)

        selected = replace_t_with_space(selected)
        if self.failed_run:
            self._update_checkpoint_tried(selected)

        self.recovery_running = False
        self.presenter.close_view()

    def open_selected_in_editor(self, selected):
        self.recovery_running = True
        ADS.clear()

        # Open editor for this checkpoint
        pid_dir = self.project_recovery.get_pid_folder_to_be_used_to_load_a_checkpoint_from()
        selected = replace_space_with_t(selected)
        checkpoint = os.path.join(pid_dir, selected)

        self.project_recovery.open_checkpoint_in_script_editor(checkpoint)

        # Restart project recovery as we stay synchronous
        self.project_recovery.clear_all_unused_checkpoints(pid_dir)
        self.project_recovery.start_recovery_thread()

        if self.failed_run:
            self._update_checkpoint_tried(selected)

        self.recovery_running = False
        self.failed_run = False
        self.presenter.close_view()

    def get_failed_run(self):
        return self.failed_run

    def has_recovery_started(self):
        return self.recovery_running

    def decide_last_checkpoint(self):
        checkpoints = self.project_recovery.listdir_fullpath(
            self.project_recovery.get_pid_folder_to_be_used_to_load_a_checkpoint_from())
        # Sort the checkpoints
        checkpoints.sort()
        return checkpoints[-1]

    def fill_rows(self):
        pid_folder = self.project_recovery.get_pid_folder_to_be_used_to_load_a_checkpoint_from()
        paths = self.project_recovery.listdir_fullpath(os.path.join(self.project_recovery.recovery_directory_hostname,
                                                                    pid_folder))
        self.project_recovery.sort_by_last_modified(paths)

        for path in paths:
            checkpoint_name = os.path.basename(path)
            checkpoint_name = replace_t_with_space(checkpoint_name)

            # If checkpoint is already present in paths then ignore and continue (Needed due to first GUI)
            if self.rows[0][0] == checkpoint_name:
                continue

            self._fill_row(path, checkpoint_name)

        # Fill self.rows with empty rows where required
        num_checkpoints = self.get_number_of_checkpoints()
        for ii in range(len(paths), num_checkpoints):
            self.rows.append(["", "", ""])

        # Now sort based on the first element of the lists (So it is in order of most recent checkpoint first)
        self.rows.sort(key=lambda x: x[0])

    @staticmethod
    def get_number_of_checkpoints():
        try:
            return int(ConfigService.getString("projectRecovery.numberOfCheckpoints"))
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail silently and return 5 (the default)
            return 5

    def _fill_first_row(self):
        pid_folder = self.project_recovery.get_pid_folder_to_be_used_to_load_a_checkpoint_from()
        paths = self.project_recovery.listdir_fullpath(os.path.join(
            self.project_recovery.recovery_directory_hostname, pid_folder))
        paths = self.project_recovery.sort_by_last_modified(paths)

        # Grab the first path as that is the one that should be loaded into the rows
        path = paths[0]
        checkpoint_name = os.path.basename(path)
        checkpoint_name = replace_t_with_space(checkpoint_name)
        self._fill_row(path, checkpoint_name)

    def _fill_row(self, path, checkpoint_name):
        num_of_ws = str(self.find_number_of_workspaces_in_directory(path))
        checked = "No"
        self.rows.append([checkpoint_name, num_of_ws, checked])

    def _update_checkpoint_tried(self, checkpoint_name):
        for row in self.rows:
            if row[0] == checkpoint_name:
                row[2] = "Yes"
                return
        raise RuntimeError("Project Recovery: Passed checkpoint name for update of GUI was incorrect: "
                           + checkpoint_name)

    def _create_thread_and_manage(self, checkpoint):
        recovery_thread = RecoveryThread()
        recovery_thread.project_recovery = self.project_recovery
        recovery_thread.checkpoint = checkpoint
        recovery_thread.start(QThread.LowPriority)

        while not recovery_thread.isFinished():
            time.sleep(0.10)
            QApplication.processEvents()

        self.failed_run = recovery_thread.failed_run_in_thread
