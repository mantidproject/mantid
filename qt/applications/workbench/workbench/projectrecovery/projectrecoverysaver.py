# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import datetime
import os
from threading import Timer

from mantid.api import AnalysisDataService as ADS, WorkspaceGroup
from mantid.kernel import logger
from mantidqt.project.projectsaver import ProjectSaver
from workbench.utils.windowfinder import find_all_windows_that_are_savable
from workbench.utils.workspacehistorygeneration import get_all_workspace_history_from_ads


class ProjectRecoverySaver(object):
    def __init__(self, project_recovery, global_figure_manager):
        self.pr = project_recovery
        self.gfm = global_figure_manager
        self._timer_thread = Timer(self.pr.time_between_saves, self.recovery_save)

    def recovery_save(self):
        """
        The function to save a recovery checkpoint
        """
        # Set that recovery thread is not running anymore
        self.pr.thread_on = False

        try:
            # Get the interfaces_list
            interfaces_list = find_all_windows_that_are_savable()

            # Check if there is anything to be saved or not
            if len(ADS.getObjectNames()) == 0 and len(interfaces_list) == 0:
                logger.debug("Project Recovery: Nothing to save")
                self._spin_off_another_time_thread()
                return

            logger.debug("Project Recovery: Saving started")

            # Create directory for save location
            recovery_dir = os.path.join(self.pr.recovery_directory_pid, datetime.datetime.now().strftime("%d-%m-%YT%H-%M-%S"))
            if not os.path.exists(recovery_dir):
                os.makedirs(recovery_dir)

            self._add_lock_file(directory=recovery_dir)

            # Save workspaces
            self._save_workspaces(directory=recovery_dir)

            # Save project
            self._save_project(directory=recovery_dir, interfaces_list=interfaces_list)

            self._remove_lock_file(directory=recovery_dir)

            # Clear the oldest checkpoints
            self.pr.remove_oldest_checkpoints()

            logger.debug("Project Recovery: Saving finished")

        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # Fail and print to debugger
            logger.debug("Project Recovery: Failed to save error msg: " + str(e))

        # Spin off another timer thread
        if not self.pr.closing_workbench:
            self._spin_off_another_time_thread()

    def _spin_off_another_time_thread(self):
        """
        Spins off another timer thread, by creating a new Timer thread object and starting it
        """
        self._timer_thread = Timer(self.pr.time_between_saves, self.recovery_save)
        self._timer_thread.start()

    def _save_workspaces(self, directory):
        """
        Save all workspaces present in the ADS to the given directory
        :param directory: String; Path to where to save the workspaces
        """
        ws_history = get_all_workspace_history_from_ads()
        if ws_history:
            script_text = "from mantid.simpleapi import *\n\n" + ws_history

            with open(os.path.join(directory, "load_workspaces.py"), "w") as writer:
                writer.write(script_text)

    @staticmethod
    def _empty_group_workspace(ws):
        """
        Check if the workspace is an empty group workspace
        :param ws: Workspace; Workspace to check
        :return: True if is an empty group workspace
        """
        if isinstance(ws, WorkspaceGroup) and len(ws.getNames()) == 0:
            return True
        else:
            return False

    def _save_project(self, directory, interfaces_list=None):
        """
        Save the project minus the workspaces
        :param directory: String; The directory to save the project to.
        :param interfaces_list: List of Lists of QObject and Encoder;
        """
        project_saver = ProjectSaver(project_file_ext=self.pr.recovery_file_ext)

        # Find all the plots from the global figuremanager
        plots = self.gfm.figs

        if interfaces_list is None:
            interfaces_list = find_all_windows_that_are_savable()

        file_name = os.path.join(directory, (os.path.basename(directory) + self.pr.recovery_file_ext))
        project_saver.save_project(
            file_name=file_name, workspace_to_save=None, plots_to_save=plots, interfaces_to_save=interfaces_list, project_recovery=False
        )

    def _add_lock_file(self, directory):
        """
        Adds a lock file to the directory
        :param directory: String; Path to the directory
        """
        # Create the file
        open(os.path.join(directory, self.pr.lock_file_name), "a").close()

    def _remove_lock_file(self, directory):
        """
        Removes a lock file from the directory
        :param directory: String; Path to the directory
        """
        lock_file = os.path.join(directory, self.pr.lock_file_name)
        if os.path.exists(lock_file):
            os.remove(lock_file)

    def start_recovery_thread(self):
        """
        Starts the recovery thread if it is not already running
        """
        if not self.pr.recovery_enabled:
            logger.debug("Project Recovery: Recovery thread not started as recovery is disabled")
            return

        if not self.pr.thread_on:
            self._timer_thread.start()
            self.pr.thread_on = True

    def stop_recovery_thread(self):
        """
        Cancels the recovery thread if it is running
        """
        if self._timer_thread is not None:
            self._timer_thread.cancel()
            self.pr.thread_on = False
