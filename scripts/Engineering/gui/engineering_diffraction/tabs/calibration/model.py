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
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections, output_settings
from Engineering.common import path_handling

CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"


class CalibrationModel(object):
    def create_new_calibration(self,
                               vanadium_path,
                               ceria_path,
                               plot_output,
                               instrument,
                               rb_num=None,
                               bank=None,
                               calfile=None,
                               spectrum_numbers=None):
        """
        Create a new calibration from a vanadium run and ceria run
        :param vanadium_path: Path to vanadium data file.
        :param ceria_path: Path to ceria (CeO2) data file
        :param plot_output: Whether the output should be plotted.
        :param instrument: The instrument the data relates to.
        :param rb_num: The RB number for file creation.
        :param bank: Optional parameter to crop by bank
        :param calfile: Optional parameter to crop using a custom calfile
        :param spectrum_numbers: Optional parameter to crop using spectrum numbers.
        """
        # vanadium corrections workspaces not used at this stage, but ensure they exist and create if not
        vanadium_corrections.fetch_correction_workspaces(vanadium_path, instrument, rb_num=rb_num)
        ceria_workspace = path_handling.load_workspace(ceria_path)
        full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                      output_settings.ENGINEERING_PREFIX, "full_calibration")
        try:
            full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
        except ValueError:
            logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
            return
        cal_params, ceria_raw, grp_ws = self.run_calibration(ceria_workspace,
                                                             bank,
                                                             calfile,
                                                             spectrum_numbers,
                                                             full_calib)
        if plot_output:
            plot_dicts = list()
            if len(cal_params) == 1:
                if calfile:
                    bank_name = "Custom"
                elif spectrum_numbers:
                    bank_name = "Cropped"
                else:
                    bank_name = bank
                plot_dicts.append(EnggUtils.generate_tof_fit_dictionary(bank_name))
                EnggUtils.plot_tof_fit(plot_dicts, [bank_name])
            else:
                plot_dicts.append(EnggUtils.generate_tof_fit_dictionary("bank_1"))
                plot_dicts.append(EnggUtils.generate_tof_fit_dictionary("bank_2"))
                EnggUtils.plot_tof_fit(plot_dicts, ["bank_1", "bank_2"])
        difa = [row['difa'] for row in cal_params]
        difc = [row['difc'] for row in cal_params]
        tzero = [row['tzero'] for row in cal_params]

        bk2bk_params = self.extract_b2b_params(ceria_raw)
        DeleteWorkspace(ceria_raw)

        params_table = []

        for i in range(len(difc)):
            params_table.append([i, difc[i], difa[i], tzero[i]])
        self.update_calibration_params_table(params_table)

        calib_dir = path.join(output_settings.get_output_path(), "Calibration", "")
        if calfile:
            EnggUtils.save_grouping_workspace(grp_ws, calib_dir, ceria_path, vanadium_path, instrument, calfile=calfile)
        elif spectrum_numbers:
            EnggUtils.save_grouping_workspace(grp_ws, calib_dir, ceria_path, vanadium_path, instrument,
                                              spec_nos=spectrum_numbers)
        self.create_output_files(calib_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path, instrument,
                                 bank, spectrum_numbers, calfile)
        if rb_num:
            user_calib_dir = path.join(output_settings.get_output_path(), "User", rb_num,
                                       "Calibration", "")
            self.create_output_files(user_calib_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path,
                                     instrument, bank, spectrum_numbers, calfile)

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

    def load_existing_calibration_files(self, file_path):
        if not path.exists(file_path):
            msg = "Could not open GSAS calibration file: " + file_path
            logger.warning(msg)
            return
        try:
            instrument, van_no, ceria_no, params_table = self.get_info_from_file(file_path)
            self.update_calibration_params_table(params_table)
        except RuntimeError:
            logger.error("Invalid file selected: ", file_path)
            return
        vanadium_corrections.fetch_correction_workspaces(instrument+van_no, instrument, is_load=True)
        bank = EnggUtils.load_relevant_calibration_files(file_path)
        grp_ws_name, roi_text = EnggUtils.load_custom_grouping_workspace(file_path)
        return instrument, van_no, ceria_no, grp_ws_name, roi_text, bank

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

    def create_output_files(self, calibration_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path,
                            instrument, bank, spectrum_numbers, calfile):
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
        kwargs = {"ceria_run": path_handling.get_run_number_from_path(ceria_path, instrument),
                  "vanadium_run": path_handling.get_run_number_from_path(vanadium_path, instrument)}

        def south_kwargs():
            kwargs["template_file"] = SOUTH_BANK_TEMPLATE_FILE
            kwargs["bank_names"] = ["South"]

        def north_kwargs():
            kwargs["template_file"] = NORTH_BANK_TEMPLATE_FILE
            kwargs["bank_names"] = ["North"]

        def generate_prm_output_file(difa_list, difc_list, tzero_list, bank_name, kwargs_to_pass):
            file_path = calibration_dir + EnggUtils.generate_output_file_name(vanadium_path, ceria_path, instrument,
                                                                              bank=bank_name)
            EnggUtils.write_ENGINX_GSAS_iparam_file(file_path, difa_list, difc_list, tzero_list, bk2bk_params,
                                                    **kwargs_to_pass)
            set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                        "last_calibration_path", file_path)

        def save_pdcal_output_file(ws_name_suffix, bank_name):
            file_path = calibration_dir + EnggUtils.generate_output_file_name(vanadium_path, ceria_path, instrument,
                                                                              bank=bank_name, ext=".nxs")
            ws_name = "engggui_calibration_" + ws_name_suffix
            SaveNexus(InputWorkspace=ws_name, Filename=file_path)

        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        if not (bank or spectrum_numbers or calfile):
            # both banks
            generate_prm_output_file(difa, difc, tzero, "all", kwargs)
            north_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "north", kwargs)
            save_pdcal_output_file("bank_1", "north")
            south_kwargs()
            generate_prm_output_file([difa[1]], [difc[1]], [tzero[1]], "south", kwargs)
            save_pdcal_output_file("bank_2", "south")
        elif bank == "1":
            north_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "north", kwargs)
            save_pdcal_output_file("bank_1", "north")
        elif bank == "2":
            south_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "south", kwargs)
            save_pdcal_output_file("bank_2", "south")
        elif spectrum_numbers:  # Custom crops use the north bank template
            north_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "Cropped", kwargs)
            save_pdcal_output_file("Cropped", "Cropped")
        else:  # custom calfile
            north_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "Custom", kwargs)
            save_pdcal_output_file("Custom", "Custom")
        logger.notice(f"\n\nCalibration files saved to: \"{calibration_dir}\"\n\n")

    @staticmethod
    def get_info_from_file(file_path):
        # TODO: Find a way to reliably get the instrument from the file without using the filename.
        instrument = file_path.split("/")[-1].split("_", 1)[0]
        # Get run numbers from file.
        run_numbers = ""
        params_table = []
        with open(file_path) as f:
            for line in f:
                if "INS    CALIB" in line:
                    run_numbers = line
                if "ICONS" in line:
                    # If formatted correctly the line should be in the format INS bank ICONS difc difa tzero
                    elements = line.split()
                    bank = elements[1]
                    params_table.append(
                        [int(bank) - 1,
                         float(elements[3]),
                         float(elements[4]),
                         float(elements[5])])

        if run_numbers == "":
            raise RuntimeError("Invalid file format.")

        words = run_numbers.split()
        ceria_no = words[2]  # Run numbers are stored as the 3rd and 4th word in this line.
        van_no = words[3]
        return instrument, van_no, ceria_no, params_table

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num)
