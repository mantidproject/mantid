# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os
import getpass
from threading import Timer
import datetime
from glob import glob
import shutil
import psutil

from mantid.kernel import ConfigService, logger
from mantid.api import AnalysisDataService as ADS, WorkspaceGroup
from mantidqt.project.projectsaver import ProjectSaver
from mantidqt.project.projectloader import ProjectLoader
from mantid.simpleapi import OrderWorkspaceHistory, AlgorithmManager


class ProjectRecovery(object):
    def __init__(self, globalfiguremanager, window_finder, multifileinterpreter):
        self.recovery_directory = os.path.join(ConfigService.getAppDataDir(), "workbench-recovery")
        self.recovery_directory_hostname = os.path.join(self.recovery_directory, getpass.getuser())
        self.recovery_directory_pid = os.path.join(self.recovery_directory_hostname, os.getpid())

        self.recovery_order_workspace_history_file = os.path.join(ConfigService.getAppDataDir(), "ordered_recovery.py")

        self.time_between_saves = 300.0  # seconds
        self._timer_thread = Timer(self.time_between_saves, self.recovery_save)

        self.recovery_file_ext = ".recfile"
        self.lock_file_name = "projectrecovery.lock"

        self.gfm = globalfiguremanager
        self.interface_finding_func = window_finder
        self.multi_file_interpreter = multifileinterpreter

        self.algs_to_ignore = ["MonitorLiveData", "EnggSaveGSASIIFitResultsToHDF5",
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
        # To ignore a property you need to first put the algorithm name in the first part of the vector for which you
        # want to ignore the property for then the property name in the second part of the vector 0 and 1 as indexes
        # respectively. This uses string representation to pass to the C++ algorithm. The outer delimiter is `,`
        # and the inner delimiter is `+` for the list of lists. e.g. [[a, b],[c, d]] = "a + b , c + d".
        self.alg_properties_to_ignore = "StartLiveData + PropertyName"

        # The recovery GUI's presenter is set when needed
        self.recovery_presenter = None

    ######################################################
    #  Utility
    ######################################################

    def start_recovery_thread(self):
        self._timer_thread.start()

    def stop_recovery_thread(self):
        self._timer_thread.cancel()

    @staticmethod
    def _remove_empty_folders_from_dir(directory):
        folders = glob(os.path.join(directory, "*", ""))
        for folder in folders:
            try:
                os.rmdir(folder)
            except OSError:
                # Fail silently as expected for all folders
                pass

    @staticmethod
    def _remove_all_folders_from_dir(directory):
        shutil.rmtree(directory)

    ######################################################
    #  Saving
    ######################################################

    def recovery_save(self):
        # todo: Get a list of interfaces that could be saved and pass it down to save_project but check it's length is
        #  greater than 0 for this check
        if len(ADS.getObjectNames()) == 0:
            logger.debug("Project Recovery: Nothing to save")
            self._spin_off_another_time_thread()
            return

        logger.debug("Project Recovery: Saving Started")

        # Create directory for save location
        recovery_dir = os.path.join(self.recovery_directory_pid, datetime.datetime.now().isoformat())
        if not os.path.exists(recovery_dir):
            os.makedirs(recovery_dir)

        self._add_lock_file(directory=recovery_dir)

        # Save workspaces
        self._save_workspaces(directory=recovery_dir)

        # Save project
        self._save_project(directory=recovery_dir)

        self._remove_lock_file(directory=recovery_dir)

        logger.debug("Project Recovery: Saving finished")

        # Spin off another timer thread
        self._spin_off_another_time_thread()

    def _spin_off_another_time_thread(self):
        self._timer_thread = Timer(self.time_between_saves, self.recovery_save)
        self._timer_thread.start()

    def _save_workspaces(self, directory):
        # Get all present workspaces
        ws_list = ADS.getObjectNames()

        if len(ws_list) == 0:
            return

        start_time = datetime.datetime.now().isoformat()

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
            alg.setProperty("IgnoreTheseAlgs", self.algs_to_ignore)
            alg.setProperty("IgnoreTheseAlgProperties", self.alg_properties_to_ignore)

            alg.execute()

    @staticmethod
    def _empty_group_workspace(ws):
        if isinstance(ws, WorkspaceGroup) and len(ws.getNames()) == 0:
            return True

    def _save_project(self, directory):
        project_saver = ProjectSaver(project_file_ext=self.recovery_file_ext)

        plots = self.gfm.figs
        interfaces_list = self.interface_finding_func()

        project_saver.save_project(directory, workspace_to_save=None, plots_to_save=plots,
                                   interfaces_to_save=interfaces_list, save_workspaces=False)

    def _add_lock_file(self, directory):
        # Create the file
        open(os.path.join(directory, self.lock_file_name), 'a').close()

    def _remove_lock_file(self, directory):
        lock_file = os.path.join(directory, self.lock_file_name)
        if os.path.exists(lock_file):
            os.remove(lock_file)

    ######################################################
    #  Decision
    ######################################################

    def check_for_recover_checkpoint(self):
        try:
            # Clean directory first
            self._remove_empty_folders_from_dir(self.recovery_directory_hostname)

            checkpoints = os.listdir(self.recovery_directory_hostname)
            return len(checkpoints) != 0 and len(checkpoints) > self._number_of_workbench_processes()
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                raise
            # fail silently and return false
            return False

    @staticmethod
    def _number_of_workbench_processes():
        if os.name == 'nt':  # Windows packaged and development
            executable_names = ["launch_workbench.pyw", "workbench-script.pyw"]
        else:  # Confirmed on Ubuntu 18.04 Dev and MacOS
            executable_names = ["workbench", "workbench-script"]

        total_mantids = 0
        for proc in psutil.process_iter():
            process_name = os.path.basename(os.path.normpath(proc.cmdline()[1]))
            if process_name in executable_names:
                total_mantids += 1

        return total_mantids

    ######################################################
    #  Loading
    ######################################################

    def attempt_recovery(self):
        self.recovery_presenter =

    def load_checkpoint(self, directory):
        # Start Regen of workspaces
        self._regen_workspaces(directory)

        # Load interfaces back. This must occur after workspaces have been loaded back because otherwise some interfaces
        # may be unable to be recreated.
        self._load_project_interfaces(directory)

    def _load_project_interfaces(self, directory):
        project_loader = ProjectLoader(self.recovery_file_ext)
        # This method will only load interfaces/plots if all workspaces that are expected have been loaded successfully
        if not project_loader.load_project(directory=directory, load_workspaces=False):
            logger.error("Project Recovery: Not all workspaces were recovered successfully, any interfaces requiring "
                         "lost workspaces are not opened")

    def _regen_workspaces(self, directory):
        self._compile_recovery_script(directory)

        # Open it in the editor and run it
        self._open_script_in_editor(self.recovery_order_workspace_history_file)
        self._run_script_in_open_editor()

    def _compile_recovery_script(self, directory):
        alg_name = "OrderWorkspaceHistory"
        alg = AlgorithmManager.createUnmanaged(alg_name, 1)
        alg.initialize()
        alg.setChild(True)
        alg.setLogging(False)
        alg.setRethrows(True)
        alg.setProperty("RecoverCheckpointFolder", directory)
        alg.setProperty("OutputFilePath", self.recovery_order_workspace_history_file)
        alg.execute()

    def _open_script_in_editor(self, script):
        # Get number of lines
        num_lines = 0
        with open(script) as f:
            num_lines = len(f.readlines())

        # todo: attach this to the GUI process bar

        self.multi_file_interpreter.open_file_in_new_tab(script)

    def _run_script_in_open_editor(self):
        self.multi_file_interpreter.execute_current()

    ######################################################
    #  Checkpoint Repair
    ######################################################

    # todo: Function for removing oldest checkpoint
    # def remove_oldest_checkpoint(self):
    # todo: Function for deleting all checkpoints

    # todo: function to load recovery checkpoint given directory

    # todo: function for putting script in new editor tab
        # todo: function must also setup progress bar of the recoveryGUI with line length

    # todo: Implement repair of checkpoint directory
        # todo: function to find all older checkpoints
            # todo: function to check if a checkpoint is older than a given time
        # todo:  function to check if a directory is locked

    # todo: Function to check if recovery checkpoint PIDs are not in use and remove them if they are

    # todo: Function to remove folders in they are empty from a list of dicrectories
