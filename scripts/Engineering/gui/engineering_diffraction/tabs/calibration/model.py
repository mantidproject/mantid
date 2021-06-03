# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs
from matplotlib import gridspec
import matplotlib.pyplot as plt

from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger
from mantid.simpleapi import PDCalibration, DeleteWorkspace, CloneWorkspace, DiffractionFocussing, \
    CreateEmptyTableWorkspace, NormaliseByCurrent, RenameWorkspace, \
    ConvertUnits, Load, ReplaceSpecialValues, SaveNexus, \
    EnggEstimateFocussedBackground, ApplyDiffCal
from Engineering.EnggUtils import write_ENGINX_GSAS_iparam_file, default_ceria_expected_peaks, \
    create_custom_grouping_workspace, create_spectrum_list_from_string, load_relevant_pdcal_outputs,\
    generate_tof_fit_workspace
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import path_handling

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"
NORTH_BANK_CAL = "EnginX_NorthBank.cal"
SOUTH_BANK_CAL = "EnginX_SouthBank.cal"


class CalibrationModel(object):
    def create_new_calibration(self,
                               vanadium_path,
                               ceria_path,
                               plot_output,
                               instrument,
                               rb_num=None,
                               bank=None,
                               spectrum_numbers=None):
        """
        Create a new calibration from a vanadium run and ceria run
        :param vanadium_path: Path to vanadium data file.
        :param ceria_path: Path to ceria (CeO2) data file
        :param plot_output: Whether the output should be plotted.
        :param instrument: The instrument the data relates to.
        :param rb_num: The RB number for file creation.
        :param bank: Optional parameter to crop by bank
        :param spectrum_numbers: Optional parameter to crop using spectrum numbers.
        """
        van_integration, van_curves = vanadium_corrections.fetch_correction_workspaces(
            vanadium_path, instrument, rb_num=rb_num)  # van_curves = None at this point if recalc vanadium
        ceria_workspace = path_handling.load_workspace(ceria_path)
        full_calib_path = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                                      path_handling.ENGINEERING_PREFIX, "full_calibration")
        try:
            full_calib = Load(full_calib_path, OutputWorkspace="full_inst_calib")
        except ValueError:
            logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
            return
        cal_params, van_curves, ceria_raw = self.run_calibration(ceria_workspace,
                                                                 vanadium_path,
                                                                 van_integration,
                                                                 bank,
                                                                 spectrum_numbers,
                                                                 full_calib)
        vanadium_corrections.handle_van_curves(van_curves, vanadium_path, instrument, rb_num)
        if plot_output:
            for i in range(len(cal_params)):
                if spectrum_numbers:
                    bank_name = "Cropped"
                elif bank is None:
                    bank_name = "bank_" + str(i + 1)
                else:
                    bank_name = bank
                generate_tof_fit_workspace(bank_name)
            if bank is None and spectrum_numbers is None:
                self._plot_tof_fit(["bank_1", "bank_2"])
            elif spectrum_numbers is None:
                self._plot_tof_fit([bank])
            else:
                self._plot_tof_fit(["Cropped"])
        difa = [row['difa'] for row in cal_params]
        difc = [row['difc'] for row in cal_params]
        tzero = [row['tzero'] for row in cal_params]

        bk2bk_params = self.extract_b2b_params(ceria_raw)
        DeleteWorkspace(ceria_raw)

        params_table = []

        for i in range(len(difc)):
            params_table.append([i, difc[i], difa[i], tzero[i]])
        self.update_calibration_params_table(params_table)

        calib_dir = path.join(path_handling.get_output_path(), "Calibration", "")
        self.create_output_files(calib_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path, instrument,
                                 bank, spectrum_numbers)
        if rb_num:
            user_calib_dir = path.join(path_handling.get_output_path(), "User", rb_num,
                                       "Calibration", "")
            self.create_output_files(user_calib_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path,
                                     instrument, bank, spectrum_numbers)

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
        # TODO write test this function in conjunction with get_info_from_file once neat way of getting test file found
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
        vanadium_corrections.fetch_correction_workspaces(instrument+van_no, instrument)
        load_relevant_pdcal_outputs(file_path)
        return instrument, van_no, ceria_no

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

    def _plot_tof_fit(self, regions):
        n_workspaces = len(regions)
        bank_ws_names = [f"engggui_tof_peaks_{region}" for region in regions]
        residuals_ws_names = [f"{ws}_residuals" for ws in bank_ws_names]
        bank_workspaces = [Ads.retrieve(ws_name) for ws_name in bank_ws_names]
        residuals_workspaces = [Ads.retrieve(ws_name) for ws_name in residuals_ws_names]

        # Create plot
        fig = plt.figure()
        gs = gridspec.GridSpec(2, n_workspaces)
        bank_axes = [fig.add_subplot(gs[0, n], projection="mantid") for n in range(n_workspaces)]
        residuals_axes = [fig.add_subplot(gs[1, n], projection="mantid") for n in range(n_workspaces)]

        for ax, ws, bank in zip(bank_axes, bank_workspaces, regions):
            self._add_plot_to_axes(ax, ws, bank)
        for ax, ws in zip(residuals_axes, residuals_workspaces):
            self._add_residuals_to_axes(ax, ws)
        fig.tight_layout()
        fig.show()

    @staticmethod
    def _add_plot_to_axes(ax, ws, bank):
        ax.plot(ws, wkspIndex=1, linestyle="-", marker="None", color='r', label="TOF Quadratic Fit")
        ax.errorbar(ws, wkspIndex=0, capsize=2, marker=".", color='b', label="Peaks Fitted", ls="None")
        ax.set_title("Engg Gui TOF Peaks " + str(bank))
        ax.legend()
        ax.set_xlabel("")  # hide here as set automatically
        ax.set_ylabel("Fitted Peaks Centre(TOF, \u03BCs)")

    @staticmethod
    def _add_residuals_to_axes(ax, ws):
        ax.errorbar(ws, color='b', marker='.', wkspIndex=0, capsize=2, ls="None")
        ax.axhline(color='r')
        ax.set_xlabel("Expected Peaks Centre(dSpacing, A)")
        ax.set_ylabel("Residuals \u03BCs")

    @staticmethod
    def run_calibration(ceria_ws,
                        vanadium_workspace,
                        van_integration,
                        bank,
                        spectrum_numbers,
                        full_calib):
        """
        Creates Engineering calibration files with PDCalibration
        :param ceria_ws: The workspace with the ceria data.
        :param vanadium_workspace: The workspace with the vanadium data
        :param van_integration: The integration values from the vanadium corrections
        :param bank: The bank to crop to, both if none.
        :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
        :return: The calibration output files, the vanadium curves workspace(s), and a clone of the ceria file
        """

        def run_pd_calibration(kwargs_to_pass):
            return PDCalibration(**kwargs_to_pass)

        def focus_and_make_van_curves(ceria_d, vanadium_d, grouping_kwarg):
            # focus ceria
            focused_ceria = DiffractionFocussing(InputWorkspace=ceria_d, **grouping_kwarg)
            ApplyDiffCal(InstrumentWorkspace=focused_ceria, ClearCalibration=True)
            tof_focused = ConvertUnits(InputWorkspace=focused_ceria, Target='TOF')

            # focus van data
            focused_van = DiffractionFocussing(InputWorkspace=vanadium_d, **grouping_kwarg)

            background_van = EnggEstimateFocussedBackground(InputWorkspace=focused_van, NIterations='15', XWindow=0.03)

            DeleteWorkspace(focused_ceria)
            DeleteWorkspace(focused_van)

            return tof_focused, background_van

        def ws_initial_process(ws):
            """Run some processing common to both the sample and vanadium workspaces"""
            NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws)
            ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib)
            ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='dSpacing')
            return ws

        def calibrate_region_of_interest(roi, df_kwarg):
            focused_roi, curves_roi = focus_and_make_van_curves(ws_d, ws_van_d, df_kwarg)
            RenameWorkspace(curves_roi, ("curves_" + roi))
            curves_output.append(curves_roi)

            # final calibration of focused data
            kwargs["InputWorkspace"] = focused_roi
            kwargs["OutputCalibrationTable"] = "engggui_calibration_" + roi
            kwargs["DiagnosticWorkspaces"] = "diag_" + roi

            cal_roi = run_pd_calibration(kwargs)[0]
            cal_output[roi] = cal_roi

        # need to clone the data as PDCalibration rebins
        ceria_raw = CloneWorkspace(InputWorkspace=ceria_ws)

        ws_van = Load(vanadium_workspace)
        ws_van_d = ws_initial_process(ws_van)

        # van sensitivity correction
        ws_van_d /= van_integration
        ReplaceSpecialValues(InputWorkspace=ws_van_d, OutputWorkspace=ws_van_d, NaNValue=0, InfinityValue=0)

        ws_d = ws_initial_process(ceria_ws)

        kwargs = {
            "PeakPositions": default_ceria_expected_peaks(final=True),
            "TofBinning": [15500, -0.0003, 52000],  # using a finer binning now have better stats
            "PeakWindow": 0.04,
            "MinimumPeakHeight": 0.5,
            "PeakFunction": 'BackToBackExponential',
            "CalibrationParameters": 'DIFC+TZERO+DIFA',
            "UseChiSq": True
        }
        cal_output = dict()
        curves_output = list()

        if spectrum_numbers is None:
            if bank == '1' or bank is None:
                df_kwarg = {"GroupingFileName": NORTH_BANK_CAL}
                calibrate_region_of_interest("bank_1", df_kwarg)
            if bank == '2' or bank is None:
                df_kwarg = {"GroupingFileName": SOUTH_BANK_CAL}
                calibrate_region_of_interest("bank_2", df_kwarg)
        else:
            grp_ws = create_custom_grouping_workspace(spectrum_numbers, ceria_raw)
            df_kwarg = {"GroupingWorkspace": grp_ws}
            calibrate_region_of_interest("Cropped", df_kwarg)

        DeleteWorkspace(ws_van)
        DeleteWorkspace("tof_focused")

        cal_params = list()
        # in the output calfile, rows are present for all detids, only read one from the region of interest
        bank_1_read_row = 0
        bank_2_read_row = 1200
        for bank_cal in cal_output:
            if bank_cal == "bank_1":
                read = bank_1_read_row
            elif bank_cal == "bank_2":
                read = bank_2_read_row
            else:
                read = create_spectrum_list_from_string(spectrum_numbers)[0]
            row = cal_output[bank_cal].row(read)
            current_fit_params = {'difc': row['difc'], 'difa': row['difa'], 'tzero': row['tzero']}
            cal_params.append(current_fit_params)
        return cal_params, curves_output, ceria_raw

    def create_output_files(self, calibration_dir, difa, difc, tzero, bk2bk_params, ceria_path, vanadium_path,
                            instrument, bank, spectrum_numbers):
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
            file_path = calibration_dir + self._generate_output_file_name(vanadium_path, ceria_path, instrument,
                                                                          bank=bank_name)
            write_ENGINX_GSAS_iparam_file(file_path, difa_list, difc_list, tzero_list, bk2bk_params, **kwargs_to_pass)

        def save_pdcal_output_file(ws_name_suffix, bank_name):
            file_path = calibration_dir + self._generate_output_file_name(vanadium_path, ceria_path, instrument,
                                                                          bank=bank_name, ext=".nxs")
            ws_name = "engggui_calibration_" + ws_name_suffix
            SaveNexus(InputWorkspace=ws_name, Filename=file_path)

        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        if bank is None and spectrum_numbers is None:
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
        elif bank is None:  # Custom cropped files use the north bank template.
            north_kwargs()
            generate_prm_output_file([difa[0]], [difc[0]], [tzero[0]], "cropped", kwargs)
            save_pdcal_output_file("cropped", "cropped")
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

    @staticmethod
    def _generate_output_file_name(vanadium_path, ceria_path, instrument, bank, ext='.prm'):
        """
        Generate an output filename in the form INSTRUMENT_VanadiumRunNo_ceriaRunNo_BANKS
        :param vanadium_path: Path to vanadium data file
        :param ceria_path: Path to ceria data file
        :param instrument: The instrument in use.
        :param bank: The bank being saved.
        :return: The filename, the vanadium run number, and ceria run number.
        """
        vanadium_no = path_handling.get_run_number_from_path(vanadium_path, instrument)
        ceria_no = path_handling.get_run_number_from_path(ceria_path, instrument)
        filename = instrument + "_" + vanadium_no + "_" + ceria_no + "_"
        if bank == "all":
            filename = filename + "all_banks" + ext
        elif bank == "north":
            filename = filename + "bank_1" + ext
        elif bank == "south":
            filename = filename + "bank_2" + ext
        elif bank == "cropped":
            filename = filename + "Cropped" + ext
        else:
            raise ValueError("Invalid bank name entered")
        return filename
