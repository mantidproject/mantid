# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-name-in-module,too-many-instance-attributes,too-many-public-methods

from qtpy import QtGui, QtCore, QtWidgets
import sys
import mantid
import numpy

sys.path.append("..")
# the following matplotlib imports cannot be placed before the setting of the backend, so we ignore flake8 warnings
from mantidqt.MPLwidgets import *
from matplotlib.figure import Figure
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot  # noqa

try:
    from qtpy.QtCore import QString
except ImportError:
    QString = type("")


class GonioTableModel(QtCore.QAbstractTableModel):
    """
    Dealing with the goniometer input
    """

    changed = QtCore.Signal(dict)  # each value is a list

    def __init__(self, axes, parent=None):
        QtCore.QAbstractTableModel.__init__(self, parent)
        self.labels = axes["gonioLabels"]
        self.dirstrings = axes["gonioDirs"]
        self.senses = axes["gonioSenses"]
        self.minvalues = axes["gonioMinvals"]
        self.maxvalues = axes["gonioMaxvals"]
        self.steps = axes["gonioSteps"]
        self.gonioColumns = ["Name", "Direction", "Sense (+/-1)", "Minim(deg)", "Maxim(deg)", "Step(deg)"]
        self.gonioRows = ["Axis0", "Axis1", "Axis2"]

    def rowCount(self, dummy_parent):
        return 3

    def columnCount(self, dummy_parent):
        return 6

    def flags(self, dummy_index):
        return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable

    def headerData(self, section, Qt_Orientation, role=None):
        if role == QtCore.Qt.DisplayRole and Qt_Orientation == QtCore.Qt.Horizontal:
            return self.gonioColumns[section]
        if role == QtCore.Qt.DisplayRole and Qt_Orientation == QtCore.Qt.Vertical:
            return self.gonioRows[section]

    def data(self, index, role):
        # pylint: disable=too-many-branches
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.EditRole or role == QtCore.Qt.DisplayRole:
            if column == 0:
                value = QString(self.labels[row])
            elif column == 1:
                value = QString(self.dirstrings[row])
            elif column == 2:
                value = QString(str(self.senses[row]))
            elif column == 3:
                value = QString(str(self.minvalues[row]))
            elif column == 4:
                value = QString(str(self.maxvalues[row]))
            elif column == 5:
                value = QString(str(self.steps[row]))
            return value
        elif role == QtCore.Qt.BackgroundRole:
            brush = QtGui.QBrush(QtCore.Qt.white)
            if column == 0 and len(self.labels[row]) > 0 and self.labels.count(self.labels[row]) == 1:
                pass
            elif column == 1 and self.validDir(self.dirstrings[row]):
                pass
            elif column == 2 and (self.senses[row] == 1 or self.senses[row] == -1):
                pass
            elif (column == 3 or column == 4) and self.minvalues[row] <= self.maxvalues[row]:
                pass
            elif column == 5 and self.steps[row] > 0.1:
                pass
            else:
                brush = QtGui.QBrush(QtCore.Qt.red)
            return brush

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        # pylint: disable=too-many-branches
        if role == QtCore.Qt.EditRole:
            row = index.row()
            column = index.column()
            if column <= 1:
                val = str(value)
                if column == 0:
                    self.labels[row] = val
                else:
                    self.dirstrings[row] = val
            elif column == 2:
                try:
                    val = int(value)
                except ValueError:
                    return False
                self.senses[row] = val
            else:
                try:
                    val = float(value)
                except ValueError:
                    return False
                if column == 3:
                    self.minvalues[row] = val
                elif column == 4:
                    self.maxvalues[row] = val
                else:
                    self.steps[row] = val
            self.dataChanged.emit(index, index)
            if self.validateGon():
                values = {
                    "gonioLabels": self.labels,
                    "gonioDirs": self.dirstrings,
                    "gonioSenses": self.senses,
                    "gonioMinvals": self.minvalues,
                    "gonioMaxvals": self.maxvalues,
                    "gonioSteps": self.steps,
                }
                self.changed.emit(values)
                return True
        return False

    def validDir(self, dirstring):
        d = numpy.fromstring(dirstring, dtype=float, sep=",")
        if len(d) == 3:
            return numpy.all(numpy.isfinite(d))
        return False

    def validateGon(self):
        for i in range(3):
            if len(self.labels[i]) == 0 or self.labels.count(self.labels[i]) > 1 or self.senses[i] not in [-1, 1]:
                return False
            if not self.validDir(self.dirstrings[i]):
                return False
            if self.minvalues[i] > self.maxvalues[i] or self.steps[i] <= 0:
                return False
        return True

    def updateGon(self, axes):
        self.beginResetModel()
        self.labels = axes["gonioLabels"]
        self.dirstrings = axes["gonioDirs"]
        self.senses = axes["gonioSenses"]
        self.minvalues = axes["gonioMinvals"]
        self.maxvalues = axes["gonioMaxvals"]
        self.steps = axes["gonioSteps"]
        self.gonioColumns = ["Name", "Direction", "Sense (+/-1)", "Minim(deg)", "Maxim(deg)", "Step(deg)"]
        self.gonioRows = ["Axis0", "Axis1", "Axis2"]
        self.endResetModel()


