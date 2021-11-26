# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs
import matplotlib.pyplot as plt

from Engineering.common import path_handling
from Engineering.EnggUtils import GROUP
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model import \
    load_full_instrument_calibration
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantid.simpleapi import logger, AnalysisDataService as Ads, SaveNexus, SaveGSS, SaveFocusedXYE, \
    Load, NormaliseByCurrent, Divide, DiffractionFocussing, RebinToWorkspace, DeleteWorkspace, ApplyDiffCal, \
    ConvertUnits, ReplaceSpecialValues, EnggEstimateFocussedBackground, AddSampleLog, CropWorkspace

FOCUSED_OUTPUT_WORKSPACE_NAME = "engggui_focusing_output_ws_"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"
CURVES_PREFIX = "engggui_curves_"
VAN_CURVE_REBINNED_NAME = "van_ws_foc_rb"

XUNIT_SUFFIXES = {'d-Spacing': 'dSpacing', 'Time-of-flight': 'TOF'}  # to put in saved focused data filename


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
        :param rb_num: Number to signify the user who is running this focus
        :param calibration: CalibrationInfo object that holds all info needed about ROI and instrument
        """

        # check correct region calibration(s) and grouping workspace(s) exists
        if not calibration.is_valid():
            return

        # check if full instrument calibration exists, if not load it
        full_calib = load_full_instrument_calibration()
        # load, focus and process vanadium (retrieve from ADS if exists)
        ws_van_foc, van_run = self.process_vanadium(vanadium_path, calibration, full_calib)

        # directories for saved focused data
        focus_dirs = [path.join(output_settings.get_output_path(), "Focus")]
        if rb_num:
            focus_dirs.append(path.join(output_settings.get_output_path(), "User", rb_num, "Focus"))
            if calibration.group == GROUP.TEXTURE:
                focus_dirs.pop(0)  # only save to RB directory to limit number files saved

        # Loop over runs and focus
        output_workspaces = []  # List of focused workspaces to plot.
        for sample_path in sample_paths:
            ws_sample = self._load_run_and_convert_to_dSpacing(sample_path, calibration.get_instrument(), full_calib)
            if ws_sample:
                # None returned if no proton charge
                ws_foc = self._focus_run_and_apply_roi_calibration(ws_sample, calibration)
                ws_foc = self._apply_vanadium_norm(ws_foc, ws_van_foc)
                self._save_output_files(focus_dirs, ws_foc, calibration, van_run, rb_num)
                # convert units to TOF and save again
                ws_foc = ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target='TOF')
                self._save_output_files(focus_dirs, ws_foc, calibration, van_run, rb_num)
                output_workspaces.append(ws_foc.name())

        # Plot the output
        if output_workspaces:
            DeleteWorkspace(VAN_CURVE_REBINNED_NAME)
            if plot_output:
                self._plot_focused_workspaces(output_workspaces)

    def process_vanadium(self, vanadium_path, calibration, full_calib):
        van_run = path_handling.get_run_number_from_path(vanadium_path, calibration.get_instrument())
        van_foc_name = CURVES_PREFIX + calibration.get_group_suffix()
        if Ads.doesExist(van_foc_name):
            ws_van_foc = Ads.retrieve(van_foc_name)
        else:
            if Ads.doesExist(van_run):
                ws_van = Ads.retrieve(van_run)  # will exist if have only changed the ROI
            else:
                ws_van = self._load_run_and_convert_to_dSpacing(vanadium_path, calibration.get_instrument(), full_calib)
                if not ws_van:
                    raise RuntimeError(f"vanadium run {van_run} has no proton_charge - "
                                       f"please supply a valid vanadium run to focus.")
            ws_van_foc = self._focus_run_and_apply_roi_calibration(ws_van, calibration, ws_foc_name=van_foc_name)
            ws_van_foc = self._smooth_vanadium(ws_van_foc)
        return ws_van_foc, van_run

    @staticmethod
    def _plot_focused_workspaces(ws_names):
        for ws_name in ws_names:
            ws_foc = Ads.retrieve(ws_name)
            ws_label = '_'.join([ws_foc.getInstrument().getName(), ws_foc.run().get('run_number').value])
            fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
            for ispec in range(ws_foc.getNumberHistograms()):
                ax.plot(ws_foc, label=f'{ws_label} focused: spec {ispec+1}', marker='.', wkspIndex=ispec)
            ax.legend()
            fig.show()

    @staticmethod
    def _load_run_and_convert_to_dSpacing(filepath, instrument, full_calib):
        runno = path_handling.get_run_number_from_path(filepath, instrument)
        ws = Load(Filename=filepath, OutputWorkspace=str(runno))
        if ws.getRun().getProtonCharge() > 0:
            ws = NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws.name())
        else:
            logger.warning(f"Run {ws.name()} has invalid proton charge.")
            DeleteWorkspace(ws)
            return None
        ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib)
        ws = ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target='dSpacing')
        return ws

    @staticmethod
    def _focus_run_and_apply_roi_calibration(ws, calibration, ws_foc_name=None):
        if not ws_foc_name:
            ws_foc_name = ws.name() + "_" + FOCUSED_OUTPUT_WORKSPACE_NAME + calibration.get_foc_ws_suffix()
        ws_foc = DiffractionFocussing(InputWorkspace=ws, OutputWorkspace=ws_foc_name,
                                      GroupingWorkspace=calibration.get_group_ws())
        ApplyDiffCal(InstrumentWorkspace=ws_foc, ClearCalibration=True)
        ws_foc = ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target='TOF')
        ApplyDiffCal(InstrumentWorkspace=ws_foc, CalibrationWorkspace=calibration.get_calibration_table())
        ws_foc = ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target='dSpacing')
        return ws_foc

    @staticmethod
    def _smooth_vanadium(van_ws_foc):
        return EnggEstimateFocussedBackground(InputWorkspace=van_ws_foc, OutputWorkspace=van_ws_foc,
                                              NIterations=1, XWindow=0.08)

    @staticmethod
    def _apply_vanadium_norm(sample_ws_foc, van_ws_foc):
        # divide by curves - automatically corrects for solid angle, det efficiency and lambda dep. flux
        sample_ws_foc = CropWorkspace(InputWorkspace=sample_ws_foc, OutputWorkspace=sample_ws_foc.name(), XMin=0.45)
        van_ws_foc_rb = RebinToWorkspace(WorkspaceToRebin=van_ws_foc, WorkspaceToMatch=sample_ws_foc,
                                         OutputWorkspace=VAN_CURVE_REBINNED_NAME)  # copy so as not to lose data
        sample_ws_foc = Divide(LHSWorkspace=sample_ws_foc, RHSWorkspace=van_ws_foc_rb,
                               OutputWorkspace=sample_ws_foc.name(), AllowDifferentNumberSpectra=False)
        sample_ws_foc = ReplaceSpecialValues(InputWorkspace=sample_ws_foc, OutputWorkspace=sample_ws_foc.name(),
                                             NaNValue=0, NaNError=0.0, InfinityValue=0, InfinityError=0.0)
        return sample_ws_foc

    def _save_output_files(self, focus_dirs, sample_ws_foc, calibration, van_run, rb_num=None):

        # set bankid for use in fit tab
        foc_suffix = calibration.get_foc_ws_suffix()
        xunit = sample_ws_foc.getDimension(0).name
        xunit_suffix = XUNIT_SUFFIXES[xunit]
        sample_run_no = sample_ws_foc.run().get('run_number').value
        # save all spectra to single ASCII files
        ascii_fname = self._generate_output_file_name(calibration.get_instrument(), sample_run_no, van_run,
                                                      calibration.get_group_suffix(), xunit_suffix, ext='')
        for focus_dir in focus_dirs:
            if not path.exists(focus_dir):
                makedirs(focus_dir)
            SaveGSS(InputWorkspace=sample_ws_foc, Filename=path.join(focus_dir, ascii_fname + '.gss'), SplitFiles=False,
                    UseSpectrumNumberAsBankID=True)
            SaveFocusedXYE(InputWorkspace=sample_ws_foc, Filename=path.join(focus_dir, ascii_fname + ".abc"),
                           SplitFiles=False, Format="TOPAS")
            # Save nxs per spectrum
            AddSampleLog(Workspace=sample_ws_foc, LogName="Vanadium Run", LogText=van_run)
            for ispec in range(sample_ws_foc.getNumberHistograms()):
                # add a bankid and vanadium to log that is read by fitting model
                bankid = foc_suffix if sample_ws_foc.getNumberHistograms() == 1 else f'{foc_suffix}_{ispec+1}'
                AddSampleLog(Workspace=sample_ws_foc, LogName="bankid", LogText=bankid.replace('_', ' '))  # overwrites
                # save spectrum as nexus
                filename = self._generate_output_file_name(calibration.get_instrument(), sample_run_no, van_run, bankid,
                                                           xunit_suffix, ext=".nxs")
                nxs_path = path.join(focus_dir, filename)
                SaveNexus(InputWorkspace=sample_ws_foc, Filename=nxs_path, WorkspaceIndexList=[ispec])
                if xunit == "Time-of-flight":
                    self._last_focused_files.append(nxs_path)

    @staticmethod
    def _generate_output_file_name(inst, sample_run_no, van_run_no, suffix, xunit, ext=""):
        return "_".join([inst,  sample_run_no, van_run_no, suffix, xunit]) + ext
