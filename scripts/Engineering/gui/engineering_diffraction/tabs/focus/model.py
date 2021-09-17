# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import csv
from os import path, makedirs
import matplotlib.pyplot as plt
from shutil import copy2

from Engineering.common import path_handling
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantid.simpleapi import logger, AnalysisDataService as Ads, SaveNexus, SaveGSS, SaveFocusedXYE, \
    Load, NormaliseByCurrent, Divide, DiffractionFocussing, RebinToWorkspace, DeleteWorkspace, ApplyDiffCal, \
    ConvertUnits, ReplaceSpecialValues, EnggEstimateFocussedBackground, AddSampleLog, ExtractSingleSpectrum

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

    def focus_run(self, sample_paths: list, vanadium_path: str, plot_output: bool, rb_num: str,
                  calibration: CalibrationInfo) -> None:
        """
        Focus some data using the current calibration.
        :param sample_paths: The paths to the data to be focused.
        :param vanadium_path: Path to the vanadium file from the current calibration
        :param plot_output: True if the output should be plotted.
        :param instrument: The instrument that the data came from.
        :param rb_num: Number to signify the user who is running this focus
        :param regions_dict: dict region name -> grp_ws_name, defining region(s) of interest to focus over
        """

        # check correct region calibration(s) and grouping workspace(s) exists
        if not calibration.is_valid():
            return

        # check if full instrument calibration exists, if not load it
        if not Ads.doesExist("full_inst_calib"):
            try:
                full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                              output_settings.ENGINEERING_PREFIX, "full_calibration")
                full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
            except RuntimeError:
                logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
                return
        else:
            full_calib = Ads.retrieve("full_inst_calib")

        # 1) load, focus and process vanadium
        ws_van = self._load_run_and_convert_to_dSpacing(vanadium_path, calibration.get_instrument(), full_calib)
        van_foc_name = CURVES_PREFIX + calibration.get_group_suffix()
        ws_van_foc = self._focus_run_and_apply_roi_calibration(ws_van, calibration, ws_foc_name=van_foc_name,
                                                               applyCal=False)
        ws_van_foc = self._smooth_vanadium(ws_van_foc)

        # 2) Loop over runs
        van_run = path_handling.get_run_number_from_path(vanadium_path, calibration.get_instrument())
        output_workspaces = []  # List of collated workspaces to plot.
        for sample_path in sample_paths:
            ws_sample = self._load_run_and_convert_to_dSpacing(sample_path, calibration.get_instrument(), full_calib)
            ws_foc = self._focus_run_and_apply_roi_calibration(ws_sample, calibration, applyCal=True)
            ws_foc = self._apply_vanadium_norm(ws_foc, ws_van_foc)
            self._save_output_files(ws_foc, calibration, van_run, rb_num)
            output_workspaces.append(ws_foc.name())

        # Plot the output
        if plot_output:
            self._plot_focused_workspaces(output_workspaces)

    @staticmethod
    def _plot_focused_workspaces(ws_names):
        for ws_name in ws_names:
            ws_foc = Ads.retrieve(ws_name)
            fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
            for ispec in range(ws_foc.getNumberHistograms()):
                ax.plot(ws_foc, color='#1f77b4', label='193749_foc: spec 1', marker='.', wkspIndex=ispec)
            ax.legend()
            fig.show()

    @staticmethod
    def _load_run_and_convert_to_dSpacing(filepath, instrument, full_calib):
        runno = path_handling.get_run_number_from_path(filepath, instrument)
        ws = Load(Filename=filepath, OutputWorkspace=str(runno))
        if ws.getRun().getProtonCharge() > 0:
            ws = NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws.name())
        else:
            logger.warning(f"Skipping focus of run {ws.name()} because it has invalid proton charge.")
            return None
        ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib)
        ws = ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target='dSpacing')
        return ws

    @staticmethod
    def _focus_run_and_apply_roi_calibration(ws, calibration, ws_foc_name = None, applyCal = True):
        if not ws_foc_name:
            ws_foc_name = ws.name() + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + calibration.get_foc_ws_suffix()
        ws_foc = DiffractionFocussing(InputWorkspace=ws, OutputWorkspace=ws_foc_name,
                                      GroupingWorkspace=calibration.get_group_ws())
        if applyCal:
            ApplyDiffCal(InstrumentWorkspace=ws_foc, CalibrationWorkspace=calibration.get_calibration_table())
        return ws_foc

    @staticmethod
    def _smooth_vanadium(van_ws_foc):
        return EnggEstimateFocussedBackground(InputWorkspace=van_ws_foc, OutputWorkspace=van_ws_foc,
                                              NIterations=1, XWindow=0.08)

    @staticmethod
    def _apply_vanadium_norm(sample_ws_foc, van_ws_foc):
        # divide by curves - automatically corrects for solid angle, det efficiency and lambda dep. flux
        van_ws_foc = RebinToWorkspace(WorkspaceToRebin=van_ws_foc, WorkspaceToMatch=sample_ws_foc,
                                      OutputWorkspace=van_ws_foc)  # do this in-place
        sample_ws_foc = Divide(LHSWorkspace=sample_ws_foc, RHSWorkspace=van_ws_foc,
                               OutputWorkspace=sample_ws_foc.name(), AllowDifferentNumberSpectra=False)
        sample_ws_foc = ReplaceSpecialValues(InputWorkspace=sample_ws_foc, OutputWorkspace=sample_ws_foc.name(),
                                             NaNValue=0, NaNError=0.0, InfinityValue=0, InfinityError=0.0)
        # convert to TOF after this
        return sample_ws_foc

    def _save_output_files(self, sample_ws_foc, calibration, van_run, rb_num = None):
        focus_dir = path.join(output_settings.get_output_path(), "Focus")
        # set bankid for use in fit tab
        foc_suffix = calibration.get_foc_ws_suffix()
        # if nspec = 1 - just use suffix
        for ispec in range(sample_ws_foc.getNumberHistograms()):
            ws_spec = ExtractSingleSpectrum(InputWorkspace=sample_ws_foc, WorkspaceIndex=ispec)
            # add a bankid and vanadium to log that is read by fitting model
            bankid = foc_suffix if sample_ws_foc.getNumberHistograms() == 1 else foc_suffix + ' ' + str(ispec+1)
            AddSampleLog(Workspace=ws_spec, LogName="bankid", LogText=bankid)
            AddSampleLog(Workspace=ws_spec, LogName="Vanadium Run", LogText=van_run)
            ws_spec.getRun().addProperty("bankid", bankid, True) # overwrites previous if exists
            # save spectrum as nexus, gss and XYE
            filename = self._generate_output_file_name(calibration, van_run, bankid, ext="")
            SaveNexus(InputWorkspace=ws_spec, Filename=path.join(focus_dir, filename + ".nxs"))
            SaveGSS(InputWorkspace=ws_spec, Filename=path.join(focus_dir, filename + ".gss"))
            SaveFocusedXYE(InputWorkspace=ws_spec, Filename=path.join(focus_dir, filename + ".abc"),
                           SplitFiles=False, Format="TOPAS")
            # copy file to rb folder if present
            if rb_num:
                rb_focus_dir = path.join(output_settings.get_output_path(), "User", rb_num, "Focus")
                for ext in [".nxs", ".gss", ".abc"]:
                    copy2(path.join(focus_dir, filename + ext),
                          path.join(rb_focus_dir, filename + ext))
        DeleteWorkspace(ws_spec.name())

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
    def _generate_output_file_name(calibration, van_run_no, suffix, ext=""):
        return "_".join([calibration.get_instrument(),  calibration.get_sample_runno(), van_run_no,
                         suffix]) + ext
