"""
Test of basic 1D plotting methods in MantidPlot
"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# =============== Create a fake workspace to plot =======================
X1 = np.linspace(0,10, 100)
Y1 = 1000*(np.sin(X1)**2) + X1*10
X1 = np.append(X1, 10.1)

X2 = np.linspace(2,12, 100)
Y2 = 500*(np.cos(X2/2.)**2) + 20
X2 = np.append(X2, 12.10)

X = np.append(X1, X2)
Y = np.append(Y1, Y2)
E = np.sqrt(Y)

CreateWorkspace(OutputWorkspace="fake", DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2, UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")

class MantidPlot1DPlotTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        """Clean up by closing the created window """
        if hasattr(self, "g") and self.g is not None:
          self.g.confirmClose(False)
          self.g.close()
        try:
          self.t.confirmClose(False)
          self.t.close()
        except AttributeError:
          pass
        QtCore.QCoreApplication.processEvents()

    def test_plotSpectrum_errorBars(self):
        g = plotSpectrum("fake", 0, error_bars=True)
        self.g = g

    def test_plotSpectrum_fromWorkspaceProxy(self):
        ws = mtd["fake"]
        g = plotSpectrum(ws, 0, error_bars=True)
        self.g = g

    def test_plotSpectrum_severalSpectra(self):
        g = plotSpectrum("fake", [0, 1])
        self.g = g

    def test_Customized1DPlot(self):
        g = plotSpectrum("fake", 0, error_bars=True)
        l = g.activeLayer()
        l.setCurveLineColor(0, QtGui.QColor("orange") )
        l.setCurveLineWidth(0, 2.0)
        l.setTitle('My customized plot of <font face="Symbol">D</font>q')
        l.setTitleFont(QtGui.QFont("Arial", 12))
        l.setTitleColor(QtGui.QColor("red"))
        l.setTitleAlignment(QtCore.Qt.AlignLeft)
        l.setScale(2, 0.0, 3.0)
        l.setAntialiasing(True)
        self.g = g

    def test_standard_plot_command(self):
        t = newTable("test", 30, 4)
        self.t = t
        for i in range(1, t.numRows()+1):
            t.setCell(1, i, i)
            t.setCell(2, i, i)
            t.setCell(3, i, i+2)
            t.setCell(4, i, i+4)

        g = plot(t, (2,3,4), Layer.Line)
        self.g = g
        l = g.activeLayer() # Plot columns 2, 3 and 4
        for i in range(0, l.numCurves()):
            l.setCurveLineColor(i, 1 + i) # Curve color is defined as an integer value. Alternatively, the 2nd argument can be of type QtGui.QColor.
            l.setCurveLineWidth(i, 0.5 + 2*i)

            l.setCurveLineStyle(1, QtCore.Qt.DotLine)
            l.setCurveLineStyle(2, QtCore.Qt.DashLine)

    def test_plotBin_with_single_index(self):
        g = plotBin("fake", 0)
        self.assertTrue(g is not None)
        self.g = g

    def test_plotBin_with_single_index_outside_number_histograms_but_still_valid_produces_plot(self):
        g = plotBin("fake", 5)
        self.assertTrue(g is not None)
        self.g = g

    def test_plotBin_with_invalid_raises_ValueError(self):
        self.assertRaises(ValueError, plotBin, "fake", 100)
        self.g = None

    def test_plotBin_command_with_list(self):
        g = plotBin("fake", [0,1])
        self.assertTrue(g is not None)
        self.g = g

    def test_plotBin_command_with_tuple(self):
        g = plotBin("fake", (0,1))
        self.assertTrue(g is not None)
        self.g = g

# Run the unit tests
mantidplottests.runTests(MantidPlot1DPlotTest)

