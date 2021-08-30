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

from Engineering.common import path_handling
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from Engineering import EnggUtils
from mantid.simpleapi import logger, AnalysisDataService as Ads, SaveNexus, SaveGSS, SaveFocusedXYE, \
    Load, NormaliseByCurrent, Divide, DiffractionFocussing, RebinToWorkspace, DeleteWorkspace, ApplyDiffCal, \
    ConvertUnits, ReplaceSpecialValues, EnggEstimateFocussedBackground, AddSampleLog

SAMPLE_RUN_WORKSPACE_NAME = "engggui_focusing_input_ws"
FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"
REGION_CALIB_WS_PREFIX = "engggui_calibration_"
CURVES_PREFIX = "engggui_curves_"

NORTH_BANK_CAL = "EnginX_NorthBank.cal"
SOUTH_BANK_CAL = "EnginX_SouthBank.cal"


class FocusModel(object):

    def __init__(self):
        self._last_focused_files = []

    def get_last_focused_files(self):
        return self._last_focused_files

    def focus_run(self, sample_paths: list, vanadium_path: str, plot_output: bool, instrument: str, rb_num: str,
                  regions_dict: dict) -> None:
        """
        Focus some data using the current calibration.
        :param sample_paths: The paths to the data to be focused.
        :param vanadium_path: Path to the vanadium file from the current calibration
        :param plot_output: True if the output should be plotted.
        :param instrument: The instrument that the data came from.
        :param rb_num: Number to signify the user who is running this focus
        :param regions_dict: dict region name -> grp_ws_name, defining region(s) of interest to focus over
        """
        full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                      output_settings.ENGINEERING_PREFIX, "full_calibration")
        if not Ads.doesExist("full_inst_calib"):
            try:
                full_calib_workspace = Load(full_calib_path, OutputWorkspace="full_inst_calib")
            except RuntimeError:
                logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
                return
        else:
            full_calib_workspace = Ads.retrieve("full_inst_calib")
        van_integration_ws, van_processed_inst_ws = vanadium_corrections.fetch_correction_workspaces(vanadium_path, instrument)

        # check correct region calibration(s) and grouping workspace(s) exists
        inst_ws = path_handling.load_workspace(sample_paths[0])
        for region in regions_dict:
            calib_exists = self._check_region_calib_ws_exists(region)
            grouping_exists = self._check_region_grouping_ws_exists(regions_dict[region], inst_ws)
            if not (calib_exists and grouping_exists):
                return

        # loop over samples provided, focus each over region(s) specified in regions_dict
        output_workspaces = []  # List of collated workspaces to plot.
        self._last_focused_files = []
        van_run_no = path_handling.get_run_number_from_path(vanadium_path, instrument)
        for sample_path in sample_paths:
            sample_workspace = path_handling.load_workspace(sample_path)
            run_no = path_handling.get_run_number_from_path(sample_path, instrument)
            # perform prefocus operations on whole instrument workspace
            prefocus_success = self._whole_inst_prefocus(sample_workspace, van_integration_ws,
                                                         full_calib_workspace)
            if not prefocus_success:
                continue
            sample_plots = []  # if both banks focused, pass list with both so plotted on same figure
            for region, grouping_kwarg in regions_dict.items():
                tof_output_name = str(run_no) + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + region
                dspacing_output_name = tof_output_name + "_dSpacing"
                region_calib_ws = self._get_region_calib_ws(region)
                curves = self._get_van_curves_for_roi(region, van_processed_inst_ws, grouping_kwarg)
                # perform focus over chosen region of interest
                self._run_focus(sample_workspace, tof_output_name, curves, grouping_kwarg,
                                region_calib_ws)
                sample_plots.append(tof_output_name)
                self._save_output(instrument, run_no, van_run_no, region, tof_output_name, rb_num)
                self._save_output(instrument, run_no, van_run_no, region, dspacing_output_name, rb_num, unit="dSpacing")
                self._output_sample_logs(instrument, run_no, van_run_no, sample_workspace, rb_num)
            output_workspaces.append(sample_plots)
            DeleteWorkspace(sample_workspace)
        # remove created grouping workspace if present
        if Ads.doesExist("grp_ws"):
            DeleteWorkspace("grp_ws")
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
                   curves,
                   grouping_ws,
                   region_calib) -> None:
        """Focus the processed full instrument workspace over the chosen region of interest
        :param input_workspace: Processed full instrument workspace converted to dSpacing
        :param tof_output_name: Name for the time-of-flight output workspace
        :param curves: Workspace containing the vanadium curves for this region of interest
        :param grouping_ws: Grouping workspace to pass to DiffractionFocussing
        :param region_calib: Region of interest calibration workspace (table ws output from PDCalibration)
        """
        # rename workspace prior to focussing to avoid errors later
        dspacing_output_name = tof_output_name + "_dSpacing"
        # focus sample over specified region of interest
        focused_sample = DiffractionFocussing(InputWorkspace=input_workspace, OutputWorkspace=dspacing_output_name,
                                              GroupingWorkspace=grouping_ws)
        curves_rebinned = RebinToWorkspace(WorkspaceToRebin=curves, WorkspaceToMatch=focused_sample)
        # flux correction - divide focused sample data by rebinned focused vanadium curve data
        Divide(LHSWorkspace=focused_sample, RHSWorkspace=curves_rebinned, OutputWorkspace=focused_sample,
               AllowDifferentNumberSpectra=True)
        # apply calibration from specified region of interest
        ApplyDiffCal(InstrumentWorkspace=focused_sample, CalibrationWorkspace=region_calib)
        # set bankid for use in fit tab
        run = focused_sample.getRun()
        if region_calib.name() == "engggui_calibration_bank_1":
            run.addProperty("bankid", 1, True)
        elif region_calib.name() == "engggui_calibration_bank_2":
            run.addProperty("bankid", 2, True)
        else:
            run.addProperty("bankid", 3, True)
        # output in both dSpacing and TOF
        ConvertUnits(InputWorkspace=focused_sample, OutputWorkspace=tof_output_name, Target='TOF')
        DeleteWorkspace(curves_rebinned)

    @staticmethod
    def _get_van_curves_for_roi(region: str, van_processed_inst_ws, grouping_ws):  # -> Workspace
        """
        Retrieve vanadium curves for this roi from the ADS if they exist, create them if not
        :param region: String describing region of interest
        :param van_processed_inst_ws: Processed instrument workspace of this vanadium run
        :param grouping_ws: Grouping workspace for DiffractionFoucussing
        :return: Curves workspace for this roi
        """
        curves_roi_name = CURVES_PREFIX + region
        # check if van curves for roi in ADS (should exist if not first run in session)
        if Ads.doesExist(curves_roi_name):
            return Ads.retrieve(curves_roi_name)
        else:
            # focus processed instrument ws over specified region of interest, iot produce vanadium curves for roi
            focused_curves = DiffractionFocussing(InputWorkspace=van_processed_inst_ws, OutputWorkspace=curves_roi_name,
                                                  GroupingWorkspace=grouping_ws)
            EnggEstimateFocussedBackground(InputWorkspace=focused_curves,
                                           OutputWorkspace=focused_curves,
                                           NIterations='15', XWindow=0.03)
            return focused_curves

    @staticmethod
    def _get_region_calib_ws(region: str):  # -> Workspace
        """
        Retrieve region calibration workspace from the ADS
        :param region: String describing region of interest
        :return: Region calibration workspace
        """
        ws_name = REGION_CALIB_WS_PREFIX + region
        return Ads.retrieve(ws_name)

    @staticmethod
    def _check_region_grouping_ws_exists(grouping_ws_name: str, inst_ws) -> bool:
        """
        Check that the required grouping workspace for this focus exists, and if not present for a North/South bank
        focus, retrieve them from the user directories or create them (expected if first focus with loaded calibration)
        :param grouping_ws_name: Name of the grouping workspace whose presence in the ADS is being checked
        :param inst_ws: Workspace containing the instrument data for use in making a bank grouping workspace
        :return: True if the required workspace exists (or has just been loaded/created), False if not
        """
        if not Ads.doesExist(grouping_ws_name):
            if "North" in grouping_ws_name:
                logger.notice("NorthBank grouping workspace not present in ADS, loading")
                EnggUtils.get_bank_grouping_workspace(1, inst_ws)
                return True
            elif "South" in grouping_ws_name:
                logger.notice("SouthBank grouping workspace not present in ADS, loading")
                EnggUtils.get_bank_grouping_workspace(2, inst_ws)
                return True
            else:
                logger.warning(f"Cannot focus as the grouping workspace \"{grouping_ws_name}\" is not present.")
                return False
        return True

    @staticmethod
    def _check_region_calib_ws_exists(region: str) -> bool:
        """
        Check that the required workspace for use in focussing the provided region of interest exist in the ADS
        :param region: String describing region of interest
        :return: True if present, False if not
        """
        region_ws_name = REGION_CALIB_WS_PREFIX + region
        present = Ads.doesExist(region_ws_name)
        if not present:
            logger.warning(f"Cannot focus as the region calibration workspace \"{region_ws_name}\" is not "
                           f"present.")
        return present

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

    def _save_output(self, instrument, sample_run, van_run, bank, sample_workspace, rb_num, unit="TOF"):
        """
        Save a focused workspace to the file system. Saves separate copies to a User directory if an rb number has
        been set.
        :param instrument: The instrument the data is from.
        :param sample_run: The sample run number that was focussed
        :param bank: The name of the bank being saved.
        :param sample_workspace: The name of the workspace to be saved.
        :param rb_num: Usually an experiment id, defines the name of the user directory.
        """
        self._save_focused_output_files_as_nexus(instrument, sample_run, van_run, bank, sample_workspace,
                                                 rb_num, unit)
        self._save_focused_output_files_as_gss(instrument, sample_run, van_run, bank, sample_workspace,
                                               rb_num, unit)
        self._save_focused_output_files_as_topas_xye(instrument, sample_run, van_run, bank, sample_workspace,
                                                     rb_num, unit)
        output_path = path.join(output_settings.get_output_path(), 'Focus')
        logger.notice(f"\n\nFocus files saved to: \"{output_path}\"\n\n")
        if rb_num:
            output_path = path.join(output_settings.get_output_path(), 'User', rb_num, 'Focus')
            logger.notice(f"\n\nFocus files also saved to: \"{output_path}\"\n\n")

    def _save_focused_output_files_as_gss(self, instrument, sample_run, van_run, bank, sample_workspace,
                                          rb_num, unit):
        gss_output_path = path.join(output_settings.get_output_path(), "Focus",
                                    self._generate_output_file_name(instrument, sample_run, van_run, bank, unit, ".gss"))
        SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)
        if rb_num:
            gss_output_path = path.join(output_settings.get_output_path(), "User", rb_num, "Focus",
                                        self._generate_output_file_name(instrument, sample_run, van_run, bank, unit, ".gss"))
            SaveGSS(InputWorkspace=sample_workspace, Filename=gss_output_path)

    def _save_focused_output_files_as_nexus(self, instrument, sample_run, van_run, bank, sample_workspace,
                                            rb_num, unit):
        file_name = self._generate_output_file_name(instrument, sample_run, van_run, bank, unit, ".nxs")
        nexus_output_path = path.join(output_settings.get_output_path(), "Focus", file_name)
        AddSampleLog(Workspace=sample_workspace, LogName="Vanadium Run", LogText=van_run)
        SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        if rb_num:
            nexus_output_path = path.join(output_settings.get_output_path(), "User", rb_num, "Focus", file_name)
            SaveNexus(InputWorkspace=sample_workspace, Filename=nexus_output_path)
        if unit == "TOF":
            self._last_focused_files.append(nexus_output_path)

    def _save_focused_output_files_as_topas_xye(self, instrument, sample_run, van_run, bank,
                                                sample_workspace, rb_num, unit):
        xye_output_path = path.join(output_settings.get_output_path(), "Focus",
                                    self._generate_output_file_name(instrument, sample_run, van_run, bank, unit, ".abc"))
        SaveFocusedXYE(InputWorkspace=sample_workspace,
                       Filename=xye_output_path,
                       SplitFiles=False,
                       Format="TOPAS")
        if rb_num:
            xye_output_path = path.join(output_settings.get_output_path(), "User", rb_num, "Focus",
                                        self._generate_output_file_name(instrument, sample_run, van_run, bank, unit, ".abc"))
            SaveFocusedXYE(InputWorkspace=sample_workspace,
                           Filename=xye_output_path,
                           SplitFiles=False,
                           Format="TOPAS")

    @staticmethod
    def _output_sample_logs(instrument, run_number, van_run_number, workspace, rb_num):
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
        focus_dir = path.join(output_settings.get_output_path(), "Focus")
        if not path.exists(focus_dir):
            makedirs(focus_dir)
        output_path = path.join(focus_dir, (instrument + "_" + run_number + "_" + van_run_number + "_sample_logs.csv"))
        write_to_file()
        if rb_num:
            focus_user_dir = path.join(output_settings.get_output_path(), "User", rb_num, "Focus")
            if not path.exists(focus_user_dir):
                makedirs(focus_user_dir)
            output_path = path.join(focus_user_dir, (instrument + "_" + run_number + "_" + van_run_number + "_sample_logs.csv"))
            write_to_file()

    @staticmethod
    def _generate_output_file_name(instrument, run_no, van_run_no, bank, unit, suffix):
        return instrument + '_' + run_no + '_' + van_run_no +'_' + bank + '_' + unit + suffix
