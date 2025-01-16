# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,relative-import
import copy
import os
import sys

import numpy
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.gui_helper import show_interface_help
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
from matplotlib.figure import Figure
from mpl_toolkits.axisartist import Subplot
from mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear
from qtpy import QtCore, QtWidgets

import mantid
from mantid.kernel import UnitConversion, Elastic, UnitParametersMap
from . import ClassicUBInputWidget
from . import DimensionSelectorWidget
from . import InstrumentSetupWidget
from . import MatrixUBInputWidget
from .ValidateOL import ValidateOL


def float2Input(x):
    if numpy.isfinite(x):
        return x
    else:
        return None


# pylint: disable=too-many-instance-attributes


class DGSPlannerGUI(QtWidgets.QWidget):
    def __init__(self, parent=None, window_flags=None, ol=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(DGSPlannerGUI, self).__init__(parent)
        if window_flags:
            self.setWindowFlags(window_flags)
        # OrientedLattice
        if ValidateOL(ol):
            self.ol = ol
        else:
            self.ol = mantid.geometry.OrientedLattice()
        self.masterDict = dict()  # holds info about instrument and ranges
        self.updatedInstrument = False
        self.instrumentElastic = False
        self.updatedOL = False
        self.wg = None  # workspace group
        self.instrumentWidget = InstrumentSetupWidget.InstrumentSetupWidget(self)
        self.setLayout(QtWidgets.QHBoxLayout())
        controlLayout = QtWidgets.QVBoxLayout()
        geometryBox = QtWidgets.QGroupBox("Instrument Geometry")
        plotBox = QtWidgets.QGroupBox("Plot Axes")
        geometryBoxLayout = QtWidgets.QVBoxLayout()
        geometryBoxLayout.addWidget(self.instrumentWidget)
        geometryBox.setLayout(geometryBoxLayout)
        controlLayout.addWidget(geometryBox)
        self.ublayout = QtWidgets.QHBoxLayout()
        self.classic = ClassicUBInputWidget.ClassicUBInputWidget(self.ol)
        self.ublayout.addWidget(self.classic, alignment=QtCore.Qt.AlignTop, stretch=1)
        self.matrix = MatrixUBInputWidget.MatrixUBInputWidget(self.ol)
        self.ublayout.addWidget(self.matrix, alignment=QtCore.Qt.AlignTop, stretch=1)
        sampleBox = QtWidgets.QGroupBox("Sample")
        sampleBox.setLayout(self.ublayout)
        controlLayout.addWidget(sampleBox)
        self.dimensionWidget = DimensionSelectorWidget.DimensionSelectorWidget(self)
        plotBoxLayout = QtWidgets.QVBoxLayout()
        plotBoxLayout.addWidget(self.dimensionWidget)
        plotControlLayout = QtWidgets.QGridLayout()
        self.plotButton = QtWidgets.QPushButton("Plot", self)
        self.oplotButton = QtWidgets.QPushButton("Overplot", self)
        self.helpButton = QtWidgets.QPushButton("?", self)
        self.colorLabel = QtWidgets.QLabel("Color by angle", self)
        self.colorButton = QtWidgets.QCheckBox(self)
        self.colorButton.toggle()
        self.aspectLabel = QtWidgets.QLabel("Aspect ratio 1:1", self)
        self.aspectButton = QtWidgets.QCheckBox(self)
        self.saveButton = QtWidgets.QPushButton("Save Figure", self)
        plotControlLayout.addWidget(self.plotButton, 0, 0)
        plotControlLayout.addWidget(self.oplotButton, 0, 1)
        plotControlLayout.addWidget(self.colorLabel, 0, 2, QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.colorButton, 0, 3)
        plotControlLayout.addWidget(self.aspectLabel, 0, 4, QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.aspectButton, 0, 5)
        plotControlLayout.addWidget(self.helpButton, 0, 6)
        plotControlLayout.addWidget(self.saveButton, 0, 7)
        plotBoxLayout.addLayout(plotControlLayout)
        plotBox = QtWidgets.QGroupBox("Plot Axes")
        plotBox.setLayout(plotBoxLayout)
        controlLayout.addWidget(plotBox)
        self.layout().addLayout(controlLayout)

        # figure
        self.figure = Figure()
        self.figure.patch.set_facecolor("white")
        self.canvas = FigureCanvas(self.figure)
        self.grid_helper = GridHelperCurveLinear((self.tr, self.inv_tr))
        self.trajfig = Subplot(self.figure, 1, 1, 1, grid_helper=self.grid_helper)
        self.figure.add_subplot(self.trajfig)
        self.toolbar = MantidNavigationToolbar(self.canvas, self)
        figureLayout = QtWidgets.QVBoxLayout()
        figureLayout.addWidget(self.toolbar, 0)
        figureLayout.addWidget(self.canvas, 1)
        self.layout().addLayout(figureLayout)
        self.needToClear = False
        self.saveDir = ""

        # connections
        self.matrix.UBmodel.changed.connect(self.updateUB)
        self.matrix.UBmodel.changed.connect(self.classic.updateOL)
        self.classic.changed.connect(self.matrix.UBmodel.updateOL)
        self.classic.changed.connect(self.updateUB)
        self.instrumentWidget.changed.connect(self.updateParams)
        self.instrumentWidget.getInstrumentComboBox().activated[str].connect(self.instrumentUpdateEvent)
        self.instrumentWidget.getEditEi().textChanged.connect(self.eiWavelengthUpdateEvent)
        self.dimensionWidget.changed.connect(self.updateParams)
        self.plotButton.clicked.connect(self.updateFigure)
        self.oplotButton.clicked.connect(self.updateFigure)
        self.helpButton.clicked.connect(self.help)
        self.saveButton.clicked.connect(self.save)
        # force an update of values
        self.instrumentWidget.updateAll()
        self.dimensionWidget.updateChanges()
        # help
        self.assistant_process = QtCore.QProcess(self)
        # pylint: disable=protected-access
        self.mantidplot_name = "DGS Planner"
        # control for cancel button
        self.iterations = 0
        self.progress_canceled = False

        # register startup
        mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "DGSPlanner", False)

    @QtCore.Slot(mantid.geometry.OrientedLattice)
    def updateUB(self, ol):
        self.ol = ol
        self.updatedOL = True
        self.trajfig.clear()

    def eiWavelengthUpdateEvent(self):
        if self.instrumentElastic:
            params = UnitParametersMap()
            ei = UnitConversion.run("Wavelength", "Energy", self.masterDict["Ei"], 0, Elastic, params)
            offset = ei * 0.01
            lowerBound = -offset
            upperBound = offset
            self.dimensionWidget.set_editMin4(lowerBound)
            self.dimensionWidget.set_editMax4(upperBound)

    def instrumentUpdateEvent(self):
        changeToElastic = self.masterDict["instrument"] in ["DEMAND", "WAND\u00b2"]
        if changeToElastic != self.instrumentElastic:
            self.instrumentElastic = changeToElastic
            self.dimensionWidget.toggleDeltaE(not changeToElastic)
        self.eiWavelengthUpdateEvent()

    @QtCore.Slot(dict)
    def updateParams(self, d):
        if self.sender() is self.instrumentWidget:
            self.updatedInstrument = True
        if "dimBasis" in d and "dimBasis" in self.masterDict and d["dimBasis"] != self.masterDict["dimBasis"]:
            self.needToClear = True
        if "dimIndex" in d and "dimIndex" in self.masterDict and d["dimIndex"] != self.masterDict["dimIndex"]:
            self.needToClear = True
        self.masterDict.update(copy.deepcopy(d))

    def help(self):
        show_interface_help(self.mantidplot_name, self.assistant_process, area="direct")

    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()

    def _create_goniometer_workspaces(self, gonioAxis0values, gonioAxis1values, gonioAxis2values, progressDialog):
        groupingStrings = []
        i = 0
        for g0 in gonioAxis0values:
            for g1 in gonioAxis1values:
                for g2 in gonioAxis2values:
                    name = "__temp_instrument" + str(i)
                    i += 1
                    progressDialog.setValue(i)
                    progressDialog.setLabelText("Creating workspace %d of %d..." % (i, self.iterations))
                    QtWidgets.qApp.processEvents()
                    if progressDialog.wasCanceled():
                        self.progress_canceled = True
                        progressDialog.close()
                        return None

                    groupingStrings.append(name)
                    mantid.simpleapi.CloneWorkspace("__temp_instrument", OutputWorkspace=name)
                    mantid.simpleapi.SetGoniometer(
                        Workspace=name,
                        Axis0=str(g0) + "," + self.masterDict["gonioDirs"][0] + "," + str(self.masterDict["gonioSenses"][0]),
                        Axis1=str(g1) + "," + self.masterDict["gonioDirs"][1] + "," + str(self.masterDict["gonioSenses"][1]),
                        Axis2=str(g2) + "," + self.masterDict["gonioDirs"][2] + "," + str(self.masterDict["gonioSenses"][2]),
                    )
        return groupingStrings

    # pylint: disable=too-many-locals
    def updateFigure(self):  # noqa: C901
        # pylint: disable=too-many-branches
        if self.updatedInstrument or self.progress_canceled:
            self.progress_canceled = False
            # get goniometer settings first
            gonioAxis0values = numpy.arange(
                self.masterDict["gonioMinvals"][0],
                self.masterDict["gonioMaxvals"][0] + 0.1 * self.masterDict["gonioSteps"][0],
                self.masterDict["gonioSteps"][0],
            )
            gonioAxis1values = numpy.arange(
                self.masterDict["gonioMinvals"][1],
                self.masterDict["gonioMaxvals"][1] + 0.1 * self.masterDict["gonioSteps"][1],
                self.masterDict["gonioSteps"][1],
            )
            gonioAxis2values = numpy.arange(
                self.masterDict["gonioMinvals"][2],
                self.masterDict["gonioMaxvals"][2] + 0.1 * self.masterDict["gonioSteps"][2],
                self.masterDict["gonioSteps"][2],
            )
            self.iterations = len(gonioAxis0values) * len(gonioAxis1values) * len(gonioAxis2values)
            if self.iterations > 10:
                reply = QtWidgets.QMessageBox.warning(
                    self,
                    "Goniometer",
                    "More than 10 goniometer settings. This might be long.\nAre you sure you want to proceed?",
                    QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                    QtWidgets.QMessageBox.No,
                )
                if reply == QtWidgets.QMessageBox.No:
                    return

            try:
                mantid.simpleapi.DeleteWorkspace(self.wg)
            except:
                pass

            instrumentName = self.masterDict["instrument"]
            if instrumentName == "DEMAND":
                instrumentName = "HB3A"
            elif instrumentName == "WAND\u00b2":
                instrumentName = "WAND"

            mantid.simpleapi.LoadEmptyInstrument(
                mantid.api.ExperimentInfo.getInstrumentFilename(instrumentName), OutputWorkspace="__temp_instrument"
            )
            if self.masterDict["instrument"] == "HYSPEC":
                mantid.simpleapi.AddSampleLog(Workspace="__temp_instrument", LogName="msd", LogText="1798.5", LogType="Number Series")
                mantid.simpleapi.AddSampleLog(
                    Workspace="__temp_instrument", LogName="s2", LogText=str(self.masterDict["S2"]), LogType="Number Series"
                )
                mantid.simpleapi.LoadInstrument(Workspace="__temp_instrument", RewriteSpectraMap=True, InstrumentName="HYSPEC")
            elif self.masterDict["instrument"] == "EXED":
                mantid.simpleapi.RotateInstrumentComponent(
                    Workspace="__temp_instrument", ComponentName="Tank", Y=1, Angle=str(self.masterDict["S2"]), RelativeRotation=False
                )
            elif instrumentName == "HB3A":
                mantid.simpleapi.AddSampleLog(
                    Workspace="__temp_instrument", LogName="2theta", LogText=str(self.masterDict["S2"]), LogType="Number Series"
                )
                mantid.simpleapi.AddSampleLog(
                    Workspace="__temp_instrument",
                    LogName="det_trans",
                    LogText=str(self.masterDict["DetZ"]),
                    LogType="Number Series",
                )
                mantid.simpleapi.LoadInstrument(Workspace="__temp_instrument", RewriteSpectraMap=True, InstrumentName="HB3A")
            elif instrumentName == "WAND":
                mantid.simpleapi.AddSampleLog(
                    Workspace="__temp_instrument", LogName="HB2C:Mot:s2.RBV", LogText=str(self.masterDict["S2"]), LogType="Number Series"
                )
                mantid.simpleapi.AddSampleLog(
                    Workspace="__temp_instrument",
                    LogName="HB2C:Mot:detz.RBV",
                    LogText=str(self.masterDict["DetZ"]),
                    LogType="Number Series",
                )
                mantid.simpleapi.LoadInstrument(Workspace="__temp_instrument", RewriteSpectraMap=True, InstrumentName="WAND")
            # masking
            if "maskFilename" in self.masterDict and len(self.masterDict["maskFilename"].strip()) > 0:
                try:
                    __maskWS = mantid.simpleapi.Load(self.masterDict["maskFilename"])
                    mantid.simpleapi.MaskDetectors(Workspace="__temp_instrument", MaskedWorkspace=__maskWS)
                except (ValueError, RuntimeError) as e:
                    reply = QtWidgets.QMessageBox.critical(
                        self,
                        "Error",
                        "The following error has occurred in loading the mask:\n" + str(e) + "\nDo you want to continue without mask?",
                        QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                        QtWidgets.QMessageBox.No,
                    )
                    if reply == QtWidgets.QMessageBox.No:
                        return
            if self.masterDict["makeFast"]:
                sp = numpy.arange(mantid.mtd["__temp_instrument"].getNumberHistograms())
                if self.masterDict["instrument"] in ["DEMAND", "WAND\u00b2"]:
                    sp = sp.reshape(-1, 512)
                    tomask = (
                        sp[:, 1::4].ravel().tolist()
                        + sp[:, 2::4].ravel().tolist()
                        + sp[:, 3::4].ravel().tolist()
                        + sp[1::4, :].ravel().tolist()
                        + sp[2::4, :].ravel().tolist()
                        + sp[3::4, :].ravel().tolist()
                    )
                else:
                    tomask = sp[1::4].tolist() + sp[2::4].tolist() + sp[3::4].tolist()
                mantid.simpleapi.MaskDetectors("__temp_instrument", SpectraList=tomask)

            progressDialog = QtWidgets.QProgressDialog(self)
            progressDialog.setMinimumDuration(0)
            progressDialog.setCancelButtonText("&Cancel")
            progressDialog.setRange(0, self.iterations)
            progressDialog.setWindowTitle("DGSPlanner progress")

            groupingStrings = self._create_goniometer_workspaces(gonioAxis0values, gonioAxis1values, gonioAxis2values, progressDialog)
            if groupingStrings is None:
                return

            progressDialog.close()
            mantid.simpleapi.DeleteWorkspace("__temp_instrument")
            self.wg = mantid.simpleapi.GroupWorkspaces(groupingStrings, OutputWorkspace="__temp_instrument")
            self.updatedInstrument = False
        # set the UB
        if self.updatedOL or not self.wg[0].sample().hasOrientedLattice():
            mantid.simpleapi.SetUB(self.wg, UB=self.ol.getUB())
            self.updatedOL = False
        # calculate coverage
        dimensions = ["Q1", "Q2", "Q3", "DeltaE"]
        progressDialog = QtWidgets.QProgressDialog(self)
        progressDialog.setMinimumDuration(0)
        progressDialog.setCancelButtonText("&Cancel")
        progressDialog.setRange(0, self.iterations)
        progressDialog.setWindowTitle("DGSPlanner progress")

        if self.masterDict["instrument"] in ["DEMAND", "WAND\u00b2"]:
            params = UnitParametersMap()
            ei = UnitConversion.run("Wavelength", "Energy", self.masterDict["Ei"], 0, Elastic, params)
        else:
            ei = self.masterDict["Ei"]

        for i in range(self.iterations):
            progressDialog.setValue(i)
            progressDialog.setLabelText("Calculating orientation %d of %d..." % (i, self.iterations))
            QtWidgets.qApp.processEvents()
            if progressDialog.wasCanceled():
                self.progress_canceled = True
                progressDialog.close()
                return

            __mdws = mantid.simpleapi.CalculateCoverageDGS(
                self.wg[i],
                Q1Basis=self.masterDict["dimBasis"][0],
                Q2Basis=self.masterDict["dimBasis"][1],
                Q3Basis=self.masterDict["dimBasis"][2],
                IncidentEnergy=ei,
                Dimension1=dimensions[self.masterDict["dimIndex"][0]],
                Dimension1Min=float2Input(self.masterDict["dimMin"][0]),
                Dimension1Max=float2Input(self.masterDict["dimMax"][0]),
                Dimension1Step=float2Input(self.masterDict["dimStep"][0]),
                Dimension2=dimensions[self.masterDict["dimIndex"][1]],
                Dimension2Min=float2Input(self.masterDict["dimMin"][1]),
                Dimension2Max=float2Input(self.masterDict["dimMax"][1]),
                Dimension2Step=float2Input(self.masterDict["dimStep"][1]),
                Dimension3=dimensions[self.masterDict["dimIndex"][2]],
                Dimension3Min=float2Input(self.masterDict["dimMin"][2]),
                Dimension3Max=float2Input(self.masterDict["dimMax"][2]),
                Dimension4=dimensions[self.masterDict["dimIndex"][3]],
                Dimension4Min=float2Input(self.masterDict["dimMin"][3]),
                Dimension4Max=float2Input(self.masterDict["dimMax"][3]),
            )

            if i == 0:
                intensity = __mdws.getSignalArray()[:, :, 0, 0] * 1.0  # to make it writeable
            else:
                if self.colorButton.isChecked():
                    tempintensity = __mdws.getSignalArray()[:, :, 0, 0]
                    intensity[numpy.where(tempintensity > 0)] = i + 1.0
                else:
                    tempintensity = __mdws.getSignalArray()[:, :, 0, 0]
                    intensity[numpy.where(tempintensity > 0)] = 1.0
        progressDialog.close()
        x = numpy.linspace(__mdws.getDimension(0).getMinimum(), __mdws.getDimension(0).getMaximum(), intensity.shape[0])
        y = numpy.linspace(__mdws.getDimension(1).getMinimum(), __mdws.getDimension(1).getMaximum(), intensity.shape[1])
        Y, X = numpy.meshgrid(y, x)
        xx, yy = self.tr(X, Y)
        Z = numpy.ma.masked_array(intensity, intensity == 0)
        Z = Z[:-1, :-1]
        # plotting
        if self.sender() is self.plotButton or self.needToClear:
            self.figure.clear()
            self.trajfig.clear()
            self.figure.add_subplot(self.trajfig)
            self.needToClear = False
        self.trajfig.pcolorfast(xx, yy, Z)

        if self.aspectButton.isChecked():
            self.trajfig.set_aspect(1.0)
        else:
            self.trajfig.set_aspect("auto")
        self.trajfig.set_xlabel(self.masterDict["dimNames"][0])
        self.trajfig.set_ylabel(self.masterDict["dimNames"][1])
        self.trajfig.grid(True)
        self.canvas.draw()
        mantid.simpleapi.DeleteWorkspace(__mdws)

    def save(self):
        fileName = QtWidgets.QFileDialog.getSaveFileName(self, "Save Plot", self.saveDir, "*.png")
        if isinstance(fileName, tuple):
            fileName = fileName[0]
        if not fileName:
            return
        data = "Instrument " + self.masterDict["instrument"] + "\n"
        if self.masterDict["instrument"] == "HYSPEC":
            data += "S2 = " + str(self.masterDict["S2"]) + "\n"
        data += "Ei = " + str(self.masterDict["Ei"]) + " meV\n"
        data += "Goniometer values:\n"
        gonioAxis0values = numpy.arange(
            self.masterDict["gonioMinvals"][0],
            self.masterDict["gonioMaxvals"][0] + 0.1 * self.masterDict["gonioSteps"][0],
            self.masterDict["gonioSteps"][0],
        )
        gonioAxis1values = numpy.arange(
            self.masterDict["gonioMinvals"][1],
            self.masterDict["gonioMaxvals"][1] + 0.1 * self.masterDict["gonioSteps"][1],
            self.masterDict["gonioSteps"][1],
        )
        gonioAxis2values = numpy.arange(
            self.masterDict["gonioMinvals"][2],
            self.masterDict["gonioMaxvals"][2] + 0.1 * self.masterDict["gonioSteps"][2],
            self.masterDict["gonioSteps"][2],
        )
        for g0 in gonioAxis0values:
            for g1 in gonioAxis1values:
                for g2 in gonioAxis2values:
                    data += "    " + self.masterDict["gonioLabels"][0] + " = " + str(g0)
                    data += "    " + self.masterDict["gonioLabels"][1] + " = " + str(g1)
                    data += "    " + self.masterDict["gonioLabels"][2] + " = " + str(g2) + "\n"
        data += "Lattice parameters:\n"
        data += "    a = " + str(self.ol.a()) + "    b = " + str(self.ol.b()) + "    c = " + str(self.ol.c()) + "\n"
        data += "    alpha = " + str(self.ol.alpha()) + "    beta = " + str(self.ol.beta()) + "    gamma = " + str(self.ol.gamma()) + "\n"
        data += "Orientation vectors:\n"
        data += "    u = " + str(self.ol.getuVector()) + "\n"
        data += "    v = " + str(self.ol.getvVector()) + "\n"
        data += (
            "Integrated "
            + self.masterDict["dimNames"][2]
            + " between "
            + str(self.masterDict["dimMin"][2])
            + " and "
            + str(self.masterDict["dimMax"][2])
            + "\n"
        )
        data += (
            "Integrated "
            + self.masterDict["dimNames"][3]
            + " between "
            + str(self.masterDict["dimMin"][3])
            + " and "
            + str(self.masterDict["dimMax"][3])
            + "\n"
        )

        info = self.figure.text(0.2, 0, data, verticalalignment="top")
        self.figure.savefig(fileName, bbox_inches="tight", additional_artists=info)
        self.saveDir = os.path.dirname(fileName)

    def tr(self, x, y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        # one of the axes is energy
        if self.masterDict["dimIndex"][0] == 3 or self.masterDict["dimIndex"][1] == 3:
            return x, y
        else:
            h1, k1, l1 = (float(temp) for temp in self.masterDict["dimBasis"][self.masterDict["dimIndex"][0]].split(","))
            h2, k2, l2 = (float(temp) for temp in self.masterDict["dimBasis"][self.masterDict["dimIndex"][1]].split(","))
            angle = numpy.radians(self.ol.recAngle(h1, k1, l1, h2, k2, l2))
            return 1.0 * x + numpy.cos(angle) * y, numpy.sin(angle) * y

    def inv_tr(self, x, y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        # one of the axes is energy
        if self.masterDict["dimIndex"][0] == 3 or self.masterDict["dimIndex"][1] == 3:
            return x, y
        else:
            h1, k1, l1 = (float(temp) for temp in self.masterDict["dimBasis"][self.masterDict["dimIndex"][0]].split(","))
            h2, k2, l2 = (float(temp) for temp in self.masterDict["dimBasis"][self.masterDict["dimIndex"][1]].split(","))
            angle = numpy.radians(self.ol.recAngle(h1, k1, l1, h2, k2, l2))
            return 1.0 * x - y / numpy.tan(angle), y / numpy.sin(angle)


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    orl = mantid.geometry.OrientedLattice(2, 3, 4, 90, 90, 90)
    mainForm = DGSPlannerGUI()
    mainForm.show()
    sys.exit(app.exec_())
