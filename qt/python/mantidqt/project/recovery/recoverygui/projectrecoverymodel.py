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
from qtpy.QtCore import Slot, QObject

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService


class ProjectRecoveryModel(QObject):
    def __init__(self, project_recovery, presenter):
        super(ProjectRecoveryModel, self).__init__()
        self.presenter = presenter
        self.project_recovery = project_recovery

        self.is_recovery_running = False
        self.has_failed_run = False

        self.rows = []
        self.fill_rows()

        self.selected_checkpoint = None

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
        if isinstance(checkpoint, str) or isinstance(checkpoint, unicode):
            # Assume if there is a T then it is a checkpoint and it needs to be replaced with a space
            checkpoint = checkpoint.replace("T", " ")
            for index in self.rows:
                if index[0] == checkpoint:
                    return index
            return ["", "", ""]
        elif isinstance(checkpoint, int):
            return self.rows[checkpoint]
        else:
            raise AttributeError("Passed checkpoint is not a valid instance for finding a row")

    def start_mantid_normally(self):
        self.presenter.close_view()

    def recover_selected_checkpoint(self, selected):
        self.is_recovery_running = True
        self.presenter.change_start_mantid_to_cancel_label()

        ADS.clear()

        # Recover given the checkpoint selected
        pid_dir = self.project_recovery.get_pid_folder_to_load_a_checkpoint_from()
        selected = selected.replace(" ", "T")
        checkpoint = os.path.join(pid_dir, selected)
        self.selected_checkpoint = selected

        try:
            self._start_recovery_of_checkpoint(checkpoint)
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail "Silently" by setting failed run to true
            self.has_failed_run = True

    def open_selected_in_editor(self, selected):
        self.is_recovery_running = True
        ADS.clear()

        # Open editor for this checkpoint
        pid_dir = self.project_recovery.get_pid_folder_to_load_a_checkpoint_from()
        selected = selected.replace(" ", "T")
        checkpoint = os.path.join(pid_dir, selected)

        try:
            self.project_recovery.open_checkpoint_in_script_editor(checkpoint)
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail "silently"
            self.has_failed_run = True

        if self.has_failed_run:
            self._update_checkpoint_tried(selected)

        self.is_recovery_running = False
        self.presenter.close_view()

    def has_recovery_started(self):
        return self.is_recovery_running

    def decide_last_checkpoint(self):
        checkpoints = self.project_recovery.listdir_fullpath(
            self.project_recovery.get_pid_folder_to_load_a_checkpoint_from())
        # Sort the checkpoints
        checkpoints.sort()
        return checkpoints[-1]

    def fill_rows(self):
        # Clear the rows
        self.rows = []

        pid_folder = self.project_recovery.get_pid_folder_to_load_a_checkpoint_from()
        paths = self.project_recovery.listdir_fullpath(os.path.join(self.project_recovery.recovery_directory_hostname,
                                                       pid_folder))

        paths.sort()
        for path in paths:
            checkpoint_name = os.path.basename(path)
            checkpoint_name = checkpoint_name.replace("T", " ")

            self._fill_row(path, checkpoint_name)

        # Fill self.rows with empty rows where required
        num_checkpoints = self.get_number_of_checkpoints()
        for ii in range(len(paths), num_checkpoints):
            self.rows.append(["", "", ""])

        # Now sort based on the first element of the lists (So it is in order of most recent checkpoint first)
        self.rows.sort(key=lambda x: x[0], reverse=True)

    @staticmethod
    def get_number_of_checkpoints():
        try:
            return int(ConfigService.getString("projectRecovery.numberOfCheckpoints"))
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail silently and return 5 (the default)
            return 5

    def _fill_row(self, path, checkpoint_name):
        num_of_ws = str(self.find_number_of_workspaces_in_directory(path))
        checked = "No"
        self.rows.append([checkpoint_name, num_of_ws, checked])

    def _update_checkpoint_tried(self, checkpoint_path):
        checkpoint_name = os.path.basename(checkpoint_path)

        # Assume if there is a T then it is a checkpoint and it needs to be replaced with a space
        checkpoint_name = checkpoint_name.replace("T", " ")

        for row in self.rows:
            if row[0] == checkpoint_name:
                row[2] = "Yes"
                return
        raise RuntimeError("Project Recovery: Passed checkpoint name for update of GUI was incorrect: "
                           + checkpoint_name)

    def _start_recovery_of_checkpoint(self, checkpoint):
        self.project_recovery.load_checkpoint(checkpoint)

        # If the run failed update the tried else it wasn't a failure and
        if self.has_failed_run:
            self._update_checkpoint_tried(self.selected_checkpoint)

        self.is_recovery_running = False
        self.presenter.close_view()

    @Slot()
    def exec_error(self):
        self.has_failed_run = True
