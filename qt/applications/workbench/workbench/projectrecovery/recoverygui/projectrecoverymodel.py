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

from qtpy.QtCore import Slot, QObject

from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService, logger


DEFAULT_NUM_CHECKPOINTS = 5


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
        """
        Returns the number of workspaces in the given directory
        :param path: String; A directory to the checkpoint
        :return: Int; Number of workspaces in the path
        """
        if not os.path.exists(path):
            raise AttributeError("Project Recovery Model: Path is not valid")

        rec_file = os.path.join(path, os.path.basename(path) + self.project_recovery.recovery_file_ext)
        if os.path.exists(rec_file):
            rec_file_dict = {}
            with open(rec_file) as reader:
                try:
                    rec_file_dict = json.load(reader)
                except json.decoder.JSONDecodeError:
                    logger.warning(
                        f"Project Recovery Model: Recovery file {rec_file} is corrupted, open plots and interfaces will "
                        "fail to recover, but workspaces may reload."
                    )
                    return 0

            return len(rec_file_dict.get("workspaces", 0))
        else:
            return 0

    def get_row(self, checkpoint):
        """
        Return the row from self.rows given the input checkpoint request
        :param checkpoint: String, unicode or Int; The identifier for which to find the row information from
        :return: List of 3 Strings; [0] Checkpoint name, [1] Number of workspaces, [2] Whether checkpoint has been tried
         or not
        """
        if isinstance(checkpoint, str):
            # Assume if there is a T then it is a checkpoint and it needs to be replaced with a space
            checkpoint = checkpoint.replace("T", " ")
            for index in self.rows:
                if index[0] == checkpoint:
                    return index
            return ["", "", ""]
        elif isinstance(checkpoint, int):
            return self.rows[checkpoint]
        else:
            raise AttributeError("Project Recovery Model: Passed checkpoint is not a valid instance for finding a row")

    def start_mantid_normally(self):
        """
        Closes the current view
        """
        self.presenter.close_view()

    def recover_selected_checkpoint(self, selected):
        """
        Recover the passed checkpoint
        :param selected: String; Checkpoint name to be recovered
        """
        # If this is a valid file then it should only be the checkpoint here
        if os.path.exists(selected):
            selected = os.path.basename(selected)

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
            # Fail "Silently" by setting failed run to true, setting checkpoint to tried and closing the view.
            logger.debug("Project Recovery: " + str(e))
            self.has_failed_run = True
            self._update_checkpoint_tried(selected)
            self.presenter.close_view()

    def open_selected_in_editor(self, selected):
        """
        Open the passed checkpoint in the editor
        :param selected: String; Checkpoint name to be opened
        """
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
        """
        Check whether recovery has started
        :return: Boolean;
        """
        return self.is_recovery_running

    def decide_last_checkpoint(self):
        """
        Decide which was the last checkpoint to be saved from the choices
        :return: The last checkpoint that was saved from the given options
        """
        checkpoints = self.project_recovery.listdir_fullpath(self.project_recovery.get_pid_folder_to_load_a_checkpoint_from())
        # Sort the checkpoints
        checkpoints.sort()
        return checkpoints[-1]

    def fill_rows(self):
        """
        Fill the given rows, from the checkpoints that are saved on the disk, by this point it is known that recovery
        should occur
        """
        # Clear the rows
        self.rows = []

        pid_folder = self.project_recovery.get_pid_folder_to_load_a_checkpoint_from()
        if pid_folder is None:
            return
        paths = self.project_recovery.listdir_fullpath(os.path.join(self.project_recovery.recovery_directory_hostname, pid_folder))

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
        """
        :return: int; The maximum number of checkpoints project recovery should allow
        """
        try:
            return int(ConfigService.getString("projectRecovery.numberOfCheckpoints"))
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail silently and return 5 (the default)
            return DEFAULT_NUM_CHECKPOINTS

    def _fill_row(self, path, checkpoint_name):
        """
        Add a list to self.rows list to represent the rows
        :param path: Srting; The path in which to fill from
        :param checkpoint_name: String; The checkpoint name to use to fill this row
        """
        num_of_ws = str(self.find_number_of_workspaces_in_directory(path))
        checked = "No"
        self.rows.append([checkpoint_name, num_of_ws, checked])

    def _update_checkpoint_tried(self, checkpoint_path):
        """
        Update that the checkpoint has been tried and show that in the self.rows
        :param checkpoint_path: String; Path to the checkpoint
        :return: None; when successful else raises RuntimeError
        """
        checkpoint_name = os.path.basename(checkpoint_path)

        # Assume if there is a T then it is a checkpoint and it needs to be replaced with a space
        checkpoint_name = checkpoint_name.replace("T", " ")

        for row in self.rows:
            if row[0] == checkpoint_name:
                row[2] = "Yes"
                return
        raise RuntimeError("Project Recovery: Passed checkpoint name for update of GUI was incorrect: " + checkpoint_name)

    def _start_recovery_of_checkpoint(self, checkpoint):
        """
        Starts the actual recovery of a given checkpoint
        :param checkpoint: The checkpoint to recover.
        """
        self.project_recovery.load_checkpoint(checkpoint)

        # If the run failed update the tried else it wasn't a failure and
        if self.has_failed_run:
            self._update_checkpoint_tried(self.selected_checkpoint)

        self.is_recovery_running = False
        self.presenter.close_view()

    @Slot()
    def exec_error(self):
        """
        Set self.has_failed_run to True
        """
        self.has_failed_run = True
