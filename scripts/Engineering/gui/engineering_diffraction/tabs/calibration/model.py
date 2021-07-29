# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs

from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger
from mantid.simpleapi import PDCalibration, DeleteWorkspace, CloneWorkspace, DiffractionFocussing, \
    CreateEmptyTableWorkspace, NormaliseByCurrent, ConvertUnits, Load, SaveNexus, ApplyDiffCal
import Engineering.EnggUtils as EnggUtils
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import path_handling

CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"


class CalibrationModel(object):
    def create_new_calibration(self, calibration, rb_num, plot_output):
        """
        Create a new calibration from a vanadium run and ceria run
        :param vanadium_path: Path to vanadium data file.
        :param ceria_path: Path to ceria (CeO2) data file
        :param rb_num: The RB number for file creation.
        """
        ceria_path = calibration.get_sample()
        van_path = calibration.get_vanadium()
        inst = calibration.get_instrument()

        # load grouping workspace (creates if doesn't exist - tries to read from xml for banks)
        group_ws = calibration.get_group_ws()
        # load ceria data
        ceria_workspace = path_handling.load_workspace(ceria_path)

        # load or create vanadium corrections workspaces
        vanadium_corrections.fetch_correction_workspaces(van_path, inst, rb_num)
        # load whole instrument calibration
        full_calib_path = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                                      path_handling.ENGINEERING_PREFIX, "full_calibration")
        try:
            full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
        except ValueError:
            logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
            return

        # run PDCalibraiton
        cal_params, ceria_raw, grp_ws = self.run_calibration(ceria_workspace, group_ws, full_calib)

        # surely do this right at end?
        if plot_output:
            self.plot_output(cal_params, calibration.get_group_suffix())

        # write diff constants to params table
        difa = [row['difa'] for row in cal_params]
        difc = [row['difc'] for row in cal_params]
        tzero = [row['tzero'] for row in cal_params]
        diff_consts = []
        for i in range(len(difc)):
            diff_consts.append([i, difc[i], difa[i], tzero[i]])
        self.update_calibration_params_table(diff_consts)

        # extract Back-to-Back Exponential params for .prm before deleting raw data
        bk2bk_params = self.extract_b2b_params(ceria_raw)
        DeleteWorkspace(ceria_raw)

        # save output
        if rb_num:
            calib_dir = path.join(path_handling.get_output_path(), "User", rb_num, "Calibration", "")
        else:
            calib_dir = path.join(path_handling.get_output_path(), "Calibration", "")

        # save grouping ws if custom or cropped
        if not calibration.group.banks:
            self.calibration.save_grouping_workspace(calib_dir)

        # get diff consts from table?
        self.create_output_files(calib_dir, difa, difc, tzero, bk2bk_params, calibration)

    @staticmethod
    def extract_b2b_params(workspace):

        ws_inst = workspace.getInstrument()
        NorthBank = ws_inst.getComponentByName("NorthBank")
        SouthBank = ws_inst.getComponentByName("SouthBank")
        params_north = []
        params_south = []
        for param_name in ["alpha", "beta_0", "beta_1", "sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]:
            params_north += [NorthBank.getNumberParameter(param_name)[0]]
            params_south += [SouthBank.getNumberParameter(param_name)[0]]

        return [params_north, params_south]

    def plot_output(self, cal_params, group_name):
        """
        Don't like the way EnggUtils.generate_tof_fit_dictionary relies on hard-coding of ws names
        If EnginX script deosn't use this func I'd be tempted to move into model
        """
        plot_dicts = list()
        if len(cal_params) == 1:
            plot_dicts.append(EnggUtils.generate_tof_fit_dictionary(group_name))
            EnggUtils.plot_tof_fit(plot_dicts, [group_name])
        else:
            # want to get rid of special code like this for both banks
            plot_dicts.append(EnggUtils.generate_tof_fit_dictionary("bank_1"))
            plot_dicts.append(EnggUtils.generate_tof_fit_dictionary("bank_2"))
            EnggUtils.plot_tof_fit(plot_dicts, ["bank_1", "bank_2"])

    def load_existing_calibration_files(self, calibration):
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
        # load vanadium workspaces
        inst = calibration.get_instrument()
        van_fname = inst + calibration.get_vanadium()
        vanadium_corrections.fetch_correction_workspaces(van_fname, inst, is_load=True)

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
    def run_calibration(ceria_ws,
                        bank,
                        calfile,
                        spectrum_numbers,
                        full_calib):
        """
        Creates Engineering calibration files with PDCalibration
        :param ceria_ws: The workspace with the ceria data.
        :param bank: The bank to crop to, both if none.
        :param calfile: The custom calibration file to crop to, not used if none.
        :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
        :return: dict containing calibrated diffractometer constants, and copy of the raw ceria workspace
        """

        def run_pd_calibration(kwargs_to_pass) -> list:
            """
            Call PDCalibration using the keyword arguments supplied, and return it's default list of output workspaces
            :param kwargs_to_pass: Keyword arguments to supply to the algorithm
            :return: List of output workspaces from PDCalibration
            """
            return PDCalibration(**kwargs_to_pass)

        def calibrate_region_of_interest(ceria_d_ws, roi: str, grouping_kwarg: dict, cal_output: dict) -> None:
            """
            Focus the processed ceria workspace (dSpacing) over the chosen region of interest, and run the calibration
            using this result
            :param ceria_d_ws: Workspace containing the processed ceria data converted to dSpacing
            :param roi: String describing chosen region of interest
            :param grouping_kwarg: Dict containing kwarg to pass to DiffractionFocussing to select the roi
            :param cal_output: Dictionary to append with the output of PDCalibration for the chosen roi
            """
            # focus ceria
            focused_ceria = DiffractionFocussing(InputWorkspace=ceria_d_ws, **grouping_kwarg)
            ApplyDiffCal(InstrumentWorkspace=focused_ceria, ClearCalibration=True)
            ConvertUnits(InputWorkspace=focused_ceria, OutputWorkspace=focused_ceria, Target='TOF')

            # calibration of focused data over chosen region of interest
            kwargs["InputWorkspace"] = focused_ceria
            kwargs["OutputCalibrationTable"] = "engggui_calibration_" + roi
            kwargs["DiagnosticWorkspaces"] = "diag_" + roi

            cal_roi = run_pd_calibration(kwargs)[0]
            cal_output[roi] = cal_roi

        # need to clone the data as PDCalibration rebins
        ceria_raw = CloneWorkspace(InputWorkspace=ceria_ws)

        # initial process of ceria ws
        NormaliseByCurrent(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws)
        ApplyDiffCal(InstrumentWorkspace=ceria_ws, CalibrationWorkspace=full_calib)
        ConvertUnits(InputWorkspace=ceria_ws, OutputWorkspace=ceria_ws, Target='dSpacing')

        kwargs = {
            "PeakPositions": EnggUtils.default_ceria_expected_peaks(final=True),
            "TofBinning": [15500, -0.0003, 52000],  # using a finer binning now have better stats
            "PeakWindow": 0.04,
            "MinimumPeakHeight": 0.5,
            "PeakFunction": 'BackToBackExponential',
            "CalibrationParameters": 'DIFC+TZERO+DIFA',
            "UseChiSq": True
        }
        cal_output = dict()
        grp_ws = None
        if (spectrum_numbers or calfile) is None:
            if bank == '1' or bank is None:
                grp_ws = EnggUtils.get_bank_grouping_workspace(1, ceria_raw)
                grouping_kwarg = {"GroupingWorkspace": grp_ws}
                calibrate_region_of_interest(ceria_ws, "bank_1", grouping_kwarg, cal_output)
            if bank == '2' or bank is None:
                grp_ws = EnggUtils.get_bank_grouping_workspace(2, ceria_raw)
                grouping_kwarg = {"GroupingWorkspace": grp_ws}
                calibrate_region_of_interest(ceria_ws, "bank_2", grouping_kwarg, cal_output)
        elif calfile is None:
            grp_ws = EnggUtils.create_grouping_workspace_from_spectra_list(spectrum_numbers, ceria_raw)
            grouping_kwarg = {"GroupingWorkspace": grp_ws}
            calibrate_region_of_interest(ceria_ws, "Cropped", grouping_kwarg, cal_output)
        else:
            grp_ws = EnggUtils.create_grouping_workspace_from_calfile(calfile, ceria_raw)
            grouping_kwarg = {"GroupingWorkspace": grp_ws}
            calibrate_region_of_interest(ceria_ws, "Custom", grouping_kwarg, cal_output)
        cal_params = list()
        # in the output calfile, rows are present for all detids, only read one from the region of interest
        for bank_cal in cal_output:
            mask_ws_name = "engggui_calibration_" + bank_cal + "_mask"
            mask_ws = Ads.retrieve(mask_ws_name)
            row_no = EnggUtils.get_first_unmasked_specno_from_mask_ws(mask_ws)
            row = cal_output[bank_cal].row(row_no)
            current_fit_params = {'difc': row['difc'], 'difa': row['difa'], 'tzero': row['tzero']}
            cal_params.append(current_fit_params)
        return cal_params, ceria_raw, grp_ws

    def create_output_files(self, calibration_dir, difa, difc, tzero, bk2bk_params, calibration):
        """
        Create output files from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param difa: DIFA values from calibration algorithm.
        :param difc: DIFC values from the calibration algorithm.
        :param tzero: TZERO values from the calibration algorithm.
        :param bk2bk_params: BackToBackExponential parameters from Parameters.xml file.
        :param ceria_path: The path to the ceria data file.
        :param vanadium_path: The path to the vanadium data file.
        :param instrument: The instrument (ENGINX or IMAT).
        :param bank: Optional parameter to crop by bank.
        :param spectrum_numbers: Optional parameter to crop using spectrum numbers.
        :param calfile: Optional parameter to crop with a custom calfile
        """

        ceria_run = calibration.get_sample_runno()
        van_run = calibration.get_vanadium_runno()

        # create calibration dir of not exiust
        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        # save prm file(s)
        prm_filepath = calibration_dir + calibration.generate_output_file_name()
        EnggUtils.write_ENGINX_GSAS_iparam_file(prm_filepath, difa, difc, tzero, bk2bk_params,
                                                bank_names=calibration.group.banks,
                                                template_file=calibration.get_prm_template_file(),
                                                ceria_run=ceria_run, vanadium_run=van_run)
        if calibration.group == EnggUtils.GROUP.BOTH:
            # output a separate prm for North and South when both banks included
            for ibank, bank in enumerate(self.group.banks):
                bank_group = EnggUtils.GROUP(str(ibank))  # so can retrieve prm template for that bank
                EnggUtils.write_ENGINX_GSAS_iparam_file(prm_filepath, [difa[ibank]], [difc[ibank]], [tzero[ibank]],
                                                        bk2bk_params, bank_names=bank,
                                                        template_file=bank_group.get_prm_template_file(),
                                                        ceria_run=ceria_run, vanadium_run=van_run)

        # save pdcal output as nexus
        filepath, ext = path.splitext(prm_filepath)
        nxs_filepath = filepath + '.nxs'
        ws_name = "engggui_calibration_" + calibration.get_group_suffix()  # would be good not to rely on wsname
        SaveNexus(InputWorkspace=ws_name, Filename=nxs_filepath)

        logger.notice(f"\n\nCalibration files saved to: \"{calibration_dir}\"\n\n")

    @staticmethod
    def read_diff_constants_from_prm(file_path):
        # TODO: Find a way to reliably get the instrument from the file without using the filename.
        diff_consts = []  # one list per component (e.g. bank)
        with open(file_path) as f:
            for line in f:
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
