# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from os import path, makedirs

from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger
from mantid.simpleapi import Load, EnggCalibrate, DeleteWorkspace, CloneWorkspace, \
    CreateWorkspace, AppendSpectra, CreateEmptyTableWorkspace
from mantidqt.plotting.functions import plot
from Engineering.EnggUtils import write_ENGINX_GSAS_iparam_file
from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from Engineering.gui.engineering_diffraction.tabs.common import path_handling

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CALIB_PARAMS_WORKSPACE_NAME = "engggui_calibration_banks_parameters"

CALIBRATION_DIR = path.join(path_handling.OUT_FILES_ROOT_DIR, "Calibration", "")

NORTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_North_bank.prm"
SOUTH_BANK_TEMPLATE_FILE = "template_ENGINX_241391_236516_South_bank.prm"


class CalibrationModel(object):
    def create_new_calibration(self,
                               vanadium_path,
                               ceria_path,
                               plot_output,
                               instrument,
                               rb_num=None):
        """
        Create a new calibration from a vanadium run and ceria run
        :param vanadium_path: Path to vanadium data file.
        :param ceria_path: Path to ceria data file
        :param plot_output: Whether the output should be plotted.
        :param instrument: The instrument the data relates to.
        :param rb_num: The RB number for file creation.
        """
        van_integration, van_curves = vanadium_corrections.fetch_correction_workspaces(
            vanadium_path, instrument)
        ceria_workspace = self.load_ceria(ceria_path)
        output = self.run_calibration(ceria_workspace, van_integration, van_curves)
        if plot_output:
            self._plot_vanadium_curves()
            for i in range(2):
                difc = [output[i].DIFC]
                tzero = [output[i].TZERO]
                self._plot_difc_zero(difc, tzero)
        difc = [output[0].DIFC, output[1].DIFC]
        tzero = [output[0].TZERO, output[1].TZERO]

        params_table = []
        for i in range(2):
            params_table.append([i, difc[i], 0.0, tzero[i]])
        self.update_calibration_params_table(params_table)

        self.create_output_files(CALIBRATION_DIR, difc, tzero, ceria_path, vanadium_path,
                                 instrument)
        if rb_num:
            user_calib_dir = path.join(path_handling.OUT_FILES_ROOT_DIR, "User", rb_num,
                                       "Calibration", "")
            self.create_output_files(user_calib_dir, difc, tzero, ceria_path, vanadium_path,
                                     instrument)

    @staticmethod
    def update_calibration_params_table(params_table):
        if len(params_table) == 0:
            return

        # Create blank or clear existing params table.
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
        for i in range(1, 3):
            if i == 1:
                curve_plot_bank_1 = plot([van_curves_ws], [0, 1, 2])
                curve_plot_bank_1.gca().set_title("Engg GUI Vanadium Curves Bank 1")
                curve_plot_bank_1.gca().legend(["Data", "Calc", "Diff"])
            if i == 2:
                curve_plot_bank_2 = plot([van_curves_ws], [3, 4, 5])
                curve_plot_bank_2.gca().set_title("Engg GUI Vanadium Curves Bank 2")
                curve_plot_bank_2.gca().legend(["Data", "Calc", "Diff"])

    @staticmethod
    def _plot_difc_zero(difc, tzero):
        for i in range(1, 3):
            bank_ws = Ads.retrieve(CalibrationModel._generate_table_workspace_name(i - 1))

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

            output_ws = "engggui_difc_zero_peaks_bank_" + str(i)
            if Ads.doesExist(output_ws):
                DeleteWorkspace(output_ws)

            AppendSpectra(ws1, ws2, OutputWorkspace=output_ws)
            DeleteWorkspace(ws1)
            DeleteWorkspace(ws2)

            difc_zero_ws = Ads.retrieve(output_ws)
            # Create plot
            difc_zero_plot = plot([difc_zero_ws], [0, 1],
                                  plot_kwargs={
                                      "linestyle": "--",
                                      "marker": "o",
                                      "markersize": "3"
                                  })
            difc_zero_plot.gca().set_title("Engg Gui Difc Zero Peaks Bank " + str(i))
            difc_zero_plot.gca().legend(("Peaks Fitted", "DifC/TZero Fitted Straight Line"))
            difc_zero_plot.gca().set_xlabel("Expected Peaks Centre(dSpacing, A)")

    @staticmethod
    def load_ceria(ceria_run_no):
        try:
            return Load(Filename=ceria_run_no, OutputWorkspace="engggui_calibration_sample_ws")
        except Exception as e:
            logger.error("Error while loading calibration sample data. "
                         "Could not run the algorithm Load successfully for the calibration sample "
                         "(run number: " + str(ceria_run_no) + "). Error description: " + str(e) +
                         " Please check also the previous log messages for details.")
            raise RuntimeError

    def run_calibration(self, ceria_ws, van_integration, van_curves):
        """
        Runs the main Engineering calibration algorithm.
        :param ceria_ws: The workspace with the ceria data.
        :param van_integration: The integration values from the vanadium corrections
        :param van_curves: The curves from the vanadium corrections.
        :return: The output of the algorithm.
        """
        output = [None] * 2
        for i in range(2):
            table_name = self._generate_table_workspace_name(i)
            output[i] = EnggCalibrate(InputWorkspace=ceria_ws,
                                      VanIntegrationWorkspace=van_integration,
                                      VanCurvesWorkspace=van_curves,
                                      Bank=str(i + 1),
                                      FittedPeaks=table_name,
                                      OutputParametersTableName=table_name)
        return output

    def create_output_files(self, calibration_dir, difc, tzero, ceria_path, vanadium_path,
                            instrument):
        """
        Create output files from the algorithms in the specified directory
        :param calibration_dir: The directory to save the files into.
        :param difc: DIFC values from the calibration algorithm.
        :param tzero: TZERO values from the calibration algorithm.
        :param ceria_path: The path to the ceria data file.
        :param vanadium_path: The path to the vanadium data file.
        :param instrument: The instrument (ENGINX or IMAT)
        """
        if not path.exists(calibration_dir):
            makedirs(calibration_dir)
        filename, vanadium_no, ceria_no = self._generate_output_file_name(vanadium_path,
                                                                          ceria_path,
                                                                          instrument,
                                                                          bank="all")
        # Both Banks
        file_path = calibration_dir + filename
        write_ENGINX_GSAS_iparam_file(file_path,
                                      difc,
                                      tzero,
                                      ceria_run=ceria_no,
                                      vanadium_run=vanadium_no)
        # North Bank
        file_path = calibration_dir + self._generate_output_file_name(
            vanadium_path, ceria_path, instrument, bank="north")[0]
        write_ENGINX_GSAS_iparam_file(file_path, [difc[0]], [tzero[0]],
                                      ceria_run=ceria_no,
                                      vanadium_run=vanadium_no,
                                      template_file=NORTH_BANK_TEMPLATE_FILE,
                                      bank_names=["North"])
        # South Bank
        file_path = calibration_dir + self._generate_output_file_name(
            vanadium_path, ceria_path, instrument, bank="south")[0]
        write_ENGINX_GSAS_iparam_file(file_path, [difc[1]], [tzero[1]],
                                      ceria_run=ceria_no,
                                      vanadium_run=vanadium_no,
                                      template_file=SOUTH_BANK_TEMPLATE_FILE,
                                      bank_names=["South"])

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num + 1)

    @staticmethod
    def _generate_output_file_name(vanadium_path, ceria_path, instrument, bank):
        """
        Generate an output filename in the form INSTRUMENT_VanadiumRunNo_CeriaRunCo_BANKS
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
            filename = filename + "all_banks.prm"
        elif bank == "north":
            filename = filename + "bank_North.prm"
        elif bank == "south":
            filename = filename + "bank_South.prm"
        else:
            raise ValueError("Invalid bank name entered")
        return filename, vanadium_no, ceria_no
