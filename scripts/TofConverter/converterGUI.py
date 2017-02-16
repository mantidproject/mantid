#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from .ui_converter import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
import math
import TofConverter.convertUnits


class MainWindow(QtGui.QMainWindow):
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
        QtGui.QMainWindow.__init__(self,parent)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.InputVal.setValidator(QtGui.QDoubleValidator(self.ui.InputVal))
        self.ui.totalFlightPathInput.setValidator(QtGui.QDoubleValidator(self.ui.totalFlightPathInput))
        self.ui.scatteringAngleInput.setValidator(QtGui.QDoubleValidator(self.ui.scatteringAngleInput))
        QtCore.QObject.connect(self.ui.convert, QtCore.SIGNAL("clicked()"), self.convert )
        QtCore.QObject.connect(self.ui.helpButton, QtCore.SIGNAL("clicked()"), self.helpClicked)
        QtCore.QObject.connect(self.ui.inputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        QtCore.QObject.connect(self.ui.outputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        self.setInstrumentInputs()

        ##defaults
        self.flightpath = -1.0
        self.Theta = -1.0
        self.output = 0.0

        try:
            import mantid
            #register startup
            mantid.UsageService.registerFeatureUsage("Interface","TofConverter",False)
        except ImportError:
            pass

    def helpClicked(self):
        # Temporary import while method is in the wrong place
        from pymantidplot.proxies import showCustomInterfaceHelp
        showCustomInterfaceHelp("TOF_Converter")

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
            QtGui.QMessageBox.warning(self, "TofConverter", str(ule))
            return
        except ArithmeticError as ae:
            QtGui.QMessageBox.warning(self, "TofConverter", str(ae))
            return
        except RuntimeError as re:
            QtGui.QMessageBox.warning(self, "TofConverter", str(re))
            return
