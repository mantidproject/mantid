#pylint: disable=invalid-name
from Ui_MainWindow import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
import math

class MainWindow(QtGui.QMainWindow):
    needsThetaInputList = ['Momentum transfer (Q Angstroms^-1)', 'd-Spacing (Angstroms)']
    needsThetaOutputList = ['Momentum transfer (Q Angstroms^-1)', 'd-Spacing (Angstroms)']
    needsFlightPathInputList = ['Time of flight (microseconds)']
    needsFlightPathOutputList = ['Time of flight (microseconds)']

    def thetaEnable (self, enabled):
        self.ui.lineEdit_4.setEnabled(enabled)
        if  enabled == False:
            self.ui.lineEdit_4.clear()

    def flightPathEnable (self, enabled):
        self.ui.lineEdit_3.setEnabled(enabled)
        if  enabled == False:
            self.ui.lineEdit_3.clear()

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
        self.ui.lineEdit_3.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_3))
        self.ui.lineEdit_4.setValidator(QtGui.QDoubleValidator(self.ui.lineEdit_4))
        QtCore.QObject.connect(self.ui.convert, QtCore.SIGNAL("clicked()"), self.convert )
        QtCore.QObject.connect(self.ui.inputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        QtCore.QObject.connect(self.ui.outputUnits, QtCore.SIGNAL("currentIndexChanged(QString)"), self.setInstrumentInputs )
        self.setInstrumentInputs()

        ##defaults
        self.flightpath = -1.0
        self.Theta = -1.0
        self.stage1output = 0.0
        self.stage2output = 0.0

    def convert(self):
        if self.ui.InputVal.text() == "":
            return
        try:
            inOption = self.ui.inputUnits.currentText()
            outOption = self.ui.outputUnits.currentText()
            if self.ui.lineEdit_3.text() !='':
                self.flightpath = float(self.ui.lineEdit_3.text())
            else:
                self.flightpath = -1.0
            if self.ui.lineEdit_4.text() !='':
                self.Theta = float(self.ui.lineEdit_4.text()) * math.pi / 360.0
            self.stage1output = self.input2energy(float(self.ui.InputVal.text()), inOption)
            self.stage2output = self.energy2output(self.stage1output,outOption)

            self.ui.lineEdit_2.clear()
            self.ui.lineEdit_2.insert(str(self.stage2output))
        except Exception, e:
            QtGui.QMessageBox.warning(self, "TofConverter", str(e))
            return

    def input2energy(self, inputval, inOption):
        e2lam = 81.787
        e2nu = 4.139
        e2v = 0.0000052276
        e2k = 2.717
        e2t = 0.086165
        e2cm = 0.123975
        iv2 = inputval ** 2

        if inOption == 'Wavelength (Angstroms)':
            Energy = e2lam / iv2

        elif inOption == 'Energy  (meV)':
            Energy = inputval

        elif inOption == 'Nu (THz)':
            Energy = e2nu * inputval

        elif inOption == 'Velocity (m/s)':
            Energy = e2v *iv2

        elif inOption == 'Momentum ( k Angstroms^-1)':
            Energy = e2k*iv2

        elif inOption == 'Temperature (K)':
            Energy = e2t *inputval

        elif inOption == 'Energy (cm^-1)':
            Energy = e2cm * inputval

        elif inOption == 'Momentum transfer (Q Angstroms^-1)':
            if self.Theta >= 0.0:
                k = inputval * 0.5 / math.sin(self.Theta)
                Energy = e2k * k * k
            else:
                raise RuntimeError("Theta > 0 is required for conversion from Q")

        elif inOption == 'd-Spacing (Angstroms)':
            lam = 2 * inputval * math.sin(self.Theta)
            Energy = e2lam / (lam * lam)

        elif  inOption == 'Time of flight (microseconds)':
            if self.flightpath >= 0.0:
                Energy = 1000000 * self.flightpath
                Energy = e2v * Energy *Energy / iv2
            else:
                raise RuntimeError("Flight path >= 0 is required for conversion from TOF")

        return Energy

    def energy2output(self, Energy, inOption):
        e2lam = 81.787
        e2nu = 4.139
        e2v = 0.0000052276
        e2k = 2.0717
        e2t = 0.086165
        e2cm = 0.123975
        iv2 = Energy ** 2

        if inOption == 'Wavelength (Angstroms)':
            OutputVal =  (e2lam/ Energy)**0.5

        elif inOption == 'Nu (THz)':
            OutputVal = Energy / e2nu

        elif inOption == 'Velocity (m/s)':
            OutputVal = (Energy / e2v)**0.5

        elif inOption == 'Momentum ( k Angstroms^-1)':
            OutputVal = (Energy / e2k)**0.5

        elif inOption == 'Temperature (K)':
            OutputVal = Energy / e2t

        elif inOption == 'Energy (cm^-1)':
            OutputVal = Energy / e2cm

        elif inOption == 'Momentum transfer (Q Angstroms^-1)':
            if self.Theta >= 0.0:
                k = (Energy / e2k) ** 0.5
                OutputVal = 2 * k * math.sin(self.Theta)
            else:
                raise RuntimeError("Theta > 0 is required for conversion to Q")

        elif inOption == 'd-Spacing (Angstroms)':
            if self.Theta >= 0.0:
                lam = (e2lam / Energy)**0.5
                OutputVal = lam * 0.5 / math.sin(self.Theta)
            else:
                raise RuntimeError("Theta > 0 is required for conversion to d-Spacing")

        elif inOption == 'Time of flight (microseconds)':
            if self.flightpath >= 0.0:
                OutputVal = self.flightpath * 1000 * ((e2v * 1000000 / Energy) ** 0.5)
            else:
                raise RuntimeError("Flight path >= 0 is required for conversion to TOF")

        elif inOption == 'Energy  (meV)':
            OutputVal = Energy

        return OutputVal
