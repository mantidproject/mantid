# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import QMetaObject, Q_ARG, Qt
from qtpy.QtWidgets import QApplication

from mantid.kernel import logger
from mantid.simpleapi import AlgorithmManager
from mantidqt.project.projectloader import ProjectLoader
from workbench.projectrecovery.recoverygui.projectrecoverypresenter import ProjectRecoveryPresenter


class ProjectRecoveryLoader(object):
    def __init__(self, project_recovery, main_window, multi_file_interpreter):
        self.pr = project_recovery
        self.main_window = main_window
        self.multi_file_interpreter = multi_file_interpreter

        self.recovery_presenter = None

    def attempt_recovery(self):
        self.recovery_presenter = ProjectRecoveryPresenter(self)

        success = self.recovery_presenter.start_recovery_view(parent=self.main_window)

        if not success:
            while not success:
                success = self.recovery_presenter.start_recovery_failure(parent=self.main_window)

        pid_dir = self.pr.get_pid_folder_to_load_a_checkpoint_from()
        # Restart project recovery as we stay synchronous
        self.pr.clear_all_unused_checkpoints(pid_dir)
        self.pr.start_recovery_thread()

    def load_checkpoint(self, directory):
        """
        Load in a checkpoint that was saved by project recovery
        :param directory: The directory from which to recover
        :return: True, when recovery fails, False when recovery succeeds
        """
        # Start Regen of workspaces
        self._regen_workspaces(directory)

        # Load interfaces back. This must occur after workspaces have been loaded back because otherwise some
        # interfaces may be unable to be recreated.
        self._load_project_interfaces(directory)

    def open_checkpoint_in_script_editor(self, checkpoint):
        self._compile_recovery_script(directory=checkpoint)
        self._open_script_in_editor(self.pr.recovery_order_workspace_history_file)

    def _load_project_interfaces(self, directory):
        project_loader = ProjectLoader(self.pr.recovery_file_ext)
        # This method will only load interfaces/plots if all workspaces that are expected have been loaded successfully
        if not project_loader.load_project(directory=directory, load_workspaces=False):
            logger.error("Project Recovery: Not all workspaces were recovered successfully, any interfaces requiring "
                         "lost workspaces are not opened")

    def _regen_workspaces(self, directory):
        self._compile_recovery_script(directory)

        # Open it in the editor and run it
        self._open_script_in_editor(self.pr.recovery_order_workspace_history_file)
        self._run_script_in_open_editor()

    def _compile_recovery_script(self, directory):
        alg_name = "OrderWorkspaceHistory"
        alg = AlgorithmManager.createUnmanaged(alg_name, 1)
        alg.initialize()
        alg.setChild(True)
        alg.setLogging(False)
        alg.setRethrows(False)
        alg.setProperty("RecoveryCheckpointFolder", directory)
        alg.setProperty("OutputFilePath", self.pr.recovery_order_workspace_history_file)
        alg.execute()

    def _open_script_in_editor(self, script):
        # Get number of lines
        with open(script) as f:
            num_lines = len(f.readlines())

        self._open_script_in_editor_call(script)

        # Force program to process events
        QApplication.processEvents()

        self.recovery_presenter.connect_progress_bar_to_recovery_view()
        self.recovery_presenter.set_up_progress_bar(num_lines)

    def _open_script_in_editor_call(self, script):
        QMetaObject.invokeMethod(self.multi_file_interpreter, "open_file_in_new_tab", Qt.AutoConnection,
                                 Q_ARG(str, script))

        # Force program to process events so the invoked method is called
        QApplication.processEvents()

    def _run_script_in_open_editor(self):
        # Make sure that exec_error is connected with sig_exec_error on the multifileinterpreter,
        # to flag the checkpoint as failed to load if an error occurs.
        self.multi_file_interpreter.current_editor().sig_exec_error.connect(self.recovery_presenter.model.exec_error)

        # Actually execute the current tab
        QMetaObject.invokeMethod(self.multi_file_interpreter, "execute_current_async_blocking",
                                 Qt.AutoConnection)

        # Force program to process events so the invoked method is called
        QApplication.processEvents()