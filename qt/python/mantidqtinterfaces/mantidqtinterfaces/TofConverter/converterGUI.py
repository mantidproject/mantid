# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from qtpy.QtWidgets import QMainWindow, QMessageBox
from qtpy.QtGui import QDoubleValidator
from qtpy import QtCore
from mantid.dataobjects import WorkspaceSingleValue
from mantid.kernel import Logger
from mantid.simpleapi import AnalysisDataService
from mantidqt.gui_helper import show_interface_help
from mantidqt.utils.qt import load_ui
import math
from mantidqtinterfaces.TofConverter import convertUnits
from typing import Dict


class MainWindow(QMainWindow):
    needsThetaInputList = [convertUnits.MOMENTUM_TRANSFER, convertUnits.D_SPACING]
    needsThetaOutputList = [convertUnits.MOMENTUM_TRANSFER, convertUnits.D_SPACING]
    needsFlightPathInputList = [convertUnits.TIME_OF_FLIGHT]
    needsFlightPathOutputList = [convertUnits.TIME_OF_FLIGHT]

    def thetaEnable(self, enabled):
        self.ui.scatteringAngleInput.setEnabled(enabled)
        if not enabled:
            self.ui.scatteringAngleInput.clear()

    def flightPathEnable(self, enabled):
        self.ui.totalFlightPathInput.setEnabled(enabled)
        if not enabled:
            self.ui.totalFlightPathInput.clear()

    def setInstrumentInputs(self):
        # get the values of the two unit strings
        inOption = self.ui.inputUnits.currentText()
        outOption = self.ui.outputUnits.currentText()

        if inOption == outOption:
            self.thetaEnable(False)
            self.flightPathEnable(False)
            self.ui.convertButton.setEnabled(False)
            self.ui.convertedVal.setEnabled(False)
            self.ui.convertedVal.setText("Input and output units are the same")
        else:
            self.ui.convertButton.setEnabled(True)
            self.ui.convertedVal.setEnabled(True)
            self.ui.convertedVal.clear()
            # for theta: enable if input or output unit requires it
            self.thetaEnable(
                not convertUnits.is_momentum_dspacing_transform(inOption, outOption)
                and (inOption in self.needsThetaInputList or outOption in self.needsThetaOutputList)
            )
            # for flightpath: enable if input or output unit requires it
            self.flightPathEnable(inOption in self.needsFlightPathInputList or outOption in self.needsFlightPathOutputList)

    def __init__(self, parent=None, window_flags=None):
        QMainWindow.__init__(self, parent)
        if window_flags:
            self.setWindowFlags(window_flags)
        self.ui = load_ui(__file__, "converter.ui", baseinstance=self)
        self.ui.InputVal.setValidator(QDoubleValidator(self.ui.InputVal))
        self.ui.InputVal.editingFinished.connect(self.input_val_editing_finished)
        self.ui.totalFlightPathInput.setValidator(QDoubleValidator(self.ui.totalFlightPathInput))
        self.ui.scatteringAngleInput.setValidator(QDoubleValidator(self.ui.scatteringAngleInput))
        self.ui.convertButton.clicked.connect(self.convert)
        self.ui.helpButton.clicked.connect(self.helpClicked)
        self.ui.inputUnits.currentIndexChanged.connect(self.setInstrumentInputs)
        self.ui.outputUnits.currentIndexChanged.connect(self.setInstrumentInputs)
        self.ui.inputWorkspace.currentTextChanged.connect(self.input_option_changed)
        self.setInstrumentInputs()

        ##defaults
        self.flightpath = -1.0
        self.Theta = -1.0
        self.output = 0.0

        # help
        self.assistant_process = QtCore.QProcess(self)
        # pylint: disable=protected-access
        self.mantidplot_name = "TOF Converter"

        # Add combo box options
        self.ui.inputUnits.addItems(convertUnits.UNIT_LIST)
        self.ui.outputUnits.addItems(convertUnits.UNIT_LIST)
        self.input_val_dict = self.get_all_single_value_workspaces()
        self.input_val_dict["User specified"] = 0
        self.ui.inputWorkspace.addItems(list(self.input_val_dict.keys()))

        try:
            import mantid

            # register startup
            mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "TofConverter", False)
        except ImportError:
            pass

    @staticmethod
    def get_all_single_value_workspaces() -> Dict[str, float]:
        single_value_workspaces = {}
        ws_names = AnalysisDataService.getObjectNames()
        if len(ws_names) == 0:
            return single_value_workspaces

        ws_list = AnalysisDataService.retrieveWorkspaces(ws_names)
        for ws in ws_list:
            if isinstance(ws, WorkspaceSingleValue):
                single_value_workspaces[ws.name()] = ws.readY(0)[0]
        return single_value_workspaces

    def helpClicked(self):
        show_interface_help(self.mantidplot_name, self.assistant_process, area="utility")

    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()

    def input_option_changed(self, input_option: str):
        if input_option in self.input_val_dict:
            self.ui.InputVal.setText(str(self.input_val_dict[input_option]))

        # Don't allow alteration of the input value if it's from a workspace
        self.ui.InputVal.setEnabled(input_option == "User specified")

    def input_val_editing_finished(self):
        if self.ui.inputWorkspace.currentText() == "User specified":
            try:
                self.input_val_dict["User specified"] = float(self.ui.InputVal.text())
            except ValueError:
                pass

    def convert(self):
        # Always reset these values before conversion.
        self.Theta = None
        self.flightpath = None
        try:
            if self.ui.InputVal.text() == "":
                raise RuntimeError("Input value is required for conversion")
            if float(self.ui.InputVal.text()) <= 0:
                raise RuntimeError("Input value must be greater than 0 for conversion")
            inOption = self.ui.inputUnits.currentText()
            outOption = self.ui.outputUnits.currentText()
            if self.ui.totalFlightPathInput.text():
                self.flightpath = float(self.ui.totalFlightPathInput.text())
            else:
                self.flightpath = -1.0
            if self.ui.scatteringAngleInput.text():
                # This Theta is the Bragg scattering angle, which is half the angle from the spherical coordinate system
                # directed along the Mantid z-axis. See https://docs.mantidproject.org/nightly/concepts/UnitFactory.html.
                # That's why there's a factor of 0.5 when converting to radians.
                normalised_angle = (float(self.ui.scatteringAngleInput.text()) + 360) % 360
                self.Theta = normalised_angle * math.pi / 360.0
            else:
                self.Theta = -1.0

            self.output = convertUnits.doConversion(self.ui.InputVal.text(), inOption, outOption, self.Theta, self.flightpath)

            self.ui.convertedVal.clear()
            self.ui.convertedVal.insert(str(self.output))
        except (UnboundLocalError, ArithmeticError, ValueError, RuntimeError) as err:
            QMessageBox.warning(self, "TofConverter", str(err))
            return
        except Exception as exc:
            Logger.error(exc)
            return
