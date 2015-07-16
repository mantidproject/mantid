#pylint: disable=invalid-name
from TofConverter.ui_converter import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui
import math

class MainWindow(QtGui.QMainWindow):
    needsThetaInputList = ['Q Angstroms^-1 (momentum transfer)', 'Angstroms (d-spacing)']
    needsThetaOutputList = ['Q Angstroms^-1 (momentum transfer)', 'Angstroms (d-spacing)']
    needsFlightPathInputList = ['microseconds (time of flight)']
    needsFlightPathOutputList = ['microseconds (time of flight)']

    def thetaEnable (self, enabled):
        self.ui.scatteringAngleInput.setEnabled(enabled)
        if  enabled == False:
            self.ui.scatteringAngleInput.clear()

    def flightPathEnable (self, enabled):
        self.ui.totalFlightPathInput.setEnabled(enabled)
        if  enabled == False:
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
        self.stage1output = 0.0
        self.stage2output = 0.0

    def helpClicked(self):
	    # Temporary import while method is in the wrong place
        from pymantidplot.proxies import showCustomInterfaceHelp
        showCustomInterfaceHelp("TOF_Converter") #need to find a way to import this module
    
	def convert(self):
        if self.ui.InputVal.text() == "":
            return
        try:
            inOption = self.ui.inputUnits.currentText()
            outOption = self.ui.outputUnits.currentText()
            if self.ui.totalFlightPathInput.text() !='':
                self.flightpath = float(self.ui.totalFlightPathInput.text())
            else:
                self.flightpath = -1.0
            if self.ui.scatteringAngleInput.text() !='':
                self.Theta = float(self.ui.scatteringAngleInput.text()) * math.pi / 360.0
            self.stage1output = self.input2energy(float(self.ui.InputVal.text()), inOption)
            self.stage2output = self.energy2output(self.stage1output,outOption)

            self.ui.convertedVal.clear()
            self.ui.convertedVal.insert(str(self.stage2output))
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

        if inOption == 'Angstroms (wavelength)':
            Energy = e2lam / iv2

        elif inOption == 'meV (energy)':
            Energy = inputval

        elif inOption == 'THz (nu)':
            Energy = e2nu * inputval

        elif inOption == 'm/s (velocity)':
            Energy = e2v *iv2

        elif inOption == 'k Angstroms^-1 (momentum)':
            Energy = e2k*iv2

        elif inOption == 'K (temperature)':
            Energy = e2t *inputval

        elif inOption == 'cm^-1 (energy)':
            Energy = e2cm * inputval

        elif inOption == 'Q Angstroms^-1 (momentum transfer)':
            if self.Theta >= 0.0:
                k = inputval * 0.5 / math.sin(self.Theta)
                Energy = e2k * k * k
            else:
                raise RuntimeError("Theta > 0 is required for conversion from Q")

        elif inOption == 'Angstroms (d-spacing)':
            lam = 2 * inputval * math.sin(self.Theta)
            Energy = e2lam / (lam * lam)

        elif  inOption == 'microseconds (time of flight)':
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

        if inOption == 'Angstroms (wavelength)':
            OutputVal =  (e2lam/ Energy)**0.5

        elif inOption == 'THz (nu)':
            OutputVal = Energy / e2nu

        elif inOption == 'm/s (velocity)':
            OutputVal = (Energy / e2v)**0.5

        elif inOption == 'k Angstroms^-1 (momentum)':
            OutputVal = (Energy / e2k)**0.5

        elif inOption == 'K (temperature)':
            OutputVal = Energy / e2t

        elif inOption == 'cm^-1 (energy)':
            OutputVal = Energy / e2cm

        elif inOption == 'Q Angstroms^-1 (momentum transfer)':
            if self.Theta >= 0.0:
                k = (Energy / e2k) ** 0.5
                OutputVal = 2 * k * math.sin(self.Theta)
            else:
                raise RuntimeError("Theta > 0 is required for conversion to Q")

        elif inOption == 'Angstroms (d-spacing)':
            if self.Theta >= 0.0:
                lam = (e2lam / Energy)**0.5
                OutputVal = lam * 0.5 / math.sin(self.Theta)
            else:
                raise RuntimeError("Theta > 0 is required for conversion to d-Spacing")

        elif inOption == 'microseconds (time of flight)':
            if self.flightpath >= 0.0:
                OutputVal = self.flightpath * 1000 * ((e2v * 1000000 / Energy) ** 0.5)
            else:
                raise RuntimeError("Flight path >= 0 is required for conversion to TOF")

        elif inOption == 'meV (energy)':
            OutputVal = Energy

        return OutputVal
