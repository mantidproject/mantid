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
from mantid.simpleapi import PDCalibration, DeleteWorkspace, CloneWorkspace, Integration, DiffractionFocussing, \
    CreateWorkspace, AppendSpectra, CreateEmptyTableWorkspace, LoadAscii, NormaliseByCurrent, AlignDetectors, \
    EditInstrumentGeometry, ConvertUnits, Load, GroupDetectors, EnggEstimateFocussedBackground, RebinToWorkspace, Divide
from Engineering.EnggUtils import write_ENGINX_GSAS_iparam_file
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import path_handling
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"
NORTH_BANK_CAL = "EnginX_NorthBank.cal"
SOUTH_BANK_CAL = "EnginX_NorthBank.cal"


class CalibrationModel(object):
    def create_new_calibration(self,
                               vanadium_path,
                               sample_path,
                               plot_output,
                               instrument,
                               rb_num=None,
                               bank=None,
                               spectrum_numbers=None):
        """
        Create a new calibration from a vanadium run and sample run
        :param vanadium_path: Path to vanadium data file.
        :param sample_path: Path to sample (CeO2) data file
        :param plot_output: Whether the output should be plotted.
        :param instrument: The instrument the data relates to.
        :param rb_num: The RB number for file creation.
        :param bank: Optional parameter to crop by bank
        :param spectrum_numbers: Optional parameter to crop using spectrum numbers.
        """
        sample_workspace = path_handling.load_workspace(sample_path)
        full_calib_path = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                                      path_handling.ENGINEERING_PREFIX, "full_calibration")
        if full_calib_path is not None and path.exists(full_calib_path):
            full_calib = LoadAscii(full_calib_path, OutputWorkspace="det_pos", Separator="Tab")
            output, sample_raw = self.run_calibration(sample_workspace,
                                                      vanadium_path,
                                                      bank,
                                                      spectrum_numbers,
                                                      full_calib_ws=full_calib)
        else:
            output, sample_raw = self.run_calibration(sample_workspace, vanadium_path, bank, spectrum_numbers)
        if plot_output:
            # TODO determine output to plot
            self._plot_vanadium_curves()
            for i in range(len(output)):
                if spectrum_numbers:
                    bank_name = "cropped"
                elif bank is None:
                    bank_name = str(i + 1)
                else:
                    bank_name = bank
                difa = output[i]['difa']
                difc = output[i]['difc']
                tzero = output[i]['tzero']
                self._generate_tof_fit_workspace(difa, difc, tzero, bank_name)
            if bank is None and spectrum_numbers is None:
                self._plot_tof_fit()
            elif spectrum_numbers is None:
                self._plot_tof_fit_single_bank_or_custom(bank)
            else:
                self._plot_tof_fit_single_bank_or_custom("cropped")
        difa = [row['difa'] for row in output] # TODO these aren't right
        difc = [row['difc'] for row in output]
        tzero = [row['tzero'] for row in output]

        bk2bk_params = self.extract_b2b_params(sample_raw)

        params_table = []

        for i in range(len(difc)):
            params_table.append([i, difc[i], difa[i], tzero[i]])
        self.update_calibration_params_table(params_table)

        calib_dir = path.join(path_handling.get_output_path(), "Calibration", "")
        self.create_output_files(calib_dir, difa, difc, tzero, bk2bk_params, sample_path, vanadium_path, instrument,
                                 bank, spectrum_numbers)
        if rb_num:
            user_calib_dir = path.join(path_handling.get_output_path(), "User", rb_num,
                                       "Calibration", "")
            self.create_output_files(user_calib_dir, difa, difc, tzero, bk2bk_params, sample_path, vanadium_path,
                                     instrument, bank, spectrum_numbers)

    def extract_b2b_params(self, workspace):

        ws_inst = workspace.getInstrument()
        NorthBank = ws_inst.getComponentByName("NorthBank")
        SouthBank = ws_inst.getComponentByName("SouthBank")
        params_north = []
        params_south = []
        for param_name in ["alpha", "beta_0","beta_1","sigma_0_sq", "sigma_1_sq", "sigma_2_sq"]:
            params_north += [NorthBank.getNumberParameter(param_name)[0]]
            params_south += [SouthBank.getNumberParameter(param_name)[0]]

        return [params_north, params_south]

    def load_existing_gsas_parameters(self, file_path):
        if not path.exists(file_path):
            logger.warning("Could not open GSAS calibration file: ", file_path)
            return
        try:
            instrument, van_no, sample_no, params_table = self.get_info_from_file(file_path)
            self.update_calibration_params_table(params_table)
        except RuntimeError:
            logger.error("Invalid file selected: ", file_path)
            return
        vanadium_corrections.fetch_correction_workspaces(instrument+van_no, instrument)
        return instrument, van_no, sample_no

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
    def _plot_vanadium_curves():
        van_curve_twin_ws = "__engggui_vanadium_curves_twin_ws"

        if Ads.doesExist(van_curve_twin_ws):
            DeleteWorkspace(van_curve_twin_ws)
        CloneWorkspace(InputWorkspace="engggui_vanadium_curves", OutputWorkspace=van_curve_twin_ws)
        van_curves_ws = Ads.retrieve(van_curve_twin_ws)

        fig = plt.figure()
        gs = gridspec.GridSpec(1, 2)
        curve_plot_bank_1 = fig.add_subplot(gs[0], projection="mantid")
        curve_plot_bank_2 = fig.add_subplot(gs[1], projection="mantid")

        curve_plot_bank_1.plot(van_curves_ws, wkspIndex=0)
        curve_plot_bank_1.plot(van_curves_ws, wkspIndex=1)
        curve_plot_bank_1.plot(van_curves_ws, wkspIndex=2)
        curve_plot_bank_1.set_title("Engg GUI Vanadium Curves Bank 1")
        curve_plot_bank_1.legend(["Data", "Calc", "Diff"])

        curve_plot_bank_2.plot(van_curves_ws, wkspIndex=3)
        curve_plot_bank_2.plot(van_curves_ws, wkspIndex=4)
        curve_plot_bank_2.plot(van_curves_ws, wkspIndex=5)
        curve_plot_bank_2.set_title("Engg GUI Vanadium Curves Bank 2")
        curve_plot_bank_2.legend(["Data", "Calc", "Diff"])

        fig.show()

    @staticmethod
    def _generate_tof_fit_workspace(difa, difc, tzero, bank):
        bank_ws = Ads.retrieve(CalibrationModel._generate_table_workspace_name(bank))

        x_val = []
        y_val = []
        y2_val = []

        difa_to_plot = difa
        difc_to_plot = difc
        tzero_to_plot = tzero

        for irow in range(0, bank_ws.rowCount()):
            x_val.append(bank_ws.cell(irow, 0))
            y_val.append(bank_ws.cell(irow, 5))
            y2_val.append(pow(x_val[irow], 2) * difa_to_plot + x_val[irow] * difc_to_plot + tzero_to_plot)

        ws1 = CreateWorkspace(DataX=x_val,
                              DataY=y_val,
                              UnitX="Expected Peaks Centre (dSpacing A)",
                              YUnitLabel="Fitted Peaks Centre(TOF, us)")
        ws2 = CreateWorkspace(DataX=x_val, DataY=y2_val)

        output_ws = "engggui_tof_peaks_bank_" + str(bank)
        if Ads.doesExist(output_ws):
            DeleteWorkspace(output_ws)

        AppendSpectra(ws1, ws2, OutputWorkspace=output_ws)
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def _plot_tof_fit(self):
        bank_1_ws = Ads.retrieve("engggui_tof_peaks_bank_1")
        bank_2_ws = Ads.retrieve("engggui_tof_peaks_bank_2")
        # Create plot
        fig = plt.figure()
        gs = gridspec.GridSpec(1, 2)
        plot_bank_1 = fig.add_subplot(gs[0], projection="mantid")
        plot_bank_2 = fig.add_subplot(gs[1], projection="mantid")

        for ax, ws, bank in zip([plot_bank_1, plot_bank_2], [bank_1_ws, bank_2_ws], [1, 2]):
            self._add_plot_to_axes(ax, ws, bank)
        fig.show()

    def _plot_tof_fit_single_bank_or_custom(self, bank):
        bank_ws = Ads.retrieve("engggui_tof_peaks_bank_" + str(bank))
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="mantid")

        self._add_plot_to_axes(ax, bank_ws, bank)
        fig.show()

    @staticmethod
    def _add_plot_to_axes(ax, ws, bank):
        ax.plot(ws, wkspIndex=0, linestyle="", marker="o", markersize="3")
        ax.plot(ws, wkspIndex=1, linestyle="--", marker="o", markersize="3")
        ax.set_title("Engg Gui TOF Peaks Bank " + str(bank))
        ax.legend(("Peaks Fitted", "TOF Quadratic Fit"))
        ax.set_xlabel("Expected Peaks Centre(dSpacing, A)")
        ax.set_ylabel("Fitted Peaks Centre(TOF, us)")

    def run_calibration(self,
                        sample_ws,
                        vanadium_workspace,
                        bank,
                        spectrum_numbers,
                        full_calib_ws=None):
        """
        Runs the main Engineering calibration algorithm.
        :param sample_ws: The workspace with the sample data.
        :param van_integration: The integration values from the vanadium corrections
        :param van_curves: The curves from the vanadium corrections.
        :param full_calib_ws: Full pixel calibration of the detector (optional)
        :param bank: The bank to crop to, both if none.
        :param spectrum_numbers: The spectrum numbers to crop to, no crop if none.
        :return: The output of the algorithm.
        """

        def run_pd_calibration(kwargs_to_pass):
            return PDCalibration(**kwargs_to_pass)

        def focus_and_normalise(ws_d, ws_van_d, cal_file):
            # focus sample
            focused_sample = DiffractionFocussing(InputWorkspace=ws_d, GroupingFileName=cal_file)

            # focus van data
            focused_van = DiffractionFocussing(InputWorkspace=ws_van_d, GroupingFileName=cal_file)
            background_van = EnggEstimateFocussedBackground(InputWorkspace=focused_van, NIterations='15', XWindow=0.03)

            # normalise by focused vanadium
            bg_rebinned = RebinToWorkspace(WorkspaceToRebin=background_van, WorkspaceToMatch=focused_sample)
            normalised = Divide(LHSWorkspace=focused_sample, RHSWorkspace=bg_rebinned)

            # edit instrument geometry so we can convert units
            EditInstrumentGeometry(Workspace=normalised, L2='1.5', Polar='90', InstrumentName='ENGIN-X')
            tof_focused = ConvertUnits(InputWorkspace=normalised, Target='TOF')

            return tof_focused

        # need to clone the data as PDCalibration rebins
        sample_raw = CloneWorkspace(InputWorkspace=sample_ws)

        # TODO get peak positions from EnggUtils as in EnggCal
        dpks = (2.705702376, 1.913220892, 1.631600313, 1.352851554, 1.104598643)

        # TODO if full_calib_ws, PreviousCalibrationTable =

        kwargs = {
            "InputWorkspace": sample_ws,
            "PeakPositions": dpks,
            "TofBinning": [10000, -0.0005, 46000],  # TODO bung all this in constants as well
            "PeakWindow": 0.03,
            "MinimumPeakHeight": 0.5,
            "PeakFunction": 'BackToBackExponential',
            "CalibrationParameters": 'DIFC+TZERO',
            "OutputCalibrationTable": 'cal_B2B_DIFC_TZERO_chisq',
            "DiagnosticWorkspaces": 'diag_B2B_DIFC_TZERO_chisq'
        }

        # initial calibration of instrument
        cal_initial = run_pd_calibration(kwargs)[0]

        ws_van = Load(vanadium_workspace)
        NormaliseByCurrent(InputWorkspace=ws_van, OutputWorkspace=ws_van)
        ws_van_d = AlignDetectors(InputWorkspace=ws_van, CalibrationWorkspace=cal_initial)

        # sensitivity correction for van
        nbins = ws_van.blocksize()
        ws_van_int = Integration(InputWorkspace=ws_van)
        ws_van_int /= nbins

        ws_van_d /= ws_van_int

        # sensitivity correction for sample
        sample = CloneWorkspace(sample_raw)
        NormaliseByCurrent(InputWorkspace=sample, OutputWorkspace=sample)
        ws_d = AlignDetectors(InputWorkspace=sample, CalibrationWorkspace=cal_initial)
        ws_d /= ws_van_int

        dpks_final = [2.705702376, 1.913220892, 1.631600313, 1.562138267, 1.352851554,
                      1.241461538, 1.210027059, 1.104598643, 1.04142562, 0.956610446,
                      0.914694494, 0.901900955, 0.855618487]
        # TODO again get this from EnggUtils
        kwargs = {
            "PeakPositions": dpks_final,
            "TofBinning": [15500, -0.0003, 52000],  # using a finer binning now have better stats
            "PeakWindow": 0.04,
            "MinimumPeakHeight": 0.5,
            "PeakFunction": 'BackToBackExponential',
            "CalibrationParameters": 'DIFC+TZERO+DIFA',
        }
        cal_output = list()

        if spectrum_numbers is None:
            if bank == 1 or bank is None:
                focused_North = focus_and_normalise(ws_d, ws_van_d, NORTH_BANK_CAL)
                # final calibration of focused data
                kwargs["InputWorkspace"] = focused_North
                kwargs["OutputCalibrationTable"] = 'engggui_calibration_bank_1'
                kwargs["DiagnosticWorkspaces"] = 'diag_North'

                cal_north = run_pd_calibration(kwargs)[0]
                cal_output.append(cal_north)

            if bank == 2 or bank is None:
                focused_South = focus_and_normalise(ws_d, ws_van_d, SOUTH_BANK_CAL)
                # final calibration of focused data
                kwargs["InputWorkspace"] = focused_South
                kwargs["OutputCalibrationTable"] = 'engggui_calibration_bank_2'
                kwargs["DiagnosticWorkspaces"] = 'diag_South'

                cal_north = run_pd_calibration(kwargs)[0]
                cal_output.append(cal_north)
        else:
            pass

        output = list()
        for bank_cal in cal_output:
            row = bank_cal.row(0)
            current_fit_params = {'difc': row['difc'], 'difa': row['difa'], 'tzero': row['tzero']}
            output.append(current_fit_params)
        return output, sample_raw

    def create_output_files(self, calibration_dir, difa, difc, tzero, bk2bk_params, sample_path, vanadium_path,
                            instrument, bank, spectrum_numbers):
        """
        Create output files from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param difa: DIFA values from calibration algorithm.
        :param difc: DIFC values from the calibration algorithm.
        :param tzero: TZERO values from the calibration algorithm.
        :param bk2bk_params: BackToBackExponential parameters from Parameters.xml file.
        :param sample_path: The path to the sample data file.
        :param vanadium_path: The path to the vanadium data file.
        :param instrument: The instrument (ENGINX or IMAT).
        :param bank: Optional parameter to crop by bank.
        :param spectrum_numbers: Optional parameter to crop using spectrum numbers.
        """
        kwargs = {"ceria_run": path_handling.get_run_number_from_path(sample_path, instrument),
                  "vanadium_run": path_handling.get_run_number_from_path(vanadium_path, instrument)}

        def south_kwargs():
            kwargs["template_file"] = SOUTH_BANK_TEMPLATE_FILE
            kwargs["bank_names"] = ["South"]

        def north_kwargs():
            kwargs["template_file"] = NORTH_BANK_TEMPLATE_FILE
            kwargs["bank_names"] = ["North"]

        def generate_output_file(difa_list, difc_list, tzero_list, bank_name, kwargs_to_pass):
            file_path = calibration_dir + self._generate_output_file_name(vanadium_path, sample_path, instrument,
                                                                          bank=bank_name)
            write_ENGINX_GSAS_iparam_file(file_path, difa_list, difc_list, tzero_list, bk2bk_params, **kwargs_to_pass)

        if not path.exists(calibration_dir):
            makedirs(calibration_dir)

        if bank is None and spectrum_numbers is None:
            generate_output_file(difa, difc, tzero, "all", kwargs)
            north_kwargs()
            generate_output_file([difa[0]], [difc[0]], [tzero[0]], "north", kwargs)
            south_kwargs()
            generate_output_file([difa[1]], [difc[1]], [tzero[1]], "south", kwargs)
        elif bank == "1":
            north_kwargs()
            generate_output_file([difa[0]], [difc[0]], [tzero[0]], "north", kwargs)
        elif bank == "2":
            south_kwargs()
            generate_output_file([difa[0]], [difc[0]], [tzero[0]], "south", kwargs)
        elif bank is None:  # Custom cropped files use the north bank template.
            north_kwargs()
            generate_output_file([difa[0]], [difc[0]], [tzero[0]], "cropped", kwargs)
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
        sample_no = words[2]  # Run numbers are stored as the 3rd and 4th word in this line.
        van_no = words[3]
        return instrument, van_no, sample_no, params_table

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num)

    @staticmethod
    def _generate_output_file_name(vanadium_path, sample_path, instrument, bank):
        """
        Generate an output filename in the form INSTRUMENT_VanadiumRunNo_SampleRunNo_BANKS
        :param vanadium_path: Path to vanadium data file
        :param sample_path: Path to sample data file
        :param instrument: The instrument in use.
        :param bank: The bank being saved.
        :return: The filename, the vanadium run number, and sample run number.
        """
        vanadium_no = path_handling.get_run_number_from_path(vanadium_path, instrument)
        sample_no = path_handling.get_run_number_from_path(sample_path, instrument)
        filename = instrument + "_" + vanadium_no + "_" + sample_no + "_"
        if bank == "all":
            filename = filename + "all_banks.prm"
        elif bank == "north":
            filename = filename + "bank_North.prm"
        elif bank == "south":
            filename = filename + "bank_South.prm"
        elif bank == "cropped":
            filename = filename + "cropped.prm"
        else:
            raise ValueError("Invalid bank name entered")
        return filename
