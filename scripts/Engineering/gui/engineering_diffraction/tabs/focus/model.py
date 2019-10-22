# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from os import path

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections, path_handling
from mantid.simpleapi import EnggFocus, Load, logger, AnalysisDataService as Ads, SaveNexus
from mantidqt.plotting.functions import plot

SAMPLE_RUN_WORKSPACE_NAME = "engggui_focusing_input_ws"
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_bank_"


class FocusModel(object):
    def focus_run(self, sample_path, banks, plot_output, instrument, rb_number, current_calib):
        """
        Focus some data using the current calibration.
        :param sample_path: The path to the data to be focused.
        :param banks: The banks that should be focused.
        :param plot_output: True if the output should be plotted.
        :param instrument: The instrument that the data came from.
        :param rb_number: The experiment number, used to create directories. Can be None
        :param current_calib: The current calibration loaded in the calibration tab.
        """
        vanadium_path = current_calib["vanadium_path"]
        if vanadium_path is None:
            return
        integration_workspace, curves_workspace = vanadium_corrections.fetch_correction_workspaces(
            vanadium_path, instrument)
        sample_workspace = self._load_focus_sample_run(sample_path)
        for name in banks:
            output_workspace_name = FOCUSED_OUTPUT_WORKSPACE_NAME + str(name)
            self._run_focus(sample_workspace, output_workspace_name, integration_workspace,
                            curves_workspace, name)
            # Plot the output
            if plot_output:
                self._plot_focused_workspace(output_workspace_name)
            # Save the output to the file system.
            self._save_focused_output_files_as_nexus(instrument, sample_path, name,
                                                     output_workspace_name, rb_number)

    @staticmethod
    def _run_focus(input_workspace, output_workspace, vanadium_integration_ws, vanadium_curves_ws,
                   bank):
        try:
            return EnggFocus(InputWorkspace=input_workspace,
                             OutputWorkspace=output_workspace,
                             VanIntegrationWorkspace=vanadium_integration_ws,
                             VanCurvesWorkspace=vanadium_curves_ws,
                             Bank=bank)
        except RuntimeError as e:
            logger.error(
                "Error in focusing, Could not run the EnggFocus algorithm successfully for bank " +
                str(bank) + ". Error Description: " + str(e))
            raise RuntimeError()

    @staticmethod
    def _load_focus_sample_run(sample_path):
        try:
            return Load(Filename=sample_path, OutputWorkspace=SAMPLE_RUN_WORKSPACE_NAME)
        except RuntimeError as e:
            logger.error(
                "Error while loading sample data for focusing. Could not load the sample with filename: "
                + sample_path + ". Error Description: " + str(e))
            raise RuntimeError

    @staticmethod
    def _plot_focused_workspace(focused):
        focused_wsp = Ads.retrieve(focused)
        plot([focused_wsp], wksp_indices=[0])

    def _save_focused_output_files_as_nexus(self, instrument, sample_path, bank, sample_workspace,
                                            rb_number):
        """
        Save a focused workspace to the file system. Saves a separate copy to a User directory if an rb number has been
        set.
        :param instrument: The instrument the data is from.
        :param sample_path: The path to the data file that was focused.
        :param bank: The name of the bank being saved.
        :param sample_workspace: The name of the workspace to be saved.
        :param rb_number: Usually an experiment id, defines the name of the user directory.
        """
        nexus_output_path = path.join(
            path_handling.OUT_FILES_ROOT_DIR, "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, ".nxs"))
        SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        if rb_number is not None:
            nexus_output_path = path.join(
                path_handling.OUT_FILES_ROOT_DIR, "User", rb_number, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, ".nxs"))
            SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)

    @staticmethod
    def _generate_output_file_name(instrument, sample_path, bank, suffix):
        run_no = path_handling.get_run_number_from_path(sample_path, instrument)
        return instrument + "_" + run_no + "_bank_" + bank + suffix
