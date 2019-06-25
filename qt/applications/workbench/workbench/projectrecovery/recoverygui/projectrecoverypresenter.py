# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

from workbench.projectrecovery.recoverygui.projectrecoverymodel import ProjectRecoveryModel
from workbench.projectrecovery.recoverygui.projectrecoverywidgetview import ProjectRecoveryWidgetView
from workbench.projectrecovery.recoverygui.recoveryfailureview import RecoveryFailureView


class ProjectRecoveryPresenter(object):
    def __init__(self, project_recovery, model=None):
        self.model = model if model else ProjectRecoveryModel(project_recovery, self)
        self.current_view = None

        self.project_recovery = project_recovery

        self.start_mantid_normally_called = False
        self.allow_start_mantid_normally = True
        self.open_selected_in_editor_selected = False

    def start_recovery_view(self, parent=None):
        """
        This function will start the recovery view and should be invoked if a recovery needs to be attempted
        :return: False if it was a failure, True if it was a successful recovery or starting mantid normally.
        """
        # Only start this view if there is nothing as current_view
        if self.current_view is not None:
            raise RuntimeError("Project Recovery: A view is already open")

        try:
            self.current_view = ProjectRecoveryWidgetView(self, parent)
            self.current_view.exec_()
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            return False

        # If start mantid normally has been clicked we want to cancel recovery attempts
        if self.start_mantid_normally_called:
            return True

        # If run has failed and recovery is not running then start the recovery GUI
        if self.model.has_failed_run:
            return False

        return True

    def start_recovery_failure(self, parent=None):
        """
        This function will start the recovery failure view and should be invoked upon a failure to recover
        :return: False if it was a failure, True if it was a successful recovery or starting mantid normally.
        """
        if self.current_view is not None:
            raise RuntimeError("Project Recovery: A view is already open")

        # Reset whether model believes recovery has been started.
        rows = self.model.rows
        self.model = ProjectRecoveryModel(self.project_recovery, self)
        self._set_checkpoint_tried_values_from_rows(rows)

        try:
            self.current_view = RecoveryFailureView(self, parent)
            self.current_view.exec_()
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            return False

        # If start mantid normally has been clicked we want to cancel recovery attempts
        if self.start_mantid_normally_called:
            return True

        # If run has failed and recovery is not running then start the recovery GUI
        if self.model.has_failed_run:
            return False

        return True

    def _set_checkpoint_tried_values_from_rows(self, rows):
        """
        Communicate to the model that a row has been tried and make sure it updates
        :param rows: List of List of Strings; The row representation
        """
        for ii in range(0, self.get_number_of_checkpoints()):
            self.model.rows[ii][2] = rows[ii][2]

    def get_row(self, index):
        """
        Return the row at the given index
        :param index: int; given index
        :return: List of Strings; The row representation
        """
        row = self.model.get_row(index)
        if row == []:
            return ["", "", ""]
        else:
            return row

    def recover_last(self):
        """
        Recover the last checkpoint that was saved
        :return: None: If recovery is already running
        """
        if self.model.is_recovery_running:
            return
        checkpoint_to_recover = self.model.decide_last_checkpoint()
        self.model.recover_selected_checkpoint(checkpoint_to_recover)

    def open_last_in_editor(self):
        """
        Open the last checkpoint in editor
        :return: None; If recovery is already running
        """
        if self.model.is_recovery_running:
            return
        checkpoint_to_recover = self.model.decide_last_checkpoint()
        self.model.open_selected_in_editor(checkpoint_to_recover)

    def start_mantid_normally(self):
        """
        Start mantid normally
        :return: None; If you starting normally has been disallowed and the current_view is None.
        """
        # If mantid has started a recovery this will be False, thus meaning it is the "Cancel" button which will in turn
        # abort the script and then "Start mantid normally"
        if not self.allow_start_mantid_normally:
            # If current_view is none then we can't run the following code so return, it could clear up checkpoints that
            # shouldn't be cleared
            if self.current_view is None:
                return
            else:
                self.current_view.emit_abort_script()

        self.start_mantid_normally_called = True
        self.model.start_mantid_normally()

    def recover_selected_checkpoint(self, selected):
        """
        Recover the passed checkpoint
        :param selected: String; The representation of the checkpoint name/folder to be recovered
        :return: None; If recovery is already running
        """
        if self.model.is_recovery_running:
            return
        self.model.recover_selected_checkpoint(selected)

    def open_selected_checkpoint_in_editor(self, selected):
        """
        Open the selected checkpoint
        :param selected: String; The representation of the checkpoint name/folder to be recovered
        :return: None; If recovery is already running
        """
        if self.model.is_recovery_running:
            return
        self.open_selected_in_editor_selected = True
        self.model.open_selected_in_editor(selected)

    def close_view(self):
        """
        Will close the currently open view
        """
        if self.current_view is not None:
            self.current_view.setVisible(False)
            self.current_view = None

    def connect_progress_bar_to_recovery_view(self):
        """
        Connects the progress bar to the multifileinterpreter via the project recovery object
        """
        self.current_view.connect_progress_bar()

    def change_start_mantid_to_cancel_label(self):
        """
        Changes the Start Mantid Normally button to Cancel Recovery
        """
        self.allow_start_mantid_normally = False
        self.current_view.change_start_mantid_button("Cancel Recovery")

    def set_up_progress_bar(self, max_value):
        """
        Sets up the progress bar by setting the max value of the progress value passed
        :param max_value: Int; The max value of the View's progress bar
        """
        self.current_view.set_progress_bar_maximum(max_value)

    def get_number_of_checkpoints(self):
        """
        Gets the number of checkpoints that need to be displayed
        :return: Int; The number of checkpoints
        """
        return self.model.get_number_of_checkpoints()
