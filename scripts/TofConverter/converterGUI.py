# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import QMainWindow, QMessageBox
from qtpy.QtGui import QDoubleValidator
from qtpy import QtCore
import os
from mantid.kernel import Logger
from mantidqt.gui_helper import show_interface_help
import math
import TofConverter.convertUnits


try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("TofConverter").information('Using legacy ui importer')
    from mantidplot import load_ui


class MainWindow(QMainWindow):
    needsThetaInputList = ['Momentum transfer (Q Angstroms^-1)', 'd-spacing (Angstroms)']
    needsThetaOutputList = ['Momentum transfer (Q Angstroms^-1)', 'd-spacing (Angstroms)']
    needsFlightPathInputList = ['Time of flight (microseconds)']
    needsFlightPathOutputList = ['Time of flight (microseconds)']

    def thetaEnable (self, enabled):
        self.ui.scatteringAngleInput.setEnabled(enabled)
        if  not enabled:
            self.ui.scatteringAngleInput.clear()

    def flightPathEnable (self, enabled):
        self.ui.totalFlightPathInput.setEnabled(enabled)
        if  not enabled:
            self.ui.totalFlightPathInput.clear()

    def setInstrumentInputs (self):
        #disable both
        self.thetaEnable(False)
        self.flightPathEnable(False)

        #get the values of the two unit strings
        inOption=self.ui.inputUnits.currentText()
        outOption=self.ui.outputUnits.currentText()

        #for theta: enable if input or output unit requires it
        if inOption in self.needsThetaInputList:
            self.thetaEnable(True)

        if outOption in self.needsThetaOutputList:
            self.thetaEnable(True)

        #for flightpath: enable if input or output unit requires it
        if inOption in self.needsFlightPathInputList:
            self.flightPathEnable(True)

        if outOption in self.needsFlightPathOutputList:
            self.flightPathEnable(True)

    def __init__(self, parent=None):
        QMainWindow.__init__(self,parent)
        self.ui = load_ui(__file__, 'converter.ui', baseinstance=self)
        self.ui.InputVal.setValidator(QDoubleValidator(self.ui.InputVal))
        self.ui.totalFlightPathInput.setValidator(QDoubleValidator(self.ui.totalFlightPathInput))
        self.ui.scatteringAngleInput.setValidator(QDoubleValidator(self.ui.scatteringAngleInput))
        self.ui.convertButton.clicked.connect(self.convert)
        self.ui.helpButton.clicked.connect(self.helpClicked)
        self.ui.inputUnits.currentIndexChanged.connect(self.setInstrumentInputs)
        self.ui.outputUnits.currentIndexChanged.connect(self.setInstrumentInputs)
        self.setInstrumentInputs()

        ##defaults
        self.flightpath = -1.0
        self.Theta = -1.0
        self.output = 0.0

        #help
        self.assistant_process = QtCore.QProcess(self)
        # pylint: disable=protected-access
        import mantid
        self.mantidplot_name='TOF Converter'
        self.collection_file = os.path.join(mantid._bindir, '../docs/qthelp/MantidProject.qhc')
        version = ".".join(mantid.__version__.split(".")[:2])
        self.qt_url = 'qthelp://org.sphinx.mantidproject.' + version + '/doc/interfaces/TOF Converter.html'
        self.external_url = 'http://docs.mantidproject.org/nightly/interfaces/TOF Converter.html'

        try:
            import mantid
            #register startup
            mantid.UsageService.registerFeatureUsage("Interface","TofConverter",False)
        except ImportError:
            pass

    def helpClicked(self):
        show_interface_help(self.mantidplot_name,
                            self.assistant_process,
                            self.collection_file,
                            self.qt_url,
                            self.external_url)

    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()

    def convert(self):
        #Always reset these values before conversion.
        self.Theta = None
        self.flightpath = None
        try:
            if self.ui.InputVal.text() == "":
                raise RuntimeError("Input value is required for conversion")
            if float(self.ui.InputVal.text()) <= 0:
                raise RuntimeError("Input value must be greater than 0 for conversion")
            inOption = self.ui.inputUnits.currentText()
            outOption = self.ui.outputUnits.currentText()
            if self.ui.totalFlightPathInput.text() !='':
                self.flightpath = float(self.ui.totalFlightPathInput.text())
            else:
                self.flightpath = -1.0
            if self.ui.scatteringAngleInput.text() !='':
                self.Theta = float(self.ui.scatteringAngleInput.text()) * math.pi / 360.0

            self.output = TofConverter.convertUnits.doConversion(self.ui.InputVal.text(), inOption, outOption, self.Theta, self.flightpath)

            self.ui.convertedVal.clear()
            self.ui.convertedVal.insert(str(self.output))
        except UnboundLocalError as ule:
            QMessageBox.warning(self, "TofConverter", str(ule))
            return
        except ArithmeticError as ae:
            QMessageBox.warning(self, "TofConverter", str(ae))
            return
        except RuntimeError as re:
            QMessageBox.warning(self, "TofConverter", str(re))
            return
