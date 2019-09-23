# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import AnalysisDataService as Ads
from mantid.kernel import logger
from mantid.simpleapi import Load, EnggVanadiumCorrections, EnggCalibrate, DeleteWorkspace, CloneWorkspace, \
    CreateWorkspace, AppendSpectra
from mantidqt.plotting.functions import plot
from qtpy import QtCore


class CalibrationModel(object):
    def __init__(self):
        self.VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
        self.CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
        self.INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"

    def create_new_calibration(self, vanadium_run_no, ceria_run_no, plot_output):
        vanadium_corrections = self.calculate_vanadium_correction(vanadium_run_no)
        van_integration = vanadium_corrections[0]
        van_curves = vanadium_corrections[1]
        ceria_workspace = self.load_ceria(ceria_run_no)
        difc, tzero = [0] * 2
        self.run_calibration(ceria_workspace, van_integration, van_curves, difc, tzero)
        if plot_output:
            self.plot_vanadium_curves()
            self._plot_difc_zero(difc, tzero)

    @staticmethod
    def plot_vanadium_curves():
        van_curve_twin_ws = "__engggui_vanadium_curves_twin_ws"

        if Ads.doesExist(van_curve_twin_ws):
            DeleteWorkspace(van_curve_twin_ws)
        CloneWorkspace(InputWorkspace="engggui_vanadium_curves", OutputWorkspace=van_curve_twin_ws)
        van_curves_ws = Ads.retrieve(van_curve_twin_ws)
        for i in range(1, 3):
            if i == 1:
                curve_plot_bank_1 = plot([van_curves_ws], [0, 1, 2]).activeLayer()
                curve_plot_bank_1.setTitle("Engg GUI Vanadium Curves Bank 1")
            if i == 2:
                curve_plot_bank_2 = plot([van_curves_ws], [3, 4, 5]).activeLayer()
                curve_plot_bank_2.setTitle("Engg GUI Vanadium Curves Bank 2")

    @staticmethod
    def _plot_difc_zero(difc, tzero):
        for i in range(1, 3):
            bank_ws = Ads.retrieve("engggui_calibration_bank_" + str(i))

            x_val = []
            y_val = []
            y2_val = []

            if i == 1:
                difc_to_plot = difc[0]
                tzero_to_plot = tzero[0]
            else:
                difc_to_plot = difc[1]
                tzero_to_plot = tzero[1]

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

            difc_zero_ws = Ads.retreive(output_ws)
            # Create plot
            difc_zero_plot = plot(difc_zero_ws, [0, 1]).activeLayer()
            difc_zero_plot.setTitle("Engg Gui Difc Zero Peaks Bank " + str(i))
            difc_zero_plot.setCurveTitle(0, "Peaks Fitted")
            difc_zero_plot.setCurveTitle(1, "DifC/TZero Fitted Straight Line")
            difc_zero_plot.xlabel("Expected Peaks Centre(dSpacing, A)")
            difc_zero_plot.setCurveLineStyly(0, QtCore.Qt.DotLine)

    def load_ceria(self, ceria_run_no):
        try:
            return Load(Filename=ceria_run_no, OutputWorkspace="engggui_calibration_sample_ws")
        except Exception as e:
            logger.error("Error while loading calibration sample data. "
                         "Could not run the algorithm Load succesfully for the calibration sample "
                         "(run number: " + str(ceria_run_no) + "). Error description: " + str(e) +
                         " Please check also the previous log messages for details.")
            raise RuntimeError

    def run_calibration(self, ceria_ws, van_integration, van_curves, difc, tzero):
        for i in range(2):
            table_name = self._generate_table_workspace_name(i)
            EnggCalibrate(InputWorkspace=ceria_ws,
                          VanIntegrationWorkspace=van_integration,
                          VanCurvesWorkspace=van_curves,
                          Bank=i,
                          FittedPeaks=table_name,
                          OutputParametersTableName=table_name,
                          DIFC=difc[i],
                          TZERO=tzero[i])

    def calculate_vanadium_correction(self, vanadium_run_no):
        try:
            Load(Filename=vanadium_run_no, OutputWorkspace=self.VANADIUM_INPUT_WORKSPACE_NAME)
        except Exception as e:
            logger.error("Error when loading vanadium sample data. "
                         "Could not run Load algorithm with vanadium run number: " +
                         str(vanadium_run_no) + ". Error description: " + str(e))
            raise RuntimeError
        EnggVanadiumCorrections(VanadiumWorkspace=self.VANADIUM_INPUT_WORKSPACE_NAME,
                                OutIntegrationWorkspace=self.INTEGRATED_WORKSPACE_NAME,
                                OutCurvesWorkspace=self.CURVES_WORKSPACE_NAME)
        Ads.remove(self.VANADIUM_INPUT_WORKSPACE_NAME)
        integrated_workspace = Ads.Instance().retrive(self.INTEGRATED_WORKSPACE_NAME)
        curves_workspace = Ads.Instance().retrieve(self.CURVES_WORKSPACE_NAME)
        return integrated_workspace, curves_workspace

    @staticmethod
    def _generate_table_workspace_name(bank_num):
        return "engggui_calibration_bank_" + str(bank_num + 1)
