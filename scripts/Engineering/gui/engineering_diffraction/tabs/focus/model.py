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
from Engineering.EnggUtils import create_custom_grouping_workspace
from mantid.simpleapi import logger, AnalysisDataService as Ads, SaveNexus, SaveGSS, SaveFocusedXYE, \
    Load, NormaliseByCurrent, Divide, DiffractionFocussing, RebinToWorkspace, DeleteWorkspace, ApplyDiffCal, \
    ConvertUnits, ReplaceSpecialValues

SAMPLE_RUN_WORKSPACE_NAME = "engggui_focusing_input_ws"
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_bank_"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_CAL = "EnginX_NorthBank.cal"
SOUTH_BANK_CAL = "EnginX_SouthBank.cal"


class FocusModel(object):

    def __init__(self):
        self._last_path = None
        self._last_path_ws = None

    def get_last_path(self):
        return self._last_path

    def focus_run(self, sample_paths, banks, plot_output, instrument, rb_num, spectrum_numbers, custom_cal):
        """
        Focus some data using the current calibration.
        :param sample_paths: The paths to the data to be focused.
        :param banks: The banks that should be focused.
        :param plot_output: True if the output should be plotted.
        :param instrument: The instrument that the data came from.
        :param rb_num: The experiment number, used to create directories. Can be None
        :param spectrum_numbers: The specific spectra that should be focused. Used instead of banks.
        :param custom_cal: User defined calibration file to crop the focus to
        """
        full_calib_path = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                                      path_handling.ENGINEERING_PREFIX, "full_calibration")
        if not Ads.doesExist("full_inst_calib"):
            try:
                full_calib_workspace = Load(full_calib_path, OutputWorkspace="full_inst_calib")
            except RuntimeError:
                logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
                return
        else:
            full_calib_workspace = Ads.retrieve("full_inst_calib")
        if not Ads.doesExist(vanadium_corrections.INTEGRATED_WORKSPACE_NAME) and not Ads.doesExist(
                vanadium_corrections.CURVES_WORKSPACE_NAME):
            return
        integration_workspace = Ads.retrieve(vanadium_corrections.INTEGRATED_WORKSPACE_NAME)
        curves_workspace = Ads.retrieve(vanadium_corrections.CURVES_WORKSPACE_NAME)
        output_workspaces = []  # List of collated workspaces to plot.
        df_kwarg, name, region_calib = None, None, None
        if spectrum_numbers:
            inst_ws = path_handling.load_workspace(sample_paths[0])
            grp_ws = create_custom_grouping_workspace(spectrum_numbers, inst_ws)
            df_kwarg = {"GroupingWorkspace": grp_ws}
            region_calib = "engggui_calibration_Cropped"
            name = 'Cropped'
        elif custom_cal:
            # TODO this functionality has not yet been fully implemented
            df_kwarg = {"GroupingFileName": custom_cal}
            region_calib = "engggui_calibration_Custom"
            name = 'Custom'
        if df_kwarg:
            # check correct region calibration exists
            if not Ads.doesExist(region_calib):
                logger.warning(f"Cannot focus as the region calibration workspace \"{region_calib}\" is not "
                               f"present.")
                return
            for sample_path in sample_paths:
                sample_workspace = path_handling.load_workspace(sample_path)
                run_no = path_handling.get_run_number_from_path(sample_path, instrument)
                tof_output_name = str(run_no) + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + name
                dspacing_output_name = tof_output_name + "_dSpacing"
                # perform prefocus operations on whole instrument workspace
                prefocus_success = self._whole_inst_prefocus(sample_workspace, integration_workspace,
                                                             full_calib_workspace)
                if not prefocus_success:
                    continue
                # perform focus over chosen region of interest
                self._run_focus(sample_workspace, tof_output_name, curves_workspace, df_kwarg, region_calib)
                output_workspaces.append([tof_output_name])
                self._save_output(instrument, sample_path, "Cropped", tof_output_name, rb_num)
                self._save_output(instrument, sample_path, "Cropped", dspacing_output_name, rb_num, unit="dSpacing")
                self._output_sample_logs(instrument, run_no, sample_workspace, rb_num)
            # remove created grouping workspace if present
            if Ads.doesExist("grp_ws"):
                DeleteWorkspace("grp_ws")
        else:
            for sample_path in sample_paths:
                sample_workspace = path_handling.load_workspace(sample_path)
                run_no = path_handling.get_run_number_from_path(sample_path, instrument)
                workspaces_for_run = []
                # perform prefocus operations on whole instrument workspace
                prefocus_success = self._whole_inst_prefocus(sample_workspace, integration_workspace,
                                                             full_calib_workspace)
                if not prefocus_success:
                    continue
                # perform focus over chosen banks
                for name in banks:
                    tof_output_name = str(run_no) + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + str(name)
                    dspacing_output_name = tof_output_name + "_dSpacing"
                    if name == '1':
                        df_kwarg = {"GroupingFileName": NORTH_BANK_CAL}
                        region_calib = "engggui_calibration_bank_1"
                    else:
                        df_kwarg = {"GroupingFileName": SOUTH_BANK_CAL}
                        region_calib = "engggui_calibration_bank_2"
                    # check correct region calibration exists
                    if not Ads.doesExist(region_calib):
                        logger.warning(f"Cannot focus as the region calibration workspace \"{region_calib}\" is not "
                                       f"present.")
                        return
                    self._run_focus(sample_workspace, tof_output_name, curves_workspace, df_kwarg, region_calib)
                    workspaces_for_run.append(tof_output_name)
                    # Save the output to the file system.
                    self._save_output(instrument, sample_path, name, tof_output_name, rb_num)
                    self._save_output(instrument, sample_path, name, dspacing_output_name, rb_num, unit="dSpacing")
                output_workspaces.append(workspaces_for_run)
                self._output_sample_logs(instrument, run_no, sample_workspace, rb_num)
                DeleteWorkspace(sample_workspace)

        # Plot the output
        if plot_output:
            for ws_names in output_workspaces:
                self._plot_focused_workspaces(ws_names)

    @staticmethod
    def _whole_inst_prefocus(input_workspace,
                             vanadium_integration_ws,
                             full_calib) -> bool:
        """This is used to perform the operations done on the whole instrument workspace, before the chosen region of
        interest is focused using _run_focus
        :param input_workspace: Raw sample run to process prior to focussing over a region of interest
        :param vanadium_integration_ws: Integral of the supplied vanadium run
        :param full_calib: Full instrument calibration workspace (table ws output from PDCalibration)
        :return True if successful, False if aborted
        """
        if input_workspace.getRun().getProtonCharge() > 0:
            NormaliseByCurrent(InputWorkspace=input_workspace, OutputWorkspace=input_workspace)
        else:
            logger.warning(f"Skipping focus of run {input_workspace.name()} because it has invalid proton charge.")
            return False
        input_workspace /= vanadium_integration_ws
        # replace nans created in sensitivity correction
        ReplaceSpecialValues(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, NaNValue=0,
                             InfinityValue=0)
        ApplyDiffCal(InstrumentWorkspace=input_workspace, CalibrationWorkspace=full_calib)
        ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target='dSpacing')
        return True

    @staticmethod
    def _run_focus(input_workspace,
                   tof_output_name,
                   vanadium_curves_ws,
                   df_kwarg,
                   region_calib) -> None:
        """Focus the processed full instrument workspace over the chosen region of interest
        :param input_workspace: Processed full instrument workspace converted to dSpacing
        :param tof_output_name: Name for the time-of-flight output workspace
        :param vanadium_curves_ws: Workspace containing the vanadium curves
        :param df_kwarg: kwarg to pass to DiffractionFocussing specifying the region of interest
        :param region_calib: Region of interest calibration workspace (table ws output from PDCalibration)
        """
        # rename workspace prior to focussing to avoid errors later
        dspacing_output_name = tof_output_name + "_dSpacing"
        # focus over specified region of interest
        focused_sample = DiffractionFocussing(InputWorkspace=input_workspace, OutputWorkspace=dspacing_output_name,
                                              **df_kwarg)
        curves_rebinned = RebinToWorkspace(WorkspaceToRebin=vanadium_curves_ws, WorkspaceToMatch=focused_sample)
        Divide(LHSWorkspace=focused_sample, RHSWorkspace=curves_rebinned, OutputWorkspace=focused_sample,
               AllowDifferentNumberSpectra=True)
        # apply calibration from specified region of interest
        ApplyDiffCal(InstrumentWorkspace=focused_sample, CalibrationWorkspace=region_calib)
        # set bankid for use in fit tab
        run = focused_sample.getRun()
        if region_calib == "engggui_calibration_bank_1":
            run.addProperty("bankid", 1, True)
        elif region_calib == "engggui_calibration_bank_2":
            run.addProperty("bankid", 2, True)
        else:
            run.addProperty("bankid", 3, True)
        # output in both dSpacing and TOF
        ConvertUnits(InputWorkspace=focused_sample, OutputWorkspace=tof_output_name, Target='TOF')
        DeleteWorkspace(curves_rebinned)

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

    def _save_output(self, instrument, sample_path, bank, sample_workspace, rb_num, unit="TOF"):
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
                                                 rb_num, unit)
        self._save_focused_output_files_as_gss(instrument, sample_path, bank, sample_workspace,
                                               rb_num, unit)
        self._save_focused_output_files_as_topas_xye(instrument, sample_path, bank, sample_workspace,
                                                     rb_num, unit)
        output_path = path.join(path_handling.get_output_path(), 'Focus')
        logger.notice(f"\n\nFocus files saved to: \"{output_path}\"\n\n")
        if rb_num:
            output_path = path.join(path_handling.get_output_path(), 'User', rb_num, 'Focus')
            logger.notice(f"\n\nFocus files also saved to: \"{output_path}\"\n\n")
        self._last_path = output_path
        if self._last_path and self._last_path_ws:
            self._last_path = path.join(self._last_path, self._last_path_ws)

    def _save_focused_output_files_as_gss(self, instrument, sample_path, bank, sample_workspace,
                                          rb_num, unit):
        gss_output_path = path.join(
            path_handling.get_output_path(), "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, unit, ".gss"))
        SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)
        if rb_num:
            gss_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, unit, ".gss"))
            SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)

    def _save_focused_output_files_as_nexus(self, instrument, sample_path, bank, sample_workspace,
                                            rb_num, unit):
        file_name = self._generate_output_file_name(instrument, sample_path, bank, unit, ".nxs")
        nexus_output_path = path.join(path_handling.get_output_path(), "Focus", file_name)
        SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        if rb_num:
            nexus_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus", file_name)
            SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        self._last_path_ws = file_name

    def _save_focused_output_files_as_topas_xye(self, instrument, sample_path, bank,
                                                sample_workspace, rb_num, unit):
        xye_output_path = path.join(
            path_handling.get_output_path(), "Focus",
            self._generate_output_file_name(instrument, sample_path, bank, unit, ".abc"))
        SaveFocusedXYE(InputWorkspace=sample_workspace,
                       Filename=xye_output_path,
                       SplitFiles=False,
                       Format="TOPAS")
        if rb_num:
            xye_output_path = path.join(
                path_handling.get_output_path(), "User", rb_num, "Focus",
                self._generate_output_file_name(instrument, sample_path, bank, unit, ".abc"))
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
    def _generate_output_file_name(instrument, sample_path, bank, unit, suffix):
        run_no = path_handling.get_run_number_from_path(sample_path, instrument)
        return instrument + '_' + run_no + '_' + "bank_" + bank + '_' + unit + suffix
