"""General tests for the basic interface of mantidplot.future.pyplot

Tests correct creation of output lines from plots (with correct
Figure, Graph, etc. data), and proper handling (exception) of wrong
input parameters. Tests plotting of normal arrays and workspaces with the following tools ('tool' kwarg): plot_spectrum, plot_bin, plot_

"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# Future pyplot
from mantidplot.future.pyplot import *

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

WorkspaceName2D = 'fake ws'
CreateWorkspace(OutputWorkspace=WorkspaceName2D, DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2,
                UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")

MDWWorkspaceName = 'mdw'
CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace=MDWWorkspaceName)

class MantidPlotFuturePyplotGeneralTest(unittest.TestCase):

    def setUp(self):
        self.g = None

    def tearDown(self):
        """Clean up by closing the created window """
        windows = self.g
        if not self.g:
            return
        if type(self.g) != list:
            windows = [self.g]
        for window in windows:
            self.close_win_by_graph(window)

    def close_win_by_graph(self, g):
        if None != g:
            g.confirmClose(False)
            g.close()
            QtCore.QCoreApplication.processEvents()

    def test_nothing(self):
        return True

    def check_output_lines(self, lines, expected_len):
        self.assertTrue(expected_len==len(lines))
        for i in range(0, len(lines)):
            self.assertTrue(isinstance(lines[i], Line2D))
            self.assertTrue(isinstance(lines[i].figure(), Figure))
            self.assertTrue(isinstance(lines[i]._graph, proxies.Graph))
            self.assertTrue(isinstance(lines[i].figure()._graph, proxies.Graph))

    def close_win(self, lines):
        if len(lines) > 0:
            self.close_win_by_graph(lines[0]._graph)        

    def test_plot_spectrum_ok(self):
        lines = plot(WorkspaceName2D, [0, 1], tool='plot_spectrum')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

        lines = plot_spectrum(WorkspaceName2D, [0, 1], tool='plot_spectrum')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

    def test_plot_bin_ok(self):
        lines = plot(WorkspaceName2D, [0, 1, 2], tool='plot_bin')
        self.check_output_lines(lines, 3)
        self.close_win(lines)

        # same with plot_bin

    def test_plot_md_ok(self):
        return
        # lines = plot(MDWWorkspaceName, tool='plot_md')
        # self.close_win(lines)

        # lines = plot_md(MDWWorkspaceName)
        # self.close_win(lines)


    def test_plot_array_ok(self):
        val = []    # empty data, should be replaced with a dummy (0,0) and generate a 'point' line
        lines = plot(val)
        self.check_output_lines(lines, 1)
        self.close_win(lines)
        # 

    def test_plot_with_more_functions(self):
        lines = plot(WorkspaceName2D, [0,1], tool='plot_spectrum', linewidth=2, linestyle='--', marker='v')
        xlim(0, 1)
        ylim(0, 1)
        xlabel('X foo label')
        ylabel('Y bar label')
        title('baz title')
        axis([0, 1, 0, 1])
        grid('off')
        grid('on')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

    def test_plot_with_kwargs(self):
        lines = plot(WorkspaceName2D, [0,1], tool='plot_spectrum', linewidth=3, linestyle='-.', marker='v')
        self.check_output_lines(lines, 2)
        self.close_win(lines)


# Run the unit tests
mantidplottests.runTests(MantidPlotFuturePyplotGeneralTest)
