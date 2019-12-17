# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from os import path, makedirs
from matplotlib import gridspec
import matplotlib.pyplot as plt

from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger
from mantid.simpleapi import Load, EnggCalibrate, DeleteWorkspace, CloneWorkspace, \
    CreateWorkspace, AppendSpectra, CreateEmptyTableWorkspace
from Engineering.EnggUtils import write_ENGINX_GSAS_iparam_file
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import path_handling

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"


class CalibrationModel(object):
    def create_new_calibration(self,
                               vanadium_path,
                               sample_path,
                               plot_output,
                               instrument,
                               rb_num=None):
        """
        Create a new calibration from a vanadium run and sample run
        :param vanadium_path: Path to vanadium data file.
        :param sample_path: Path to sample (CeO2) data file
        :param plot_output: Whether the output should be plotted.
        :param instrument: The instrument the data relates to.
        :param rb_num: The RB number for file creation.
        """
        van_integration, van_curves = vanadium_corrections.fetch_correction_workspaces(
            vanadium_path, instrument, rb_num=rb_num)
        sample_workspace = self.load_sample(sample_path)
        output = self.run_calibration(sample_workspace, van_integration, van_curves)
        if plot_output:
            self._plot_vanadium_curves()
            for i in range(2):
                difc = [output[i].DIFC]
                tzero = [output[i].TZERO]
                self._generate_difc_tzero_workspace(difc, tzero, i + 1)
            self._plot_difc_tzero()
        difc = [output[0].DIFC, output[1].DIFC]
        tzero = [output[0].TZERO, output[1].TZERO]

        params_table = []
        for i in range(2):
            params_table.append([i, difc[i], 0.0, tzero[i]])
        self.update_calibration_params_table(params_table)

        calib_dir = path.join(path_handling.get_output_path(), "Calibration", "")
        self.create_output_files(calib_dir, difc, tzero, sample_path, vanadium_path, instrument)
        if rb_num:
            user_calib_dir = path.join(path_handling.get_output_path(), "User", rb_num,
                                       "Calibration", "")
            self.create_output_files(user_calib_dir, difc, tzero, sample_path, vanadium_path,
                                     instrument)

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
        vanadium_corrections.fetch_correction_workspaces(van_no, instrument)
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
    def _generate_difc_tzero_workspace(difc, tzero, bank):
        bank_ws = Ads.retrieve(CalibrationModel._generate_table_workspace_name(bank - 1))

        x_val = []
        y_val = []
        y2_val = []

        difc_to_plot = difc[0]
        tzero_to_plot = tzero[0]

        for irow in range(0, bank_ws.rowCount()):
            x_val.append(bank_ws.cell(irow, 0))
            y_val.append(bank_ws.cell(irow, 5))
            y2_val.append(x_val[irow] * difc_to_plot + tzero_to_plot)

        ws1 = CreateWorkspace(DataX=x_val,
                              DataY=y_val,
                              UnitX="Expected Peaks Centre (dSpacing A)",
                              YUnitLabel="Fitted Peaks Centre(TOF, us)")
        ws2 = CreateWorkspace(DataX=x_val, DataY=y2_val)

        output_ws = "engggui_difc_zero_peaks_bank_" + str(bank)
        if Ads.doesExist(output_ws):
            DeleteWorkspace(output_ws)

        AppendSpectra(ws1, ws2, OutputWorkspace=output_ws)
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    @staticmethod
    def _plot_difc_tzero():
        bank_1_ws = Ads.retrieve("engggui_difc_zero_peaks_bank_1")
        bank_2_ws = Ads.retrieve("engggui_difc_zero_peaks_bank_2")
        # Create plot
        fig = plt.figure()
        gs = gridspec.GridSpec(1, 2)
        plot_bank_1 = fig.add_subplot(gs[0], projection="mantid")
        plot_bank_2 = fig.add_subplot(gs[1], projection="mantid")

        for ax, ws, bank in zip([plot_bank_1, plot_bank_2], [bank_1_ws, bank_2_ws], [1, 2]):
            ax.plot(ws, wkspIndex=0, linestyle="--", marker="o", markersize="3")
            ax.plot(ws, wkspIndex=1, linestyle="--", marker="o", markersize="3")
            ax.set_title("Engg Gui Difc Zero Peaks Bank " + str(bank))
            ax.legend(("Peaks Fitted", "DifC/TZero Fitted Straight Line"))
            ax.set_xlabel("Expected Peaks Centre(dSpacing, A)")
        fig.show()

    @staticmethod
    def load_sample(sample_run_no):
        try:
            return Load(Filename=sample_run_no, OutputWorkspace="engggui_calibration_sample_ws")
        except Exception as e:
            logger.error("Error while loading calibration sample data. "
                         "Could not run the algorithm Load successfully for the calibration sample "
                         "(run number: " + str(sample_run_no) + "). Error description: " + str(e) +
                         " Please check also the previous log messages for details.")
            raise RuntimeError

    def run_calibration(self, sample_ws, van_integration, van_curves):
        """
        Runs the main Engineering calibration algorithm.
        :param sample_ws: The workspace with the sample data.
        :param van_integration: The integration values from the vanadium corrections
        :param van_curves: The curves from the vanadium corrections.
        :return: The output of the algorithm.
        """
        output = [None] * 2
        for i in range(2):
            table_name = self._generate_table_workspace_name(i)
            output[i] = EnggCalibrate(InputWorkspace=sample_ws,
                                      VanIntegrationWorkspace=van_integration,
                                      VanCurvesWorkspace=van_curves,
                                      Bank=str(i + 1),
                                      FittedPeaks=table_name)
        return output

    def create_output_files(self, calibration_dir, difc, tzero, sample_path, vanadium_path,
                            instrument):
        """
        Create output files from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param difc: DIFC values from the calibration algorithm.
        :param tzero: TZERO values from the calibration algorithm.
        :param sample_path: The path to the sample data file.
        :param vanadium_path: The path to the vanadium data file.
        :param instrument: The instrument (ENGINX or IMAT)
        """
        if not path.exists(calibration_dir):
            makedirs(calibration_dir)
        filename = self._generate_output_file_name(vanadium_path,
                                                   sample_path,
                                                   instrument,
                                                   bank="all")
        # Both Banks
        file_path = calibration_dir + filename
        write_ENGINX_GSAS_iparam_file(file_path,
                                      difc,
                                      tzero,
                                      ceria_run=sample_path,
                                      vanadium_run=vanadium_path)
        # North Bank
        file_path = calibration_dir + self._generate_output_file_name(
            vanadium_path, sample_path, instrument, bank="north")
        write_ENGINX_GSAS_iparam_file(file_path, [difc[0]], [tzero[0]],
                                      ceria_run=sample_path,
                                      vanadium_run=vanadium_path,
                                      template_file=NORTH_BANK_TEMPLATE_FILE,
                                      bank_names=["North"])
        # South Bank
        file_path = calibration_dir + self._generate_output_file_name(
            vanadium_path, sample_path, instrument, bank="south")
        write_ENGINX_GSAS_iparam_file(file_path, [difc[1]], [tzero[1]],
                                      ceria_run=sample_path,
                                      vanadium_run=vanadium_path,
                                      template_file=SOUTH_BANK_TEMPLATE_FILE,
                                      bank_names=["South"])

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
        return "engggui_calibration_bank_" + str(bank_num + 1)

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
        else:
            raise ValueError("Invalid bank name entered")
        return filename
