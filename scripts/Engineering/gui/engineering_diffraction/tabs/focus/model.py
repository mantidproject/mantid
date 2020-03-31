# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import csv
from os import path, makedirs
from matplotlib import gridspec
import matplotlib.pyplot as plt

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections, path_handling
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantid.simpleapi import EnggFocus, logger, AnalysisDataService as Ads, SaveNexus, SaveGSS, SaveFocusedXYE, \
    LoadAscii

SAMPLE_RUN_WORKSPACE_NAME = "engggui_focusing_input_ws"
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_bank_"


class FocusModel(object):
    def focus_run(self, sample_paths, banks, plot_output, instrument, rb_num, spectrum_numbers):
        """
        Focus some data using the current calibration.
        :param sample_paths: The paths to the data to be focused.
        :param banks: The banks that should be focused.
        :param plot_output: True if the output should be plotted.
        :param instrument: The instrument that the data came from.
        :param rb_num: The experiment number, used to create directories. Can be None
        :param spectrum_numbers: The specific spectra that should be focused. Used instead of banks.
        """
        if not Ads.doesExist(vanadium_corrections.INTEGRATED_WORKSPACE_NAME) and not Ads.doesExist(
                vanadium_corrections.CURVES_WORKSPACE_NAME):
            return
        integration_workspace = Ads.retrieve(vanadium_corrections.INTEGRATED_WORKSPACE_NAME)
        curves_workspace = Ads.retrieve(vanadium_corrections.CURVES_WORKSPACE_NAME)
        output_workspaces = []  # List of collated workspaces to plot.
        full_calib_path = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                                      path_handling.ENGINEERING_PREFIX, "full_calibration")
        if full_calib_path is not None and path.exists(full_calib_path):
            full_calib_workspace = LoadAscii(full_calib_path, OutputWorkspace="det_pos", Separator="Tab")
        else:
            full_calib_workspace = None
        if spectrum_numbers is None:
            for sample_path in sample_paths:
                sample_workspace = path_handling.load_workspace(sample_path)
                run_no = path_handling.get_run_number_from_path(sample_path, instrument)
                workspaces_for_run = []
                for name in banks:
                    output_workspace_name = str(run_no) + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + str(name)
                    self._run_focus(sample_workspace, output_workspace_name, integration_workspace,
                                    curves_workspace, name, full_calib_workspace)
                    workspaces_for_run.append(output_workspace_name)
                    # Save the output to the file system.
                    self._save_output(instrument, sample_path, name, output_workspace_name, rb_num)
                output_workspaces.append(workspaces_for_run)
                self._output_sample_logs(instrument, run_no, sample_workspace, rb_num)
        else:
            for sample_path in sample_paths:
                sample_workspace = path_handling.load_workspace(sample_path)
                run_no = path_handling.get_run_number_from_path(sample_path, instrument)
                output_workspace_name = str(run_no) + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + "cropped"
                self._run_focus(sample_workspace, output_workspace_name, integration_workspace,
                                curves_workspace, None, full_calib_workspace, spectrum_numbers)
                output_workspaces.append([output_workspace_name])
                self._save_output(instrument, sample_path, "cropped", output_workspace_name, rb_num)
                self._output_sample_logs(instrument, run_no, sample_workspace, rb_num)
        # Plot the output
        if plot_output:
            for ws_names in output_workspaces:
                self._plot_focused_workspaces(ws_names)

    @staticmethod
    def _run_focus(input_workspace,
                   output_workspace,
                   vanadium_integration_ws,
                   vanadium_curves_ws,
                   bank,
                   full_calib_ws=None,
                   spectrum_numbers=None):
        kwargs = {
            "InputWorkspace": input_workspace,
            "OutputWorkspace": output_workspace,
            "VanIntegrationWorkspace": vanadium_integration_ws,
            "VanCurvesWorkspace": vanadium_curves_ws
        }
        if full_calib_ws is not None:
            kwargs["DetectorPositions"] = full_calib_ws

        if bank is None:
            kwargs["SpectrumNumbers"] = spectrum_numbers
        else:
            kwargs["Bank"] = bank

        try:
            return EnggFocus(**kwargs)
        except RuntimeError as e:
            logger.error(
                "Error in focusing, Could not run the EnggFocus algorithm successfully for bank "
                + str(bank) + ". Error Description: " + str(e))
            raise RuntimeError()

    @staticmethod
    def _plot_focused_workspaces(focused_workspaces):
        fig = plt.figure()
        gs = gridspec.GridSpec(1, len(focused_workspaces))
        plots = [
            fig.add_subplot(gs[i], projection="mantid") for i in range(len(focused_workspaces))
        ]

        for ax, ws_name in zip(plots, focused_workspaces):
            ax.plot(Ads.retrieve(ws_name), wkspIndex=0)
            ax.set_title(ws_name)
        fig.show()

    def _save_output(self, instrument, sample_path, bank, sample_workspace, rb_num):
        """
        Save a focused workspace to the file system. Saves separate copies to a User directory if an rb number has
        been set.
        :param instrument: The instrument the data is from.
        :param sample_path: The path to the data file that was focused.
        :param bank: The name of the bank being saved.
        :param sample_workspace: The name of the workspace to be saved.
        :param rb_num: Usually an experiment id, defines the name of the user directory.
        """
        self._save_focused_output_files_as_nexus(instrument, sample_path, bank, sample_workspace,
                                                 rb_num)
        self._save_focused_output_files_as_gss(instrument, sample_path, bank, sample_workspace,
                                               rb_num)
        self._save_focused_output_files_as_topas_xye(instrument, sample_path, bank, sample_workspace,
                                                     rb_num)

    def _save_focused_output_files_as_gss(self, instrument, sample_path, bank, sample_workspace,
                                          rb_num):
        gss_output_path = path.join(
            path_handling.get_output_path(), "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, ".gss"))
        SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)
        if rb_num:
            gss_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, ".gss"))
            SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)

    def _save_focused_output_files_as_nexus(self, instrument, sample_path, bank, sample_workspace,
                                            rb_num):
        nexus_output_path = path.join(
            path_handling.get_output_path(), "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, ".nxs"))
        SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        if rb_num:
            nexus_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, ".nxs"))
            SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)

    def _save_focused_output_files_as_topas_xye(self, instrument, sample_path, bank,
                                                sample_workspace, rb_num):
        xye_output_path = path.join(
            path_handling.get_output_path(), "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, ".abc"))
        SaveFocusedXYE(InputWorkspace=sample_workspace,
                       Filename=xye_output_path,
                       SplitFiles=False,
                       Format="TOPAS")
        if rb_num:
            xye_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, ".abc"))
            SaveFocusedXYE(InputWorkspace=sample_workspace,
                           Filename=xye_output_path,
                           SplitFiles=False,
                           Format="TOPAS")

    @staticmethod
    def _output_sample_logs(instrument, run_number, workspace, rb_num):
        def write_to_file():
            with open(output_path, "w", newline="") as logfile:
                writer = csv.writer(logfile, ["Sample Log", "Avg Value"])
                for log in output_dict:
                    writer.writerow([log, output_dict[log]])

        output_dict = {}
        sample_run = workspace.getRun()
        log_names = sample_run.keys()
        # Collect numerical sample logs.
        for name in log_names:
            try:
                output_dict[name] = sample_run.getPropertyAsSingleValue(name)
            except ValueError:
                logger.information(f"Could not convert {name} to a numerical value. It will not be included in the "
                                   f"sample logs output file.")
        focus_dir = path.join(path_handling.get_output_path(), "Focus")
        if not path.exists(focus_dir):
            makedirs(focus_dir)
        output_path = path.join(focus_dir, (instrument + "_" + run_number + "_sample_logs.csv"))
        write_to_file()
        if rb_num:
            focus_user_dir = path.join(path_handling.get_output_path(), "User", rb_num, "Focus")
            if not path.exists(focus_user_dir):
                makedirs(focus_user_dir)
            output_path = path.join(focus_user_dir, (instrument + "_" + run_number + "_sample_logs.csv"))
            write_to_file()

    @staticmethod
    def _generate_output_file_name(instrument, sample_path, bank, suffix):
        run_no = path_handling.get_run_number_from_path(sample_path, instrument)
        return instrument + "_" + run_no + "_bank_" + bank + suffix