class InstrumentSetupWidget(QtWidgets.QWidget):
    # signal when things change and valid
    changed = QtCore.Signal(dict)

    def __init__(self, parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(InstrumentSetupWidget, self).__init__()
        metrics = QtGui.QFontMetrics(self.font())
        self.signaldict = dict()
        # instrument selector
        self.instrumentList = [
            "ARCS",
            "CHESS",
            "CNCS",
            "DNS",
            "DEMAND",
            "EXED",
            "FOCUS",
            "HET",
            "HYSPEC",
            "LET",
            "MAPS",
            "MARI",
            "MERLIN",
            "SEQUOIA",
            "WAND\u00b2",
        ]
        self.combo = QtWidgets.QComboBox(self)
        for inst in self.instrumentList:
            self.combo.addItem(inst)
        defaultInstrument = mantid.config.getInstrument().name()
        if defaultInstrument in self.instrumentList:
            self.instrument = defaultInstrument
            self.combo.setCurrentIndex(self.instrumentList.index(defaultInstrument))
        else:
            self.instrument = self.instrumentList[0]
            self.combo.setCurrentIndex(0)
        self.signaldict["instrument"] = self.instrument
        self.labelInst = QtWidgets.QLabel("Instrument")
        # S2 and Ei edits
        self.S2 = 0.0
        self.Ei = 10.0
        self.DetZ = 0.0
        self.signaldict["S2"] = self.S2
        self.signaldict["Ei"] = self.Ei
        self.signaldict["DetZ"] = self.DetZ
        self.validatorS2 = QtGui.QDoubleValidator(-90.0, 90.0, 5, self)
        self.validatorDetZ = QtGui.QDoubleValidator(-999999.0, 999999.0, 5, self)
        self.validatorEi = QtGui.QDoubleValidator(1.0, 10000.0, 5, self)
        self.labelS2 = QtWidgets.QLabel("S2")
        self.labelDetZ = QtWidgets.QLabel("DetZ")
        self.labelEi = QtWidgets.QLabel("Incident Energy")
        self.editS2 = QtWidgets.QLineEdit()
        self.editS2.setValidator(self.validatorS2)
        self.editDetZ = QtWidgets.QLineEdit()
        self.editDetZ.setValidator(self.validatorDetZ)
        self.editEi = QtWidgets.QLineEdit()
        self.editEi.setValidator(self.validatorEi)
        self.editS2.setText(QString(format(self.S2, ".2f")))
        self.editDetZ.setText(QString(format(self.DetZ, ".1f")))
        self.editEi.setText(QString(format(self.Ei, ".1f")))
        self.editEi.setFixedWidth(metrics.width("8888.88"))
        self.editS2.setFixedWidth(metrics.width("888.88"))
        # fast checkbox
        self.fast = QtWidgets.QCheckBox("Fast", self)
        self.fast.toggle()
        self.updateFast()
        # masking
        self.labelMask = QtWidgets.QLabel("Mask file")
        self.editMask = QtWidgets.QLineEdit()
        self.buttonMask = QtWidgets.QPushButton("LoadMask")
        # goniometer settings
        self.labelGon = QtWidgets.QLabel("Goniometer")
        self.tableViewGon = QtWidgets.QTableView(self)
        self.tableViewGon.setMinimumWidth(metrics.width("Minimum ") * 8)
        self.tableViewGon.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.tableViewGon.verticalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)

        self.goniometerNames = ["psi", "gl", "gs"]
        self.goniometerDirections = ["0,1,0", "0,0,1", "1,0,0"]
        self.goniometerRotationSense = [1, 1, 1]
        self.goniometerMin = [0.0, 0.0, 0.0]
        self.goniometerMax = [0.0, 0.0, 0.0]
        self.goniometerStep = [1.0, 1.0, 1.0]

        values = {
            "gonioLabels": self.goniometerNames,
            "gonioDirs": self.goniometerDirections,
            "gonioSenses": self.goniometerRotationSense,
            "gonioMinvals": self.goniometerMin,
            "gonioMaxvals": self.goniometerMax,
            "gonioSteps": self.goniometerStep,
        }
        self.goniomodel = GonioTableModel(values, self)
        self.tableViewGon.setModel(self.goniomodel)
        self.tableViewGon.update()
        self.signaldict.update(values)

        # goniometer figure
        self.figure = Figure(figsize=(2, 3))
        self.figure.patch.set_facecolor("white")
        self.canvas = FigureCanvasQTAgg(self.figure)
        self.gonfig = None
        self.updateFigure()
        # layout
        self.gridI = QtWidgets.QGridLayout()
        self.gridI.addWidget(self.labelInst, 0, 0)
        self.gridI.addWidget(self.combo, 0, 1)
        self.gridI.addWidget(self.labelEi, 0, 2)
        self.gridI.addWidget(self.editEi, 0, 3)
        self.gridI.addWidget(self.labelS2, 0, 4)
        self.gridI.addWidget(self.editS2, 0, 5)
        self.gridI.addWidget(self.labelDetZ, 0, 6)
        self.gridI.addWidget(self.editDetZ, 0, 7)
        self.gridI.addWidget(self.fast, 0, 8)
        self.setLayout(QtWidgets.QHBoxLayout())
        self.rightside = QtWidgets.QVBoxLayout()
        self.maskLayout = QtWidgets.QHBoxLayout()
        self.maskLayout.addWidget(self.labelMask)
        self.maskLayout.addWidget(self.editMask)
        self.maskLayout.addWidget(self.buttonMask)
        self.layout().addLayout(self.rightside)
        self.rightside.addLayout(self.gridI)
        self.rightside.addLayout(self.maskLayout)
        self.rightside.addWidget(self.labelGon)
        self.rightside.addWidget(self.tableViewGon)
        self.layout().addWidget(self.canvas)
        # connections
        self.editS2.textEdited.connect(self.checkValidInputs)
        self.editDetZ.textEdited.connect(self.checkValidInputs)
        self.editMask.textEdited.connect(self.setMaskFile)
        self.combo.activated[str].connect(self.instrumentSelected)
        self.fast.stateChanged.connect(self.updateFast)
        self.buttonMask.clicked.connect(self.loadMaskFromFile)
        self.editEi.textEdited.connect(self.checkValidInputs)
        # call instrumentSelected once
        self.instrumentSelected(self.instrument)
        # connect goniometer change with figure
        self.goniomodel.changed.connect(self.updateFigure)
        self.updateAll()

    def updateFigure(self):
        # plot directions
        if self.gonfig is not None:
            self.gonfig.clear()
        self.gonfig = Axes3D(self.figure, auto_add_to_figure=False)
        self.figure.add_axes(self.gonfig)
        self.gonfig.set_frame_on(False)
        self.gonfig.set_xlim3d(-0.6, 0.6)
        self.gonfig.set_ylim3d(-0.6, 0.6)
        self.gonfig.set_zlim3d(-1, 5)
        self.gonfig.set_axis_off()
        self.gonfig.plot([0, -1], [-3, -3], [0, 0], zdir="y", color="black")
        self.gonfig.plot([0, 0], [-3, -2], [0, 0], zdir="y", color="black")
        self.gonfig.plot([0, 0], [-3, -3], [0, 1], zdir="y", color="black")
        self.gonfig.text(0, 1, -2.5, "Z", zdir=None, color="black")
        self.gonfig.text(-1, 0, -2.5, "X", zdir=None, color="black")
        self.gonfig.plot([0, 0], [-3, -3], [-2, -0.5], zdir="y", color="black", linewidth=3)
        self.gonfig.text(0, -1, -2.5, "Beam", zdir=None, color="black")
        self.gonfig.view_init(10, 45)

        colors = ["b", "g", "r"]
        for i in range(3):
            circle = numpy.array(
                [mantid.kernel.Quat(0, 0, 0.5 * numpy.sin(t), 0.5 * numpy.cos(t)) for t in numpy.arange(0, 1.51 * numpy.pi, 0.1 * numpy.pi)]
            )
            if self.goniometerRotationSense[i] == 1:
                circle = numpy.append(circle, mantid.kernel.Quat(0, 0, -0.45, -0.05))
                circle = numpy.append(circle, mantid.kernel.Quat(0, 0, -0.55, -0.05))
                circle = numpy.append(circle, mantid.kernel.Quat(0, 0, -0.5, 0))
            else:
                circle = numpy.insert(circle, 0, mantid.kernel.Quat(0, 0, 0, 0.5))
                circle = numpy.insert(circle, 1, mantid.kernel.Quat(0, 0, 0.05, 0.45))
                circle = numpy.insert(circle, 2, mantid.kernel.Quat(0, 0, 0.05, 0.55))

            t = numpy.fromstring(self.goniometerDirections[i], dtype=float, sep=",")
            vt = mantid.kernel.V3D(t[0], t[1], t[2])
            vt *= 1.0 / vt.norm()
            direction = mantid.kernel.Quat(mantid.kernel.V3D(1, 0, 0), vt)
            directionS = mantid.kernel.Quat(direction[0], -direction[1], -direction[2], -direction[3])
            gonAxis = numpy.array([mantid.kernel.Quat(0, 1, 0, 0), mantid.kernel.Quat(0, -1, 0, 0)])

            newcircle = direction * circle * directionS
            newgonAxis = direction * gonAxis * directionS
            parray = numpy.array([(p[1], p[2] + 2 * i, p[3]) for p in newcircle])
            self.gonfig.plot(parray[:, 0], parray[:, 1], parray[:, 2], zdir="y", color=colors[i])
            parray = numpy.array([(p[1], p[2] + 2 * i, p[3]) for p in newgonAxis])
            self.gonfig.plot(parray[:, 0], parray[:, 1], parray[:, 2], zdir="y", color=colors[i])
            self.gonfig.plot([t[0], -t[0]], [t[1] + 2 * i, -t[1] + 2 * i], [t[2], -t[2]], zdir="y", color=colors[i])
            self.gonfig.text(0, 1, 2 * i, self.goniometerNames[i], zdir=None, color=colors[i])

        # plot sample
        self.gonfig.text(0, 0, 6.7, "Sample", zdir=None, color="black")
        u = numpy.linspace(0, 2 * numpy.pi, 50)
        v = numpy.linspace(0, numpy.pi, 50)
        x = 0.3 * numpy.outer(numpy.cos(u), numpy.sin(v))
        y = 0.3 * numpy.outer(numpy.sin(u), numpy.sin(v))
        z = 0.3 * numpy.outer(numpy.ones(numpy.size(u)), numpy.cos(v))
        self.gonfig.plot_surface(x, y, z + 6, color="black", rstride=4, cstride=4)
        self.canvas.draw()
        self.updateAll()

    def instrumentSelected(self, text):
        d = dict()
        self.instrument = text
        d["instrument"] = str(self.instrument)
        if self.instrument in ["HYSPEC", "EXED", "DEMAND", "WAND\u00b2"]:
            self.labelS2.show()
            self.editS2.show()
        else:
            self.labelS2.hide()
            self.editS2.hide()

        if self.instrument in ["DEMAND", "WAND\u00b2"]:
            self.labelDetZ.show()
            self.editDetZ.show()
            self.setLabelEi("Wavelength")
            if self.instrument == "DEMAND":
                self.setDetZVal(str(410.258))
                self.setLabelDetZ("DetTrans")
                self.setEiVal(str(1.542))
                self.goniometerNames = ["omega", "chi", "phi"]
                self.goniometerDirections = ["0,1,0", "0,0,1", "0,1,0"]
                self.goniometerRotationSense = [-1, -1, -1]
            else:
                self.setDetZVal(str(0.0))
                self.setLabelDetZ("DetZ")
                self.setEiVal(str(1.488))
                self.goniometerNames = ["s1", "sgl", "sgu"]
                self.goniometerDirections = ["0,1,0", "1,0,0", "0,0,1"]
                self.goniometerRotationSense = [1, -1, -1]
        else:
            self.labelDetZ.hide()
            self.editDetZ.hide()
            self.setLabelEi("Incident Energy")
            self.setEiVal(str(10.0))
            self.goniometerNames = ["psi", "gl", "gs"]
            self.goniometerDirections = ["0,1,0", "0,0,1", "1,0,0"]
            self.goniometerRotationSense = [1, 1, 1]

        d["gonioLabels"] = self.goniometerNames
        d["gonioDirs"] = self.goniometerDirections
        d["gonioSenses"] = self.goniometerRotationSense

        self.updateAll(**d)
        self.updateTable()
        self.updateFigure()

    def updateFast(self, *dummy_args):
        d = dict()
        d["makeFast"] = self.fast.isChecked()
        self.updateAll(**d)

    def loadMaskFromFile(self):
        fileName = QtWidgets.QFileDialog.getOpenFileName(self, "Open Mask File", "", "Processed Nexus (*.nxs);;All Files (*)")
        if not fileName:
            return
        if isinstance(fileName, tuple):
            fileName = fileName[0]
        self.editMask.setText(QString(fileName))
        self.setMaskFile()

    def setMaskFile(self):
        filename = str(self.editMask.text())
        d = {"maskFilename": filename}
        self.updateAll(**d)

    def setLabelEi(self, newLabel):
        self.labelEi.setText(newLabel)

    def setEiVal(self, val):
        self.Ei = float(val)
        self.editEi.setText(val)
        self.updateAll(Ei=float(val))

    def setLabelDetZ(self, newLabel):
        self.labelDetZ.setText(newLabel)

    def setDetZVal(self, val):
        self.DetZ = float(val)
        self.editDetZ.setText(val)
        self.updateAll(DetZ=float(val))

    def getInstrumentComboBox(self):
        return self.combo

    def getEditEi(self):
        return self.editEi

    def updateTable(self):
        values = {
            "gonioLabels": self.goniometerNames,
            "gonioDirs": self.goniometerDirections,
            "gonioSenses": self.goniometerRotationSense,
            "gonioMinvals": self.goniometerMin,
            "gonioMaxvals": self.goniometerMax,
            "gonioSteps": self.goniometerStep,
        }
        self.goniomodel.updateGon(values)

    def checkValidInputs(self, *dummy_args, **dummy_kwargs):
        sender = self.sender()
        state = sender.validator().validate(sender.text(), 0)[0]
        d = dict()
        if state == QtGui.QValidator.Acceptable:
            color = "#ffffff"
            if sender == self.editS2:
                self.S2 = float(sender.text())
                d["S2"] = self.S2
            if sender == self.editEi:
                self.Ei = float(sender.text())
                d["Ei"] = self.Ei
            if sender == self.editDetZ:
                self.DetZ = float(sender.text())
                d["DetZ"] = self.DetZ
        else:
            color = "#ff0000"
        sender.setStyleSheet("QLineEdit { background-color: %s }" % color)
        if state == QtGui.QValidator.Acceptable:
            self.updateAll(**d)

    def updateAll(self, *args, **kwargs):
        if len(args) > 0:
            self.signaldict.update(args[0])
        if kwargs != {}:
            self.signaldict.update(kwargs)
        self.changed.emit(self.signaldict)


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    mainForm = InstrumentSetupWidget()
    mainForm.show()
    sys.exit(app.exec_())
