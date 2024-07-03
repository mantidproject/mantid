# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-name-in-module,too-many-instance-attributes,super-on-old-class,too-few-public-methods
from qtpy import QtGui, QtCore, QtWidgets
import sys
import numpy

try:
    from qtpy.QtCore import QString
except ImportError:
    QString = type("")


def returnValid(validity, teststring, pos):
    if QString is str:
        return (validity, teststring, pos)
    else:
        return (validity, pos)


class EmptyOrDoubleValidator(QtGui.QValidator):
    def __init__(self, dummy_parent):
        super(EmptyOrDoubleValidator, self).__init__()

    def validate(self, teststring, pos):
        if len(str(teststring)) == 0:
            return returnValid(QtGui.QValidator.Acceptable, teststring, pos)
        else:
            try:
                float(str(teststring))
                return returnValid(QtGui.QValidator.Acceptable, teststring, pos)
            except ValueError:
                try:
                    # this is the case when you start typing - or 1e or 1e- and putting 1 at the end would make it a float
                    float(str(teststring) + "1")
                    return returnValid(QtGui.QValidator.Intermediate, teststring, pos)
                except ValueError:
                    return returnValid(QtGui.QValidator.Invalid, teststring, pos)


class V3DValidator(QtGui.QValidator):
    def __init__(self, dummy_parent):
        super(V3DValidator, self).__init__()

    def validate(self, teststring, pos):
        parts = str(teststring).split(",")
        if len(parts) > 3:
            return returnValid(QtGui.QValidator.Invalid, teststring, pos)
        if len(parts) == 3:
            try:
                float(parts[0])
                float(parts[1])
                float(parts[2])
                return returnValid(QtGui.QValidator.Acceptable, teststring, pos)
            except ValueError:
                try:
                    float(parts[0] + "1")
                    float(parts[1] + "1")
                    float(parts[2] + "1")
                    return returnValid(QtGui.QValidator.Intermediate, teststring, pos)
                except ValueError:
                    return returnValid(QtGui.QValidator.Invalid, teststring, pos)
        return returnValid(QtGui.QValidator.Intermediate, teststring, pos)


def FloatToQString(value):
    if numpy.isfinite(value):
        return QString(format(value, ".3f"))
    else:
        return QString("")


def translation(number, character):
    if number == 0:
        return "0"
    if number == 1:
        return character
    if number == -1:
        return "-" + character
    return str(number) + character


