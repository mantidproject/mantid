# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from mantidqt.project.recovery.recoverygui.projectrecoverymodel import ProjectRecoveryModel
from mantidqt.project.recovery.recoverygui.projectrecoverywidgetview import ProjectRecoveryWidgetView

class ProjectRecoveryPresenter(object):
    def __init__(self, project_recovery):
        self.project_recovery = project_recovery

        self.current_view = None
        self.model = ProjectRecoveryModel(self.project_recovery)

        self.start_mantid_normally_called = False

    def start_recovery_view(self):
        # Only start this view if there is nothing as current_view
        if self.current_view is not None:
            raise RuntimeError("Project Recovery: A view is already open")

        try:
            self.current_view = ProjectRecoveryWidgetView(self)
            self.current_view.exec_()
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            return True

        # If start mantid normally we want to cancel
        if self.start_mantid_normally_called:
            return False

        # If run has failed and recovery is not running
        if self.model.get_failed_run():
            return True

        return False

    def start_recovery_failure(self):
        if self.current_view is not None:
            raise RuntimeError("Project Recovery: A view is already open")

        try:
            self.current_view = RecoveryFailureView(self)
            self.current_view.exec_()
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            return True

        # If start mantid normally we want to cancel
        if self.start_mantid_normally_called:
            return False

        # If run has failed and recovery is not running
        if self.model.get_failed_run():
            return True

        return False

    def get_row(self, index):
        return self.model.get_row(index)

    def recover_last(self):
        if self.model.has_recovery_started():
            return
        checkpoint_to_recover = self.model.decide_last_checkpoint()
        self.model.recover_selected_checkpoint(checkpoint_to_recover)

    def open_last_in_editor(self):
        if self.model.has_recovery_started():
            return
        checkpoint_to_recover = self.model.decide_last_checkpoint()
        self.model.open_selected_in_editor(checkpoint_to_recover)

    def start_mantid_normally(self):
        self.start_mantid_normally_called = True
        self.model.start_mantid_normally()

    def recover_selected_checkpoint(self, selected):
        if self.model.has_recovery_started():
            return
        self.model.recover_selected_checkpoint(selected)

    def open_selected_checkpoint_in_editor(self, selected):
        if self.model.has_recovery_started():
            return
        self.model.open_selected_in_editor(selected)

    def close_view(self):
        self.current_view = None
        # todo: actively delete the view

    def connect_progress_bar_to_recovery_view(self):
        self.current_view.connect_progress_bar()

    def emit_abort_script(self):
        self.current_view.emit_abort_script()

    def change_start_mantid_to_cancel_label(self):
        self.current_view.change_start_mantid_button("Cancel Recovery")

    def fill_all_rows(self):
        # We only want this to run once so only if current_view is RecoveryView
        if isinstance(self.current_view, ProjectRecoveryWidgetView):
            self.model.fill_rows()

    def set_up_progress_bar(self, max_value):
        self.current_view.set_progress_bar_maximum(max_value)

    def get_number_of_checkpoints(self):
        return self.model.get_number_of_checkpoints()
