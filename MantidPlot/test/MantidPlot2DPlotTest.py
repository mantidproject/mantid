# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test of basic 2D plotting methods in MantidPlot
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

CreateWorkspace(OutputWorkspace="fake", DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2,
                UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")

class MantidPlot2DPlotTest(unittest.TestCase):

    def setUp(self):
        self.g = None

    def tearDown(self):
        """Clean up by closing the created window """
        windows = self.g
        if type(self.g) != list:
            windows = [self.g]

        for window in windows:
          window.confirmClose(False)
          window.close()
          QtCore.QCoreApplication.processEvents()

    def test_plot2D_using_string_name(self):
        self.g = plot2D("fake")
        self.assertTrue(isinstance(self.g, proxies.Graph))

    def test_plot2D_using_workspace_handle(self):
        ws = mtd["fake"]
        self.g = plot2D(ws)
        self.assertTrue(isinstance(self.g, proxies.Graph))

    def test_plot2D_multiple_workspaces(self):
        CreateWorkspace(OutputWorkspace="fake2", DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2,
                        UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")
        plots = plot2D(["fake", "fake2"])
        self.g = plots

        self.assertTrue(isinstance(plots, list))
        self.assertEqual(2, len(plots))
        self.assertTrue(isinstance(plots[0], proxies.Graph))
        self.assertTrue(isinstance(plots[1], proxies.Graph))

        plots[-1].confirmClose(False) # for deletion
        DeleteWorkspace("fake2")
        plots.pop()

    def test_plot2D_given_window_reuses_given_window(self):
        win1 = plot2D("fake")
        self.g = plot2D("fake", window=win1)

        open_wins = app.windows()
        self.assertEqual(1, len(open_wins))

# Run the unit tests
mantidplottests.runTests(MantidPlot2DPlotTest)