class DimensionSelectorWidget(QtWidgets.QWidget):
    changed = QtCore.Signal(dict)

    def __init__(self, parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(DimensionSelectorWidget, self).__init__()
        # validators
        self.doubleValidator = EmptyOrDoubleValidator(self)
        self.positiveDoubleValidator = QtGui.QDoubleValidator(self)
        self.positiveDoubleValidator.setBottom(1e-10)
        self.V3DValidator = V3DValidator(self)
        # labels
        self._labelBasis = QtWidgets.QLabel("    Projection Basis")
        self._labelBasis1 = QtWidgets.QLabel("    Projection u")
        self._labelBasis2 = QtWidgets.QLabel("    Projection v")
        self._labelBasis3 = QtWidgets.QLabel("    Projection w")
        self._labelMin = QtWidgets.QLabel("Min")
        self._labelMax = QtWidgets.QLabel("Max")
        self._labelStep = QtWidgets.QLabel("Step")
        # lineedits
        self._editBasis1 = QtWidgets.QLineEdit()
        self._editBasis1.setValidator(self.V3DValidator)
        self._editBasis2 = QtWidgets.QLineEdit()
        self._editBasis2.setValidator(self.V3DValidator)
        self._editBasis3 = QtWidgets.QLineEdit()
        self._editBasis3.setValidator(self.V3DValidator)
        self._editMin1 = QtWidgets.QLineEdit()
        self._editMin1.setValidator(self.doubleValidator)
        self._editMin2 = QtWidgets.QLineEdit()
        self._editMin2.setValidator(self.doubleValidator)
        self._editMin3 = QtWidgets.QLineEdit()
        self._editMin3.setValidator(self.doubleValidator)
        self._editMin4 = QtWidgets.QLineEdit()
        self._editMin4.setValidator(self.doubleValidator)
        self._editMax1 = QtWidgets.QLineEdit()
        self._editMax1.setValidator(self.doubleValidator)
        self._editMax2 = QtWidgets.QLineEdit()
        self._editMax2.setValidator(self.doubleValidator)
        self._editMax3 = QtWidgets.QLineEdit()
        self._editMax3.setValidator(self.doubleValidator)
        self._editMax4 = QtWidgets.QLineEdit()
        self._editMax4.setValidator(self.doubleValidator)
        self._editStep1 = QtWidgets.QLineEdit()
        self._editStep1.setValidator(self.positiveDoubleValidator)
        self._editStep2 = QtWidgets.QLineEdit()
        self._editStep2.setValidator(self.positiveDoubleValidator)
        self._comboDim1 = QtWidgets.QComboBox(self)
        self._comboDim2 = QtWidgets.QComboBox(self)
        self._comboDim3 = QtWidgets.QComboBox(self)
        self._comboDim4 = QtWidgets.QComboBox(self)
        self._comboDim4.setMinimumContentsLength(12)
        # basis
        self.basis = ["1,0,0", "0,1,0", "0,0,1"]
        # default values
        self.dimNames = ["[H,0,0]", "[0,K,0]", "[0,0,L]", "DeltaE"]

        self.dimMin = [-numpy.inf, -numpy.inf, -numpy.inf, -numpy.inf]
        self.dimMax = [numpy.inf, numpy.inf, numpy.inf, numpy.inf]
        self.dimStep = [0.05, 0.05, 0.05, 1]
        self.dimIndex = [0, 1, 2, 3]
        # layout
        grid = QtWidgets.QGridLayout()
        self.setLayout(grid)
        grid.addWidget(self._labelMin, 0, 1)
        grid.addWidget(self._labelMax, 0, 2)
        grid.addWidget(self._labelStep, 0, 3)
        grid.addWidget(self._labelBasis, 0, 4, 1, 2)
        grid.addWidget(self._comboDim1, 1, 0)
        grid.addWidget(self._editMin1, 1, 1)
        grid.addWidget(self._editMax1, 1, 2)
        grid.addWidget(self._editStep1, 1, 3)
        grid.addWidget(self._labelBasis1, 1, 4)
        grid.addWidget(self._editBasis1, 1, 5)
        grid.addWidget(self._comboDim2, 2, 0)
        grid.addWidget(self._editMin2, 2, 1)
        grid.addWidget(self._editMax2, 2, 2)
        grid.addWidget(self._editStep2, 2, 3)
        grid.addWidget(self._labelBasis2, 2, 4)
        grid.addWidget(self._editBasis2, 2, 5)
        grid.addWidget(self._comboDim3, 3, 0)
        grid.addWidget(self._editMin3, 3, 1)
        grid.addWidget(self._editMax3, 3, 2)
        grid.addWidget(self._labelBasis3, 3, 4)
        grid.addWidget(self._editBasis3, 3, 5)
        grid.addWidget(self._comboDim4, 4, 0)
        grid.addWidget(self._editMin4, 4, 1)
        grid.addWidget(self._editMax4, 4, 2)

        self._editBasis1.setText(QString(self.basis[0]))
        self._editBasis2.setText(QString(self.basis[1]))
        self._editBasis3.setText(QString(self.basis[2]))
        self.updateCombo()
        self.updateGui()
        # connections
        self._editBasis1.textEdited.connect(self.basisChanged)
        self._editBasis2.textEdited.connect(self.basisChanged)
        self._editBasis3.textEdited.connect(self.basisChanged)
        self._editMin1.textEdited.connect(self.limitsChanged)
        self._editMin2.textEdited.connect(self.limitsChanged)
        self._editMin3.textEdited.connect(self.limitsChanged)
        self._editMin4.textEdited.connect(self.limitsChanged)
        self._editMax1.textEdited.connect(self.limitsChanged)
        self._editMax2.textEdited.connect(self.limitsChanged)
        self._editMax3.textEdited.connect(self.limitsChanged)
        self._editMax4.textEdited.connect(self.limitsChanged)
        self._editStep1.textEdited.connect(self.stepChanged)
        self._editStep2.textEdited.connect(self.stepChanged)
        self._comboDim1.currentIndexChanged.connect(self.comboChanged)
        self._comboDim2.currentIndexChanged.connect(self.comboChanged)
        self._comboDim3.currentIndexChanged.connect(self.comboChanged)
        self._comboDim4.currentIndexChanged.connect(self.comboChanged)
        self.inhibitSignal = False
        self.updateChanges()

    def comboChanged(self, idx):
        if self.inhibitSignal:
            return
        senderList = [self._comboDim1, self._comboDim2, self._comboDim3, self._comboDim4]
        senderIndex = senderList.index(self.sender())
        # swap names, mins, maxes and steps
        self.dimNames[idx], self.dimNames[senderIndex] = self.dimNames[senderIndex], self.dimNames[idx]
        self.dimMin[idx], self.dimMin[senderIndex] = self.dimMin[senderIndex], self.dimMin[idx]
        self.dimMax[idx], self.dimMax[senderIndex] = self.dimMax[senderIndex], self.dimMax[idx]
        self.dimStep[idx], self.dimStep[senderIndex] = self.dimStep[senderIndex], self.dimStep[idx]
        self.dimIndex[idx], self.dimIndex[senderIndex] = self.dimIndex[senderIndex], self.dimIndex[idx]
        self.inhibitSignal = True
        self.updateCombo()
        self.updateGui()
        self.inhibitSignal = False
        self.updateChanges()

    def stepChanged(self):
        sender = self.sender()
        validator = sender.validator()
        state = validator.validate(sender.text(), 0)[0]
        if state == QtGui.QValidator.Acceptable:
            color = "#ffffff"
        else:
            color = "#ff0000"
        sender.setStyleSheet("QLineEdit { background-color: %s }" % color)
        if state == QtGui.QValidator.Acceptable:
            if sender == self._editStep1:
                self.dimStep[0] = float(sender.text())
            else:
                self.dimStep[1] = float(sender.text())
            self.updateChanges()

    def limitsChanged(self):
        minSenders = [self._editMin1, self._editMin2, self._editMin3, self._editMin4]
        maxSenders = [self._editMax1, self._editMax2, self._editMax3, self._editMax4]
        sender = self.sender()
        if sender in minSenders:
            senderIndex = minSenders.index(sender)
            if len(str(sender.text()).strip()) == 0:
                self.dimMin[senderIndex] = -numpy.inf
                color = "#ffffff"
            else:
                try:
                    tempvalue = float(sender.text())
                    self.dimMin[senderIndex] = tempvalue
                    if tempvalue < self.dimMax[senderIndex]:
                        color = "#ffffff"
                    else:
                        color = "#ff0000"
                except ValueError:
                    color = "#ff0000"
        if sender in maxSenders:
            senderIndex = maxSenders.index(sender)
            if len(str(sender.text()).strip()) == 0:
                self.dimMax[senderIndex] = numpy.inf
                color = "#ffffff"
            else:
                try:
                    tempvalue = float(sender.text())
                    self.dimMax[senderIndex] = tempvalue
                    if tempvalue > self.dimMin[senderIndex]:
                        color = "#ffffff"
                    else:
                        color = "#ff0000"
                except ValueError:
                    color = "#ff0000"
        minSenders[senderIndex].setStyleSheet("QLineEdit { background-color: %s }" % color)
        maxSenders[senderIndex].setStyleSheet("QLineEdit { background-color: %s }" % color)
        if color == "#ffffff":
            self.updateChanges()

    def basisChanged(self):
        sender = self.sender()
        validator = sender.validator()
        state = validator.validate(sender.text(), 0)[0]
        if state == QtGui.QValidator.Acceptable:
            color = "#ffffff"
        elif state == QtGui.QValidator.Intermediate:
            color = "#ffaaaa"
        else:
            color = "#ff0000"
        sender.setStyleSheet("QLineEdit { background-color: %s }" % color)
        if state == QtGui.QValidator.Acceptable:
            self.validateBasis()

    def validateBasis(self):
        color = "#ff0000"
        if (
            self._editBasis1.validator().validate(self._editBasis1.text(), 0)[0] == QtGui.QValidator.Acceptable
            and self._editBasis2.validator().validate(self._editBasis2.text(), 0)[0] == QtGui.QValidator.Acceptable
            and self._editBasis3.validator().validate(self._editBasis3.text(), 0)[0] == QtGui.QValidator.Acceptable
        ):
            b1 = numpy.fromstring(str(self._editBasis1.text()), sep=",")
            b2 = numpy.fromstring(str(self._editBasis2.text()), sep=",")
            b3 = numpy.fromstring(str(self._editBasis3.text()), sep=",")
            if numpy.abs(numpy.inner(b1, numpy.cross(b2, b3))) > 1e-5:
                color = "#ffffff"
                self.basis = [str(self._editBasis1.text()), str(self._editBasis2.text()), str(self._editBasis3.text())]
                self.updateNames([b1, b2, b3])
        self._editBasis1.setStyleSheet("QLineEdit { background-color: %s }" % color)
        self._editBasis2.setStyleSheet("QLineEdit { background-color: %s }" % color)
        self._editBasis3.setStyleSheet("QLineEdit { background-color: %s }" % color)

    def updateNames(self, basis):
        chars = ["H", "K", "L"]
        for i in range(3):
            indexMax = numpy.argmax(numpy.abs(basis[i]))
            self.dimNames[i] = "[" + ",".join([translation(x, chars[indexMax]) for x in basis[i]]) + "]"
        self.dimNames[3] = "DeltaE"
        self.updateCombo()
        self.dimMin = [-numpy.inf, -numpy.inf, -numpy.inf, -numpy.inf]
        self.dimMax = [numpy.inf, numpy.inf, numpy.inf, numpy.inf]
        self.dimStep = [0.05, 0.05, 0.05, 1]
        self.dimIndex = [0, 1, 2, 3]
        self.updateGui()
        self.updateChanges()

    def toggleDeltaE(self, on):
        self._editMin4.setEnabled(on)
        self._editMax4.setEnabled(on)
        self._comboDim4.setEnabled(on)

        if not on:
            self._comboDim4.setCurrentIndex(self.dimNames.index("DeltaE"))
            self.updateCombo()
            self._comboDim1.model().item(3).setFlags(QtCore.Qt.NoItemFlags)
            self._comboDim2.model().item(3).setFlags(QtCore.Qt.NoItemFlags)
            self._comboDim3.model().item(3).setFlags(QtCore.Qt.NoItemFlags)
        else:
            self.set_editMin4(-numpy.inf)
            self.set_editMax4(numpy.inf)
            self.updateCombo()

    def updateGui(self):
        self._editMin1.setText(FloatToQString(self.dimMin[0]))
        self._editMin2.setText(FloatToQString(self.dimMin[1]))
        self._editMin3.setText(FloatToQString(self.dimMin[2]))
        self._editMin4.setText(FloatToQString(self.dimMin[3]))
        self._editMax1.setText(FloatToQString(self.dimMax[0]))
        self._editMax2.setText(FloatToQString(self.dimMax[1]))
        self._editMax3.setText(FloatToQString(self.dimMax[2]))
        self._editMax4.setText(FloatToQString(self.dimMax[3]))
        self._editStep1.setText(QString(format(self.dimStep[0], ".3f")))
        self._editStep2.setText(QString(format(self.dimStep[1], ".3f")))

    def updateCombo(self):
        self.inhibitSignal = True
        self._comboDim1.clear()
        self._comboDim2.clear()
        self._comboDim3.clear()
        self._comboDim4.clear()
        self._comboDim1.addItems(self.dimNames)
        self._comboDim2.addItems(self.dimNames)
        self._comboDim3.addItems(self.dimNames)
        self._comboDim4.addItems(self.dimNames)
        self._comboDim1.setCurrentIndex(0)
        self._comboDim2.setCurrentIndex(1)
        self._comboDim3.setCurrentIndex(2)
        self._comboDim4.setCurrentIndex(3)
        self.inhibitSignal = False

    def updateChanges(self):
        d = dict()
        d["dimBasis"] = self.basis
        d["dimNames"] = self.dimNames
        d["dimMin"] = self.dimMin
        d["dimMax"] = self.dimMax
        d["dimStep"] = self.dimStep
        d["dimIndex"] = self.dimIndex
        self.changed.emit(d)

    def set_editMin4(self, val):
        self._editMin4.setText(FloatToQString(val))
        self.dimMin[3] = val
        self.updateChanges()

    def set_editMax4(self, val):
        self._editMax4.setText(FloatToQString(val))
        self.dimMax[3] = val
        self.updateChanges()


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    mainForm = DimensionSelectorWidget()
    mainForm.show()
    sys.exit(app.exec_())
