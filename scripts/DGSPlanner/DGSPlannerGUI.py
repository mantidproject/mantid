# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,relative-import
from __future__ import (absolute_import, division, print_function)
from . import InstrumentSetupWidget
from . import ClassicUBInputWidget
from . import MatrixUBInputWidget
from . import DimensionSelectorWidget
from PyQt4 import QtCore, QtGui
import sys
import mantid
import mantidqtpython as mqt
from .ValidateOL import ValidateOL
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
try:
    from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar
except ImportError:
    try:
        from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
    except ImportError:
        raise ImportError("Cannot import navigation toolbar")
from matplotlib.figure import Figure
from mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear
from mpl_toolkits.axisartist import Subplot
import numpy
import copy
import os


def float2Input(x):
    if numpy.isfinite(x):
        return x
    else:
        return None


# pylint: disable=too-many-instance-attributes


class CustomNavigationToolbar(NavigationToolbar):
    toolitems = [t for t in NavigationToolbar.toolitems if t[0] in ('Home', 'Pan', 'Zoom')]


class DGSPlannerGUI(QtGui.QWidget):
    def __init__(self, ol=None, parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(DGSPlannerGUI, self).__init__(parent)
        # OrientedLattice
        if ValidateOL(ol):
            self.ol = ol
        else:
            self.ol = mantid.geometry.OrientedLattice()
        self.masterDict = dict()  # holds info about instrument and ranges
        self.updatedInstrument = False
        self.updatedOL = False
        self.wg = None  # workspace group
        self.instrumentWidget = InstrumentSetupWidget.InstrumentSetupWidget(self)
        self.setLayout(QtGui.QHBoxLayout())
        controlLayout = QtGui.QVBoxLayout()
        controlLayout.addWidget(self.instrumentWidget)
        self.ublayout = QtGui.QHBoxLayout()
        self.classic = ClassicUBInputWidget.ClassicUBInputWidget(self.ol)
        self.ublayout.addWidget(self.classic, alignment=QtCore.Qt.AlignTop, stretch=1)
        self.matrix = MatrixUBInputWidget.MatrixUBInputWidget(self.ol)
        self.ublayout.addWidget(self.matrix, alignment=QtCore.Qt.AlignTop, stretch=1)
        controlLayout.addLayout(self.ublayout)
        self.dimensionWidget = DimensionSelectorWidget.DimensionSelectorWidget(self)
        controlLayout.addWidget(self.dimensionWidget)
        plotControlLayout = QtGui.QGridLayout()
        self.plotButton = QtGui.QPushButton("Plot", self)
        self.oplotButton = QtGui.QPushButton("Overplot", self)
        self.helpButton = QtGui.QPushButton("?", self)
        self.colorLabel = QtGui.QLabel('Color by angle', self)
        self.colorButton = QtGui.QCheckBox(self)
        self.colorButton.toggle()
        self.aspectLabel = QtGui.QLabel('Aspect ratio 1:1', self)
        self.aspectButton = QtGui.QCheckBox(self)
        self.saveButton = QtGui.QPushButton("Save Figure", self)
        plotControlLayout.addWidget(self.plotButton, 0, 0)
        plotControlLayout.addWidget(self.oplotButton, 0, 1)
        plotControlLayout.addWidget(self.colorLabel, 0, 2, QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.colorButton, 0, 3)
        plotControlLayout.addWidget(self.aspectLabel, 0, 4, QtCore.Qt.AlignRight)
        plotControlLayout.addWidget(self.aspectButton, 0, 5)
        plotControlLayout.addWidget(self.helpButton, 0, 6)
        plotControlLayout.addWidget(self.saveButton, 0, 7)
        controlLayout.addLayout(plotControlLayout)
        self.layout().addLayout(controlLayout)

        # figure
        self.figure = Figure()
        self.figure.patch.set_facecolor('white')
        self.canvas = FigureCanvas(self.figure)
        self.grid_helper = GridHelperCurveLinear((self.tr, self.inv_tr))
        self.trajfig = Subplot(self.figure, 1, 1, 1, grid_helper=self.grid_helper)
        self.trajfig.hold(True)
        self.figure.add_subplot(self.trajfig)
        self.toolbar = CustomNavigationToolbar(self.canvas, self)
        figureLayout = QtGui.QVBoxLayout()
        figureLayout.addWidget(self.toolbar,0)
        figureLayout.addWidget(self.canvas,1)
        self.layout().addLayout(figureLayout)
        self.needToClear = False
        self.saveDir = ''

        # connections
        self.matrix.UBmodel.changed.connect(self.updateUB)
        self.matrix.UBmodel.changed.connect(self.classic.updateOL)
        self.classic.changed.connect(self.matrix.UBmodel.updateOL)
        self.classic.changed.connect(self.updateUB)
        self.instrumentWidget.changed.connect(self.updateParams)
        self.dimensionWidget.changed.connect(self.updateParams)
        self.plotButton.clicked.connect(self.updateFigure)
        self.oplotButton.clicked.connect(self.updateFigure)
        self.helpButton.clicked.connect(self.help)
        self.saveButton.clicked.connect(self.save)
        # force an update of values
        self.instrumentWidget.updateAll()
        self.dimensionWidget.updateChanges()
        # help
        self.assistantProcess = QtCore.QProcess(self)
        # pylint: disable=protected-access
        self.collectionFile = os.path.join(mantid._bindir, '../docs/qthelp/MantidProject.qhc')
        version = ".".join(mantid.__version__.split(".")[:2])
        self.qtUrl = 'qthelp://org.sphinx.mantidproject.' + version + '/doc/interfaces/DGS Planner.html'
        self.externalUrl = 'http://docs.mantidproject.org/nightly/interfaces/DGS Planner.html'
        # control for cancel button
        self.iterations = 0
        self.progress_canceled = False

        # register startup
        mantid.UsageService.registerFeatureUsage("Interface", "DGSPlanner", False)

    @QtCore.pyqtSlot(mantid.geometry.OrientedLattice)
    def updateUB(self, ol):
        self.ol = ol
        self.updatedOL = True
        self.trajfig.clear()

    @QtCore.pyqtSlot(dict)
    def updateParams(self, d):
        if self.sender() is self.instrumentWidget:
            self.updatedInstrument = True
        if 'dimBasis' in d and 'dimBasis' in self.masterDict and d['dimBasis'] != self.masterDict['dimBasis']:
            self.needToClear = True
        if 'dimIndex' in d and 'dimIndex' in self.masterDict and d['dimIndex'] != self.masterDict['dimIndex']:
            self.needToClear = True
        self.masterDict.update(copy.deepcopy(d))

    def help(self):
        try:
            import pymantidplot
            pymantidplot.proxies.showCustomInterfaceHelp('DGS Planner')
        except ImportError:
            self.assistantProcess.close()
            self.assistantProcess.waitForFinished()
            helpapp = QtCore.QLibraryInfo.location(QtCore.QLibraryInfo.BinariesPath) + QtCore.QDir.separator()
            helpapp += 'assistant'
            args = ['-enableRemoteControl', '-collectionFile', self.collectionFile, '-showUrl', self.qtUrl]
            if os.path.isfile(helpapp) and os.path.isfile(self.collectionFile):
                self.assistantProcess.close()
                self.assistantProcess.waitForFinished()
                self.assistantProcess.start(helpapp, args)
            else:
                mqt.MantidQt.API.MantidDesktopServices.openUrl(QtCore.QUrl(self.externalUrl))

    def closeEvent(self, event):
        self.assistantProcess.close()
        self.assistantProcess.waitForFinished()
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
                    QtGui.qApp.processEvents()
                    if progressDialog.wasCanceled():
                        self.progress_canceled = True
                        progressDialog.close()
                        return None

                    groupingStrings.append(name)
                    mantid.simpleapi.CloneWorkspace("__temp_instrument", OutputWorkspace=name)
                    mantid.simpleapi.SetGoniometer(Workspace=name,
                                                   Axis0=str(g0) + "," + self.masterDict['gonioDirs'][0] +
                                                   "," + str(self.masterDict['gonioSenses'][0]),
                                                   Axis1=str(g1) + "," + self.masterDict['gonioDirs'][1] +
                                                   "," + str(self.masterDict['gonioSenses'][1]),
                                                   Axis2=str(g2) + "," + self.masterDict['gonioDirs'][2] +
                                                   "," + str(self.masterDict['gonioSenses'][2]))
        return groupingStrings

    # pylint: disable=too-many-locals
    def updateFigure(self):
        # pylint: disable=too-many-branches
        if self.updatedInstrument or self.progress_canceled:
            self.progress_canceled = False
            # get goniometer settings first
            gonioAxis0values = numpy.arange(self.masterDict['gonioMinvals'][0],
                                            self.masterDict['gonioMaxvals'][0] + 0.1 * self.masterDict['gonioSteps'][0],
                                            self.masterDict['gonioSteps'][0])
            gonioAxis1values = numpy.arange(self.masterDict['gonioMinvals'][1],
                                            self.masterDict['gonioMaxvals'][1] + 0.1 * self.masterDict['gonioSteps'][1],
                                            self.masterDict['gonioSteps'][1])
            gonioAxis2values = numpy.arange(self.masterDict['gonioMinvals'][2],
                                            self.masterDict['gonioMaxvals'][2] + 0.1 * self.masterDict['gonioSteps'][2],
                                            self.masterDict['gonioSteps'][2])
            self.iterations = len(gonioAxis0values) * len(gonioAxis1values) * len(gonioAxis2values)
            if self.iterations > 10:
                reply = QtGui.QMessageBox.warning(self, 'Goniometer',
                                                  "More than 10 goniometer settings. This might be long.\n"
                                                  "Are you sure you want to proceed?",
                                                  QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No)
                if reply == QtGui.QMessageBox.No:
                    return

            if self.wg is not None:
                mantid.simpleapi.DeleteWorkspace(self.wg)

            mantid.simpleapi.LoadEmptyInstrument(
                mantid.api.ExperimentInfo.getInstrumentFilename(self.masterDict['instrument']),
                OutputWorkspace="__temp_instrument")
            if self.masterDict['instrument'] == 'HYSPEC':
                mantid.simpleapi.AddSampleLog(Workspace="__temp_instrument", LogName='msd', LogText='1798.5',
                                              LogType='Number Series')
                mantid.simpleapi.AddSampleLog(Workspace="__temp_instrument", LogName='s2',
                                              LogText=str(self.masterDict['S2']), LogType='Number Series')
                mantid.simpleapi.LoadInstrument(Workspace="__temp_instrument", RewriteSpectraMap=True,
                                                InstrumentName="HYSPEC")
            elif self.masterDict['instrument'] == 'EXED':
                mantid.simpleapi.RotateInstrumentComponent(Workspace="__temp_instrument",
                                                           ComponentName='Tank',
                                                           Y=1,
                                                           Angle=str(self.masterDict['S2']),
                                                           RelativeRotation=False)
            # masking
            if 'maskFilename' in self.masterDict and len(self.masterDict['maskFilename'].strip()) > 0:
                try:
                    __maskWS = mantid.simpleapi.Load(self.masterDict['maskFilename'])
                    mantid.simpleapi.MaskDetectors(Workspace="__temp_instrument", MaskedWorkspace=__maskWS)
                except (ValueError, RuntimeError) as e:
                    reply = QtGui.QMessageBox.critical(self, 'Error',
                                                       "The following error has occurred in loading the mask:\n" +
                                                       str(e) + "\nDo you want to continue without mask?",
                                                       QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                                                       QtGui.QMessageBox.No)
                    if reply == QtGui.QMessageBox.No:
                        return
            if self.masterDict['makeFast']:
                sp = list(range(mantid.mtd["__temp_instrument"].getNumberHistograms()))
                tomask = sp[1::4] + sp[2::4] + sp[3::4]
                mantid.simpleapi.MaskDetectors("__temp_instrument", SpectraList=tomask)

            progressDialog = QtGui.QProgressDialog(self)
            progressDialog.setMinimumDuration(0)
            progressDialog.setCancelButtonText("&Cancel")
            progressDialog.setRange(0, self.iterations)
            progressDialog.setWindowTitle("DGSPlanner progress")

            groupingStrings = self._create_goniometer_workspaces(gonioAxis0values, gonioAxis1values, gonioAxis2values,
                                                                 progressDialog)
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
        dimensions = ['Q1', 'Q2', 'Q3', 'DeltaE']
        progressDialog = QtGui.QProgressDialog(self)
        progressDialog.setMinimumDuration(0)
        progressDialog.setCancelButtonText("&Cancel")
        progressDialog.setRange(0, self.iterations)
        progressDialog.setWindowTitle("DGSPlanner progress")
        for i in range(self.iterations):
            progressDialog.setValue(i)
            progressDialog.setLabelText("Calculating orientation %d of %d..." % (i, self.iterations))
            QtGui.qApp.processEvents()
            if progressDialog.wasCanceled():
                self.progress_canceled = True
                progressDialog.close()
                return

            __mdws = mantid.simpleapi.CalculateCoverageDGS(self.wg[i],
                                                           Q1Basis=self.masterDict['dimBasis'][0],
                                                           Q2Basis=self.masterDict['dimBasis'][1],
                                                           Q3Basis=self.masterDict['dimBasis'][2],
                                                           IncidentEnergy=self.masterDict['Ei'],
                                                           Dimension1=dimensions[self.masterDict['dimIndex'][0]],
                                                           Dimension1Min=float2Input(self.masterDict['dimMin'][0]),
                                                           Dimension1Max=float2Input(self.masterDict['dimMax'][0]),
                                                           Dimension1Step=float2Input(self.masterDict['dimStep'][0]),
                                                           Dimension2=dimensions[self.masterDict['dimIndex'][1]],
                                                           Dimension2Min=float2Input(self.masterDict['dimMin'][1]),
                                                           Dimension2Max=float2Input(self.masterDict['dimMax'][1]),
                                                           Dimension2Step=float2Input(self.masterDict['dimStep'][1]),
                                                           Dimension3=dimensions[self.masterDict['dimIndex'][2]],
                                                           Dimension3Min=float2Input(self.masterDict['dimMin'][2]),
                                                           Dimension3Max=float2Input(self.masterDict['dimMax'][2]),
                                                           Dimension4=dimensions[self.masterDict['dimIndex'][3]],
                                                           Dimension4Min=float2Input(self.masterDict['dimMin'][3]),
                                                           Dimension4Max=float2Input(self.masterDict['dimMax'][3]))

            if i == 0:
                intensity = __mdws.getSignalArray()[:, :, 0, 0] * 1.  # to make it writeable
            else:
                if self.colorButton.isChecked():
                    tempintensity = __mdws.getSignalArray()[:, :, 0, 0]
                    intensity[numpy.where(tempintensity > 0)] = i + 1.
                else:
                    tempintensity = __mdws.getSignalArray()[:, :, 0, 0]
                    intensity[numpy.where(tempintensity > 0)] = 1.
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
            self.trajfig.set_aspect(1.)
        else:
            self.trajfig.set_aspect('auto')
        self.trajfig.set_xlabel(self.masterDict['dimNames'][0])
        self.trajfig.set_ylabel(self.masterDict['dimNames'][1])
        self.trajfig.grid(True)
        self.canvas.draw()
        mantid.simpleapi.DeleteWorkspace(__mdws)

    def save(self):
        fileName = str(QtGui.QFileDialog.getSaveFileName(self, 'Save Plot', self.saveDir, '*.png'))
        data = "Instrument " + self.masterDict['instrument'] + '\n'
        if self.masterDict['instrument'] == 'HYSPEC':
            data += "S2 = " + str(self.masterDict['S2']) + '\n'
        data += "Ei = " + str(self.masterDict['Ei']) + ' meV\n'
        data += "Goniometer values:\n"
        gonioAxis0values = numpy.arange(self.masterDict['gonioMinvals'][0], self.masterDict['gonioMaxvals'][0]
                                        + 0.1 * self.masterDict['gonioSteps'][0], self.masterDict['gonioSteps'][0])
        gonioAxis1values = numpy.arange(self.masterDict['gonioMinvals'][1], self.masterDict['gonioMaxvals'][1]
                                        + 0.1 * self.masterDict['gonioSteps'][1], self.masterDict['gonioSteps'][1])
        gonioAxis2values = numpy.arange(self.masterDict['gonioMinvals'][2], self.masterDict['gonioMaxvals'][2]
                                        + 0.1 * self.masterDict['gonioSteps'][2], self.masterDict['gonioSteps'][2])
        for g0 in gonioAxis0values:
            for g1 in gonioAxis1values:
                for g2 in gonioAxis2values:
                    data += "    " + self.masterDict['gonioLabels'][0] + " = " + str(g0)
                    data += "    " + self.masterDict['gonioLabels'][1] + " = " + str(g1)
                    data += "    " + self.masterDict['gonioLabels'][2] + " = " + str(g2) + '\n'
        data += "Lattice parameters:\n"
        data += "    a = " + str(self.ol.a()) + "    b = " + str(self.ol.b()) + "    c = " + str(self.ol.c()) + '\n'
        data += "    alpha = " + str(self.ol.alpha()) + "    beta = " + str(self.ol.beta()) + "    gamma = " + str(
            self.ol.gamma()) + '\n'
        data += "Orientation vectors:\n"
        data += "    u = " + str(self.ol.getuVector()) + '\n'
        data += "    v = " + str(self.ol.getvVector()) + '\n'
        data += "Integrated " + self.masterDict['dimNames'][2] + " between " + \
                str(self.masterDict['dimMin'][2]) + " and " + str(self.masterDict['dimMax'][2]) + '\n'
        data += "Integrated " + self.masterDict['dimNames'][3] + " between " + \
                str(self.masterDict['dimMin'][3]) + " and " + str(self.masterDict['dimMax'][3]) + '\n'

        info = self.figure.text(0.2, 0, data, verticalalignment='top')
        self.figure.savefig(fileName, bbox_inches='tight', additional_artists=info)
        self.saveDir = os.path.dirname(fileName)

    def tr(self, x, y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        # one of the axes is energy
        if self.masterDict['dimIndex'][0] == 3 or self.masterDict['dimIndex'][1] == 3:
            return x, y
        else:
            h1, k1, l1 = (float(temp) for temp in
                          self.masterDict['dimBasis'][self.masterDict['dimIndex'][0]].split(','))
            h2, k2, l2 = (float(temp) for temp in
                          self.masterDict['dimBasis'][self.masterDict['dimIndex'][1]].split(','))
            angle = numpy.radians(self.ol.recAngle(h1, k1, l1, h2, k2, l2))
            return 1. * x + numpy.cos(angle) * y, numpy.sin(angle) * y

    def inv_tr(self, x, y):
        x, y = numpy.asarray(x), numpy.asarray(y)
        # one of the axes is energy
        if self.masterDict['dimIndex'][0] == 3 or self.masterDict['dimIndex'][1] == 3:
            return x, y
        else:
            h1, k1, l1 = (float(temp) for temp in
                          self.masterDict['dimBasis'][self.masterDict['dimIndex'][0]].split(','))
            h2, k2, l2 = (float(temp) for temp in
                          self.masterDict['dimBasis'][self.masterDict['dimIndex'][1]].split(','))
            angle = numpy.radians(self.ol.recAngle(h1, k1, l1, h2, k2, l2))
            return 1. * x - y / numpy.tan(angle), y / numpy.sin(angle)


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    orl = mantid.geometry.OrientedLattice(2, 3, 4, 90, 90, 90)
    mainForm = DGSPlannerGUI()
    mainForm.show()
    sys.exit(app.exec_())
