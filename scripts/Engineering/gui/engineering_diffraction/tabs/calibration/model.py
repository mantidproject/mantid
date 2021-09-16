# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs
from shutil import copy2
from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger, UnitParams
from mantid.simpleapi import PDCalibration, DeleteWorkspace, DiffractionFocussing, \
    CreateEmptyTableWorkspace, NormaliseByCurrent, ConvertUnits, Load, SaveNexus, ApplyDiffCal
import Engineering.EnggUtils as EnggUtils
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.common import path_handling

CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_South_bank.prm"


class CalibrationModel(object):
    def create_new_calibration(self, calibration, rb_num, plot_output):
        """
        Create a new calibration from a ceria run
        :param ceria_path: Path to ceria (CeO2) data file
        :param rb_num: The RB number for file creation.
        """
        ceria_path = calibration.get_sample_path()

        # load ceria data
        ceria_workspace = path_handling.load_workspace(ceria_path)

        # load whole instrument calibration
        full_calib = self.load_full_instrument_calibration()

        # run PDCalibration
        focused_ceria, cal_table, diag_ws, mask = self.run_calibration(ceria_workspace, calibration, full_calib)

        # extract diffractometer constants
        diff_consts = self.extract_diff_consts_from_ws(focused_ceria, mask)
        difa = [diffs[UnitParams.difa] for diffs in diff_consts]
        difc = [diffs[UnitParams.difc] for diffs in diff_consts]
        tzero = [diffs[UnitParams.tzero] for diffs in diff_consts]
        # write to table
        rows = []
        for i in range(len(difc)):
            rows.append([i, difc[i], difa[i], tzero[i]])
        self.update_calibration_params_table(rows)

        if plot_output:
            EnggUtils.plot_tof_vs_d_from_calibration(diag_ws, focused_ceria)

        # extract Back-to-Back Exponential params for .prm before deleting raw data
        bk2bk_params = self.extract_b2b_params(ceria_workspace)
        DeleteWorkspace(ceria_workspace)

        # save output
        if rb_num:
            calib_dir = path.join(output_settings.get_output_path(), "User", rb_num, "Calibration", "")
        else:
            calib_dir = path.join(output_settings.get_output_path(), "Calibration", "")

        # get diff consts from table?
        self.create_output_files(calib_dir, difa, difc, tzero, bk2bk_params, calibration)

    @staticmethod
    def extract_b2b_params(workspace):

        ws_inst = workspace.getInstrument()
        NorthBank = ws_inst.getComponentByName("NorthBank")
        SouthBank = ws_inst.getComponentByName("SouthBank")
        params_north = []
        params_south = []
        for param_name in ["alpha_0", "beta_0", "beta_1", "sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]:
            params_north += [NorthBank.getNumberParameter(param_name)[0]]
            params_south += [SouthBank.getNumberParameter(param_name)[0]]

        return [params_north, params_south]

    def load_existing_calibration_files(self, calibration):
        self.load_full_instrument_calibration()
        # load prm
        prm_filepath = calibration.prm_filepath
        if not path.exists(prm_filepath):
            msg = f"Could not open GSAS calibration file: {prm_filepath}"
            logger.warning(msg)
            return
        try:
            # read diff constants from prm
            diff_consts = self.read_diff_constants_from_prm(prm_filepath)
            self.update_calibration_params_table(diff_consts)
        except RuntimeError:
            logger.error(f"Invalid file selected: {prm_filepath}")
            return
        # how about grouping ws if made?
        calibration.load_relevant_calibration_files()

    @staticmethod
    def update_calibration_params_table(params_table):
        if len(params_table) == 0:
            return

        # Create blank, or clear rows from existing, params table.
        if Ads.doesExist(CALIB_PARAMS_WORKSPACE_NAME):
            workspace = Ads.retrieve(CALIB_PARAMS_WORKSPACE_NAME)
            workspace.setRowCount(0)
        else:
            workspace = CreateEmptyTableWorkspace(OutputWorkspace=CALIB_PARAMS_WORKSPACE_NAME)
            workspace.addColumn("int", "bankid")
            workspace.addColumn("double", "difc")
            workspace.addColumn("double", "difa")
            workspace.addColumn("double", "tzero")

        for row in params_table:
            workspace.addRow(row)

    @staticmethod
    def run_calibration(ceria_ws, calibration, full_instrument_cal_ws):
        """
        Creates Engineering calibration files with PDCalibration
        :param ceria_ws: The workspace with the ceria data.
        :param bank: The bank to crop to, both if none.
        :param calfile: The custom calibration file to crop to, not used if none.
        :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
        :return: dict containing calibrated diffractometer constants, and copy of the raw ceria workspace
        """

        # initial process of ceria ws
        NormaliseByCurrent(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws)
        ApplyDiffCal(InstrumentWorkspace=ceria_ws, CalibrationWorkspace=full_instrument_cal_ws)
        ConvertUnits(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws, Target='dSpacing')

        # get grouping workspace and focus
        grp_ws = calibration.get_group_ws(ceria_ws)  # (creates if doesn't exist)
        focused_ceria = DiffractionFocussing(InputWorkspace=ceria_ws, GroupingWorkspace=grp_ws)
        ApplyDiffCal(InstrumentWorkspace=focused_ceria, ClearCalibration=True)  # DIFC of detector in middle of bank
        focused_ceria = ConvertUnits(InputWorkspace=focused_ceria, Target='TOF')

        # Run PDCalibration to fit peaks in TOF
        foc_name = focused_ceria.name()  # PDCal invalidates ptr during rebin so keep track of ws name
        cal_table_name = "engggui_calibration_" + calibration.get_group_suffix()
        diag_ws_name = "diag_" + calibration.get_group_suffix()
        cal_table, diag_ws, mask = PDCalibration(InputWorkspace=foc_name, OutputCalibrationTable=cal_table_name,
                                                 DiagnosticWorkspaces=diag_ws_name,
                                                 PeakPositions=EnggUtils.default_ceria_expected_peaks(final=True),
                                                 TofBinning=[15500, -0.0003, 52000],
                                                 PeakWindow=0.04,
                                                 MinimumPeakHeight=0.5,
                                                 PeakFunction='BackToBackExponential',
                                                 CalibrationParameters='DIFC+TZERO+DIFA',
                                                 UseChiSq=True)
        ApplyDiffCal(InstrumentWorkspace=foc_name, CalibrationWorkspace=cal_table)
        # store cal_table in calibration
        calibration.set_calibration_table(cal_table)
        return Ads.retrieve(foc_name), cal_table, diag_ws, mask

    def load_full_instrument_calibration(self):
        if Ads.doesExist("full_inst_calib"):
            full_calib = Ads.retrieve("full_inst_calib")
        else:
            full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                          output_settings.ENGINEERING_PREFIX, "full_calibration")
            try:
                full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
            except ValueError:
                logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
                return
        return full_calib

    def extract_diff_consts_from_ws(self, ws_foc, mask_ws):
        si = ws_foc.spectrumInfo()
        masked_detIDs = mask_ws.getMaskedDetectors()
        diff_consts = []
        for ispec in range(ws_foc.getNumberHistograms()):
            if ws_foc.getSpectrum(ispec).getDetectorIDs()[0] in masked_detIDs:
                # spectrum has detectors that were masked in output of PDCal
                logger.warning(f'PDCalibration failed for spectrum index {ispec} - proceeding with uncalibrated DIFC.')
            diff_consts.append(si.diffractometerConstants(ispec))
        return diff_consts

    def create_output_files(self, calibration_dir, difa, difc, tzero, bk2bk_params, calibration):
        """
        Create output files from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param difa: DIFA values from calibration algorithm.
        :param difc: DIFC values from the calibration algorithm.
        :param tzero: TZERO values from the calibration algorithm.
        :param bk2bk_params: BackToBackExponential parameters from Parameters.xml file.
        :param CalibrationInfo object with details of calibration and grouping
        """
        ceria_run = calibration.get_sample_runno()

        # create calibration dir of not exiust
        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        # save grouping ws if custom or cropped
        if not calibration.group.banks:
            calibration.save_grouping_workspace(calibration_dir)

        # save prm file(s)
        prm_filepath = calibration_dir + calibration.generate_output_file_name()
        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                    "last_calibration_path", prm_filepath)
        EnggUtils.write_ENGINX_GSAS_iparam_file(prm_filepath, difa, difc, tzero, bk2bk_params,
                                                bank_names=calibration.group.banks,
                                                template_file=calibration.get_prm_template_file(),
                                                ceria_run=ceria_run)

        # save pdcal output as nexus
        filepath, ext = path.splitext(prm_filepath)
        nxs_filepath = filepath + '.nxs'
        SaveNexus(InputWorkspace=calibration.get_calibration_table(), Filename=nxs_filepath)
        if calibration.group == EnggUtils.GROUP.BOTH:
            # output a separate prm for North and South when both banks included
            for ibank, bank in enumerate(calibration.group.banks):
                bank_group = EnggUtils.GroupingInfo(EnggUtils.GROUP(str(ibank + 1)))  # to get prm template for the bank
                prm_filepath_bank = calibration_dir + bank_group.generate_output_file_name(
                    calibration.get_sample_path(), calibration.get_instrument())
                EnggUtils.write_ENGINX_GSAS_iparam_file(prm_filepath_bank,
                                                        [difa[ibank]], [difc[ibank]], [tzero[ibank]],
                                                        bk2bk_params, bank_names=bank_group.group.banks,
                                                        template_file=bank_group.get_prm_template_file(),
                                                        ceria_run=ceria_run)
                # copy pdcal output nxs for both banks
                filepath, ext = path.splitext(prm_filepath_bank)
                nxs_filepath_bank = filepath + '.nxs'
                copy2(nxs_filepath, nxs_filepath_bank)
        logger.notice(f"\n\nCalibration files saved to: \"{calibration_dir}\"\n\n")

    @staticmethod
    def read_diff_constants_from_prm(file_path):
        # TODO: Find a way to reliably get the instrument from the file without using the filename.
        diff_consts = []  # one list per component (e.g. bank)
        with open(file_path) as f:
            for line in f.readlines():
                if "ICONS" in line:
                    # If formatted correctly the line should be in the format INS bank ICONS difc difa tzero
                    elements = line.split()
                    bank = elements[1]
                    diff_consts.append(
                        [int(bank) - 1,
                         float(elements[3]),
                         float(elements[4]),
                         float(elements[5])])

        if not diff_consts:
            raise RuntimeError("Invalid file format.")

        return diff_consts

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num)
