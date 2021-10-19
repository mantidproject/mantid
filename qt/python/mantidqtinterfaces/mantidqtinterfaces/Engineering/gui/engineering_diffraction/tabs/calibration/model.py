# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs
from shutil import copy2
from numpy import array, degrees
from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger, UnitParams
from mantid.simpleapi import PDCalibration, DeleteWorkspace, DiffractionFocussing, CreateDetectorTable,\
    CreateEmptyTableWorkspace, NormaliseByCurrent, ConvertUnits, Load, SaveNexus, ApplyDiffCal
import Engineering.EnggUtils as EnggUtils
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_presenter import CALIB_FOLDER
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.common import path_handling

DIFF_CONSTS_TABLE_NAME = "diffractometer_consts_table"


class CalibrationModel(object):

    def create_new_calibration(self, calibration, rb_num, plot_output):
        """
        Create a new calibration from a ceria run
        :param ceria_path: Path to ceria (CeO2) data file
        :param rb_num: The RB number for file creation.
        """

        # load ceria data
        ceria_workspace = path_handling.load_workspace(calibration.get_ceria_path())

        # load whole instrument calibration
        full_calib = load_full_instrument_calibration()

        # run PDCalibration
        focused_ceria, cal_table, diag_ws = self.run_calibration(ceria_workspace, calibration, full_calib)

        # extract diffractometer constants from calibration and write to table
        self.make_diff_consts_table(focused_ceria)

        if plot_output:
            title_prefix = f"{calibration.get_foc_ws_suffix().replace('_', ' ')} spec: "
            EnggUtils.plot_tof_vs_d_from_calibration(diag_ws, focused_ceria,
                                                     EnggUtils.default_ceria_expected_peaks(final=True),
                                                     title_prefix=title_prefix)

        # save output
        if rb_num:
            calib_dir = path.join(output_settings.get_output_path(), "User", rb_num, "Calibration", "")
        else:
            calib_dir = path.join(output_settings.get_output_path(), "Calibration", "")
        self.create_output_files(calib_dir, calibration, focused_ceria)

        DeleteWorkspace(ceria_workspace)

    def write_prm_file(self, ws_foc, prm_savepath, spec_nums = None):
        """
        Save GSAS prm file for ENGINX data - for specification see manual
        https://subversion.xray.aps.anl.gov/EXPGUI/gsas/all/GSAS%20Manual.pdf
        :param ws_foc: focused workspace (used to get detector positions and diff constants)
        :param prm_savepath: path to save prm to
        :param spec_index: list of indices to save (e.g. can specify a particular bank)
        """
        if not spec_nums:
            spec_nums = range(ws_foc.getNumberHistograms())  # one block per spectrum in ws
        nspec = len(spec_nums)
        # read header
        with open(path.join(CALIB_FOLDER, "template_ENGINX_prm_header.prm")) as fheader:
            lines = fheader.readlines()
        lines[1] = lines[1].replace('2', f'{nspec}')  # replace with nspectra in header
        lines[13] = lines[13].replace('241391', f'{ws_foc.run().get("run_number").value}')  # replace run num
        # add blocks
        si = ws_foc.spectrumInfo()
        inst = ws_foc.getInstrument()
        endl = lines[0][-1]  # new line char
        for iblock, ispec in enumerate(spec_nums):
            # detector parameters
            l2 = si.l2(ispec)
            phi, tth = degrees(si.geographicalAngles(ispec))
            dc = si.diffractometerConstants(ispec)
            difa, difc, tzero = [dc[param] for param in [UnitParams.difa, UnitParams.difc, UnitParams.tzero]]
            block = [f'INS  {iblock+1}I ITYP\t0\t1.0000\t80.0000\t0{endl}']
            block.extend(f'INS  {iblock+1}BNKPAR\t{l2:.3f}\t{abs(tth):.3f}\t{phi:.3f}\t0.000\t0.000\t0\t0{endl}')
            block.extend(f'INS  {iblock+1} ICONS\t{difc:.2f}\t{difa:.2f}\t{tzero:.2f}{endl}')
            # TOF peak profile parameters
            alpha0, beta0, beta1, sig0_sq, sig1_sq, sig2_sq = self.getParametersFromDetector(inst,
                                                                                             ws_foc.getDetector(ispec))
            block.extend(f'INS  {iblock+1}PRCF1 \t3\t21\t0.00050{endl}')
            block.extend(f'INS  {iblock+1}PRCF11\t{alpha0:.6E}\t{beta0:.6E}\t{beta1:.6E}\t{sig0_sq:.6E}{endl}')
            block.extend(f'INS  {iblock+1}PRCF12\t{sig1_sq:.6E}\t{sig2_sq:.6E}\t{0.0:.6E}\t{0.0:.6E}{endl}')
            for iblank in [3, 4, 5]:
                block.extend(f'INS  {iblock+1}PRCF1{iblank}\t{0.0:.6E}\t{0.0:.6E}\t{0.0:.6E}\t{0.0:.6E}{endl}')
            block.extend(f'INS  {iblock+1}PRCF16\t{0.0:.6E}{endl}')
            # append to lines
            lines.extend(block)
        # write lines to prm file
        with open(prm_savepath, 'w') as fout:
            fout.writelines(lines)

    def load_existing_calibration_files(self, calibration):
        load_full_instrument_calibration()
        # load prm
        prm_filepath = calibration.prm_filepath
        if not path.exists(prm_filepath):
            msg = f"Could not open GSAS calibration file: {prm_filepath}"
            logger.warning(msg)
            return
        try:
            # read diff constants from prm
            self.write_diff_consts_to_table_from_prm(prm_filepath)
        except RuntimeError:
            logger.error(f"Invalid file selected: {prm_filepath}")
            return
        calibration.load_relevant_calibration_files()

    def write_diff_consts_to_table_from_prm(self, prm_filepath):
        """
        read diff consntants from prm file and write in table workspace
        :param prm_filepath: path to prm file
        """
        diff_consts = self.read_diff_constants_from_prm(prm_filepath)
        # make table
        table = CreateEmptyTableWorkspace(OutputWorkspace=DIFF_CONSTS_TABLE_NAME)
        table.addColumn("int", "Index")
        table.addColumn("double", "DIFA")
        table.addColumn("double", "DIFC")
        table.addColumn("double", "TZERO")
        # add to row per spectrum to table
        for ispec in range(len(diff_consts)):
            table.addRow([ispec, *diff_consts[ispec, :]])

    @staticmethod
    def make_diff_consts_table(ws_foc):
        """
        Summarise diff constants in table workspace (adapt from detector table)
        :param ws_foc: focused ceria workspace
        """
        table = CreateDetectorTable(InputWorkspace=ws_foc, DetectorTableWorkspace=DIFF_CONSTS_TABLE_NAME)
        col_names = ['Spectrum No', "Detector ID(s)", "Monitor", "DIFC - Uncalibrated"]
        for col in col_names:
            table.removeColumn(col)

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
        grp_ws = calibration.get_group_ws()  # (creates if doesn't exist)
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

        # warn user which spectra were unsuccessfully calibrated
        focused_ceria = Ads.retrieve(foc_name)
        masked_detIDs = mask.getMaskedDetectors()
        for ispec in range(focused_ceria.getNumberHistograms()):
            if focused_ceria.getSpectrum(ispec).getDetectorIDs()[0] in masked_detIDs:
                logger.warning(f'PDCalibration failed for spectrum index {ispec} - proceeding with uncalibrated DIFC.')

        # store cal_table in calibration
        calibration.set_calibration_table(cal_table_name)

        return focused_ceria, cal_table, diag_ws

    def create_output_files(self, calibration_dir, calibration, ws_foc):
        """
        Create output files (.prm for GSAS and .nxs of calibration table) from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param calibration: CalibrationInfo object with details of calibration and grouping
        :param ws_foc: focused ceria workspace
        """
        # create calibration dir of not exist
        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        # save grouping ws if custom or cropped
        if not calibration.group.banks:
            calibration.save_grouping_workspace(calibration_dir)

        # save prm file(s)
        prm_filepath = path.join(calibration_dir, calibration.generate_output_file_name())
        self.write_prm_file(ws_foc, prm_filepath)
        calibration.set_prm_filepath(prm_filepath)
        # save pdcal output as nexus
        filepath, ext = path.splitext(prm_filepath)
        nxs_filepath = filepath + '.nxs'
        SaveNexus(InputWorkspace=calibration.get_calibration_table(), Filename=nxs_filepath)

        # if both banks calibrated save individual banks separately as well
        if calibration.group == EnggUtils.GROUP.BOTH:
            # output a separate prm for North and South when both banks included
            for ibank, bank in enumerate(calibration.group.banks):
                # create temp group to get prm template for the bank
                bank_group = CalibrationInfo(EnggUtils.GROUP(str(ibank + 1)),
                                             calibration.get_instrument(), calibration.get_ceria_path())
                prm_filepath_bank = path.join(calibration_dir, bank_group.generate_output_file_name())
                self.write_prm_file(ws_foc, prm_filepath_bank, spec_nums=[ibank])
                # copy pdcal output nxs for both banks
                filepath, ext = path.splitext(prm_filepath_bank)
                nxs_filepath_bank = filepath + '.nxs'
                copy2(nxs_filepath, nxs_filepath_bank)
        logger.notice(f"\n\nCalibration files saved to: \"{calibration_dir}\"\n\n")

    @staticmethod
    def getParametersFromDetector(instrument, detector):
        """
        Get BackToBackExponential parameters from highest level component in tree
        :param instrument:
        :param detector:
        :return: list of parameters
        """
        inst_tree = detector.getFullName().split(detector.getNameSeparator())[0].split('/')
        param_names = ["alpha_0", "beta_0", "beta_1", "sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]
        params = None
        for comp_name in inst_tree:
            comp = instrument.getComponentByName(comp_name)
            if comp.hasParameter(param_names[0]):
                params = [comp.getNumberParameter(param)[0] for param in param_names]
                break
        return params

    @staticmethod
    def read_diff_constants_from_prm(file_path):
        """
        :param file_path: path to prm file
        :return: (nspec x 3) array with columns difa difc tzero (in that order - same as in detector table).
        """
        diff_consts = []  # one list per component (e.g. bank)
        with open(file_path) as f:
            for line in f.readlines():
                if "ICONS" in line:
                    # If formatted correctly the line should be in the format INS bank ICONS difc difa tzero
                    elements = line.split()
                    diff_consts.append([float(elements[ii]) for ii in [4, 3, 5]])
        if not diff_consts:
            raise RuntimeError("Invalid file format.")
        return array(diff_consts)

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num)


def load_full_instrument_calibration():
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
