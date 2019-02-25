# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import datetime
import os
from threading import Timer

from mantid.api import AnalysisDataService as ADS, WorkspaceGroup
from mantid.kernel import logger, UsageService
from mantid.simpleapi import AlgorithmManager
from mantidqt.project.projectsaver import ProjectSaver
from workbench.app.windowfinder import find_all_windows_that_are_savable


# To ignore an algorithm in project recovery please put it's name here, this is done to stop these algorithm
# calls from being saved. e.g. MonitorLiveData is ignored because StartLiveData is the only one that is needed
# to restart this workspace.
ALGS_TO_IGNORE = ["MonitorLiveData", "EnggSaveGSASIIFitResultsToHDF5",
                  "EnggSaveSinglePeakFitResultsToHDF5", "ExampleSaveAscii", "SANSSave", "SaveANSTOAscii",
                  "SaveAscii", "SaveBankScatteringAngles", "SaveCSV", "SaveCalFile", "SaveCanSAS1D",
                  "SaveDaveGrp", "SaveDetectorsGrouping", "SaveDiffCal", "SaveDiffFittingAscii",
                  "SaveDspacemap", "SaveFITS", "SaveFocusedXYE", "SaveFullprofResolution", "SaveGDA",
                  "SaveGEMMAUDParamFile", "SaveGSASInstrumentFile", "SaveGSS", "SaveHKL",
                  "SaveILLCosmosAscii", "SaveISISNexus", "SaveIsawDetCal", "SaveIsawPeaks",
                  "SaveIsawQvector", "SaveIsawUB", "SaveLauenorm", "SaveMD", "SaveMDWorkspaceToVTK",
                  "SaveMask", "SaveNISTDAT", "SaveNXSPE", "SaveNXTomo", "SaveNXcanSAS", "SaveNexus",
                  "SaveNexusPD", "SaveNexusProcessed", "SaveOpenGenieAscii", "SavePAR", "SavePDFGui",
                  "SavePHX", "SaveParameterFile", "SavePlot1D", "SavePlot1DAsJson", "SaveRKH",
                  "SaveReflCustomAscii", "SaveReflThreeColumnAscii", "SaveReflections",
                  "SaveReflectometryAscii", "SaveSESANS", "SaveSPE", "SaveTBL", "SaveToSNSHistogramNexus",
                  "SaveVTK", "SaveVulcanGSS", "SaveYDA", "SaveZODS"]

# If you want to ignore an algorithms' property, then add to the string below in the format:
# "AlgorithmName + PropertyName". The final string should look like "a + b , c + d, ...".
# This uses string representation to pass to the C++ algorithm. The outer delimiter is `,`
# and the inner delimiter is `+` for the list of lists. e.g. [[a, b],[c, d]] = "a + b , c + d".
ALG_PROPERTIES_TO_IGNORE = "StartLiveData + MonitorLiveData"


class ProjectRecoverySaver(object):
    def __init__(self, project_recovery, global_figure_manager):
        self.pr = project_recovery
        self.gfm = global_figure_manager
        self._timer_thread = Timer(self.pr.time_between_saves, self.recovery_save)

    def recovery_save(self):
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
            recovery_dir = os.path.join(self.pr.recovery_directory_pid,
                                        datetime.datetime.now().strftime('%d-%m-%YT%H-%M-%S'))
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
        self._timer_thread = Timer(self.pr.time_between_saves, self.recovery_save)
        self._timer_thread.start()

    def _save_workspaces(self, directory):
        # Get all present workspaces
        ws_list = ADS.getObjectNames()

        if len(ws_list) == 0:
            return

        start_time = UsageService.getStartTime().toISO8601String()

        alg_name = "GeneratePythonScript"
        alg = AlgorithmManager.createUnmanaged(alg_name, 1)
        alg.setChild(True)
        alg.setLogging(False)

        for index, ws in enumerate(ws_list):
            if self._empty_group_workspace(ws):
                continue

            filename = str(index) + ".py"
            filename = os.path.join(directory, filename)

            alg.initialize()
            alg.setProperty("AppendTimestamp", True)
            alg.setProperty("InputWorkspace", ws)
            alg.setPropertyValue("Filename", filename)
            alg.setPropertyValue("StartTimestamp", start_time)
            alg.setProperty("IgnoreTheseAlgs", ALGS_TO_IGNORE)
            alg.setProperty("IgnoreTheseAlgProperties", ALG_PROPERTIES_TO_IGNORE)

            alg.execute()

    @staticmethod
    def _empty_group_workspace(ws):
        if isinstance(ws, WorkspaceGroup) and len(ws.getNames()) == 0:
            return True

    def _save_project(self, directory, interfaces_list=None):
        project_saver = ProjectSaver(project_file_ext=self.pr.recovery_file_ext)

        plots = self.gfm.figs
        if interfaces_list is None:
            interfaces_list = find_all_windows_that_are_savable()

        project_saver.save_project(directory, workspace_to_save=None, plots_to_save=plots,
                                   interfaces_to_save=interfaces_list, project_recovery=False)

    def _add_lock_file(self, directory):
        # Create the file
        open(os.path.join(directory, self.pr.lock_file_name), 'a').close()

    def _remove_lock_file(self, directory):
        lock_file = os.path.join(directory, self.pr.lock_file_name)
        if os.path.exists(lock_file):
            os.remove(lock_file)

    def start_recovery_thread(self):
        if not self.pr.recovery_enabled:
            logger.debug("Project Recovery: Recovery thread not started as recovery is disabled")
            return

        if not self.pr.thread_on:
            self._timer_thread.start()
            self.pr.thread_on = True

    def stop_recovery_thread(self):
        if self._timer_thread is not None:
            self._timer_thread.cancel()
            self.pr.thread_on = False
