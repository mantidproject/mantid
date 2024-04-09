# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os

from qtpy.QtWidgets import QApplication

from mantid.api import AlgorithmManager
from mantid.kernel import logger
from mantidqt.project.projectloader import ProjectLoader
from workbench.projectrecovery.recoverygui.projectrecoverypresenter import ProjectRecoveryPresenter


class ProjectRecoveryLoader(object):
    def __init__(self, project_recovery, main_window, multi_file_interpreter):
        self.pr = project_recovery
        self.main_window = main_window
        self.multi_file_interpreter = multi_file_interpreter

        self.recovery_presenter = None

    def attempt_recovery(self):
        """
        Attempt to recover by launching the GUI relevant to project recovery and repeating with failure if failure
        occurs.
        """

        # Block updates to the algorithm progress bar whilst recovery is running to avoid queueing Qt updates
        # to the Workbench's progress bar
        with self.main_window.algorithm_selector.block_progress_widget_updates():
            self.recovery_presenter = ProjectRecoveryPresenter(self.pr)

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
        """
        Open the passed checkpoint in the script editor
        :param checkpoint: String; path to the checkpoint file
        """
        self._copy_in_recovery_script(directory=checkpoint)
        self._open_script_in_editor(self.pr.recovery_order_workspace_history_file)

    def _load_project_interfaces(self, directory):
        """
        Load the passed project interfaces in the given directory, using the project mantidqt package
        :param directory: String; Path to the directory in which the saved file is present
        """
        project_loader = ProjectLoader(self.pr.recovery_file_ext)
        # This method will only load interfaces/plots if all workspaces that are expected have been loaded successfully
        file_name = os.path.join(directory, (os.path.basename(directory) + self.pr.recovery_file_ext))
        if not project_loader.load_project(file_name=file_name, load_workspaces=False):
            logger.error(
                "Project Recovery: Not all workspaces were recovered successfully, any interfaces requiring "
                "lost workspaces are not opened"
            )

    def _regen_workspaces(self, directory):
        """
        Make all the calls that are required to start the regeneration of the workspaces from the stored scripts
        :param directory: String; Path to the directory in which resides the workspace.py files
        """
        self._copy_in_recovery_script(directory)

        # Open it in the editor and run it
        self._open_script_in_editor(self.pr.recovery_order_workspace_history_file)
        self._run_script_in_open_editor()

    def _copy_in_recovery_script(self, directory):
        """
        Move the saved recovery script to the ~./mantid or %APPDATA%/mantidproject/mantid folder based on your OS
        :param directory: String; The directory in which the load_workspaces.py files exists
        """
        saved_recovery_script = os.path.join(directory, "load_workspaces.py")

        if os.path.exists(saved_recovery_script):
            with open(saved_recovery_script, "r") as reader, open(self.pr.recovery_order_workspace_history_file, "w") as writer:
                writer.write(reader.read())
        elif os.path.exists(directory):
            # This is to ensure compatibility with backups from version 6.8 and earlier.
            # Prior to 6.9, one python file would be saved for each workspace in the ADS.
            alg_name = "OrderWorkspaceHistory"
            alg = AlgorithmManager.createUnmanaged(alg_name, 1)
            alg.initialize()
            alg.setChild(True)
            alg.setLogging(False)
            alg.setRethrows(False)
            alg.setProperty("RecoveryCheckpointFolder", directory)
            alg.setProperty("OutputFilePath", self.pr.recovery_order_workspace_history_file)
            alg.execute()

        self.multi_file_interpreter.mark_file_change_as_ours(self.pr.recovery_order_workspace_history_file)

    def _open_script_in_editor(self, script):
        """
        Open the passed script in the Workbench script editor
        :param script: String; Path to the script
        """
        # Get number of lines
        with open(script) as f:
            num_lines = len(f.readlines())

        self._open_script_in_editor_call(script)

        # Force program to process events
        QApplication.processEvents()

        self.recovery_presenter.connect_progress_bar_to_recovery_view()
        self.recovery_presenter.set_up_progress_bar(num_lines)

    def _open_script_in_editor_call(self, script):
        """
        Open script in editor method invokation to guarantee it occurring on the correct thread.
        :param script: String; Path to the script
        :return:
        """
        self.multi_file_interpreter.open_file_in_new_tab(script)
        self.multi_file_interpreter.reload_file(script)

    def _run_script_in_open_editor(self):
        """
        Run the currently open script that is in the editor
        """
        # Make sure that exec_error is connected with sig_exec_error on the multifileinterpreter,
        # to flag the checkpoint as failed to load if an error occurs.
        self.multi_file_interpreter.current_editor().sig_exec_error.connect(self.recovery_presenter.model.exec_error)

        # Actually execute the current tab
        self.multi_file_interpreter.execute_current_async_blocking()
