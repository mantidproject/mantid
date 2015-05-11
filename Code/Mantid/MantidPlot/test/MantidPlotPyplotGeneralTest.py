"""General tests for the basic interface of mantidplot.pyplot

Tests correct creation of output lines from plots (with correct
Figure, Graph, etc. data), and proper handling (exception) of wrong
input parameters. Tests plotting of normal arrays and workspaces with the following tools ('tool' kwarg): plot_spectrum, plot_bin, plot_

"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# =============== Create fake workspaces to plot =======================
X1 = np.linspace(0,10, 100)
Y1 = 1000*(np.sin(X1)**2) + X1*10
X1 = np.append(X1, 10.1)

X2 = np.linspace(2,12, 100)
Y2 = 500*(np.cos(X2/2.)**2) + 20
X2 = np.append(X2, 12.10)

X = np.append(X1, X2)
Y = np.append(Y1, Y2)
E = np.sqrt(Y)

# this one has 2 spectra
WorkspaceName2D = 'fake ws'
CreateWorkspace(OutputWorkspace=WorkspaceName2D, DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2,
                UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Test/faked data Workspace, 2 spectra")

sec_X3 = np.linspace(2,12, 100)
sec_Y3 = 200*(np.tan(sec_X3/2.4)**2) + 15
sec_X3 = np.append(sec_X3, 12.10)

sec_X = np.append(X, sec_X3)
sec_Y = np.append(Y, sec_Y3)
sec_E = np.power(sec_Y, 0.6)

# this one has 3 spectra
SecondWorkspaceName2D = 'another fake ws'
CreateWorkspace(OutputWorkspace=SecondWorkspaceName2D, DataX=list(sec_X), DataY=list(sec_Y), DataE=list(sec_E), NSpec=3,
                UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Test/faked data Workspace, 3 spectra")

# plot_md needs an MD workspace with a single non-integrated dimension
MDWWorkspaceName = 'mdw'
mdSignal = np.sin(range(0,100,1))
errInput = mdSignal/20.5
CreateMDHistoWorkspace(Dimensionality="1", Names='x', Units='m', Extents='0,10', NumberOfBins=len(mdSignal), SignalInput=mdSignal, ErrorInput=errInput, OutputWorkspace=MDWWorkspaceName)

class MantidPlotPyplotGeneralTest(unittest.TestCase):

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
        """ Check that the lines returned by a plot are correctly built """
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
        lines_spec = plot_spectrum(WorkspaceName2D, [0, 1])
        self.check_output_lines(lines_spec, 2)
        self.close_win(lines_spec)

        tool_names = ['plot_spectrum', 'plot_sp', 'spectrum', 'sp']
        for tname in tool_names:
            lines = plot(WorkspaceName2D, [0, 1], tool=tname)
            self.check_output_lines(lines, 2)
            self.close_win(lines)

            if self.assertTrue(len(lines) == len(lines_spec)):
                for i in range(0, len(lines)):
                    self.assertEqual(lines[i].get_xdata(), lines_spec[i].get_xdata())
                    self.assertEqual(lines[i].get_ydata(), lines_spec[i].get_ydata())

    def test_plot_bin_ok(self):
        lines_bin = plot_bin(WorkspaceName2D, [0, 1, 2])
        self.check_output_lines(lines_bin, 3)
        self.close_win(lines_bin)

        tool_names = ['plot_bin', 'bin']
        for tname in tool_names:
            lines = plot(WorkspaceName2D, [0, 1, 2], tool=tname)
            self.check_output_lines(lines, 3)
            self.close_win(lines)

            if self.assertTrue(len(lines) == len(lines_bin)):
                for i in range(0, len(lines)):
                    self.assertEqual(lines[i].get_xdata(), lines2_bin[i].get_xdata())
                    self.assertEqual(lines[i].get_ydata(), lines2_bin[i].get_ydata())

    def test_lines_get_data(self):
        y = [0.2, 0.5, 0.1, 0.6]
        # note this assumes that plot will make a dummy workspace using 0,1,2... as X
        x = range(0, len(y), 1)

        lines = plot(y)
        self.check_output_lines(lines, 1)
        # and here also check the values
        if 1==len(lines):
            self.assertEqual(lines[0].get_xdata(), x)
            self.assertEqual(lines[0].get_ydata(), y)
        self.close_win(lines)

    def test_plot_md_ok(self):
        lines = plot_md(MDWWorkspaceName)
        self.assertEqual(len(lines), 1)
        self.close_win(lines)

        tool_names = ['plot_md', 'md']
        for tnames in tool_names:
            lines = plot(MDWWorkspaceName, tool='plot_md')
            self.assertEqual(len(lines), 1)
            self.close_win(lines)

        # now see what happens with non-md workspaces
        try:
            self.assertRaises(ValueError, plot(WorkspaceName2D, tool='plot_md'), "won't see this")
        except:
            print "Failed, as it should"

        try:
            self.assertRaises(ValueError, plot_md(WorkspaceName2D), "won't see this")
        except:
            print "Failed, as it should"

    def test_plot_array_ok(self):
        val = []    # empty data, will be replaced with a dummy (0,0) and generate a 'point' line
        lines = plot(val)
        self.check_output_lines(lines, 1)
        self.close_win(lines)

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

    def test_plot_with_style_args(self):
        # note that these tests also use various aliases for the tool name 'plot_spectrum'
        # Not adding all the possible combinations here, as this suite is becoming time consuming
        lines = plot(WorkspaceName2D, [0,1], '--g', tool='plot_spectrum')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

        lines = plot(WorkspaceName2D, [0,1], 'y:>', tool='plot_sp')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

    def test_plot_with_style_args_and_kwargs(self):
        lines = plot(WorkspaceName2D, [0,1], '-.m', tool='spectrum')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

        lines = plot(WorkspaceName2D, [0,1], 'r-.>', tool='sp')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

    def test_plot_with_kwargs(self):
        lines = plot(WorkspaceName2D, [0,1], tool='plot_spectrum', linewidth=3, linestyle='-.', marker='v')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

        lines = plot(WorkspaceName2D, [0,1], tool='plot_sp', linewidth=3, linestyle='-.', marker='v')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

        lines = plot(SecondWorkspaceName2D, [0,1, 2], tool='sp', linewidth=3, linestyle='-.', marker='v')
        self.check_output_lines(lines, 3)
        self.close_win(lines)

    def test_wrong_kwargs(self):
        # funny kwargs should have no big consequences
        lines = plot(WorkspaceName2D, [0,1], tool='plot_spectrum', linewidth=3, linestyle='-.', marker='v', funny_foo='bar', funny_baz='qux')
        self.check_output_lines(lines, 2)
        self.close_win(lines)

    def test_multi_plot_commands(self):
        lines = plot(WorkspaceName2D, [0,1], SecondWorkspaceName2D, [0, 1, 2])
        self.check_output_lines(lines, 5)
        self.close_win(lines)

        lines = plot(WorkspaceName2D, [0,1], SecondWorkspaceName2D, [0, 1, 2])
        self.check_output_lines(lines, 5)
        self.close_win(lines)

        # this one mixes up positional and kw args
        try:
            self.assertRaises(ValueError, plot(WorkspaceName2D, [0,1], WorkspaceName2D, tool='plot_spectrum'), "wont see this")
        except:
            print "Failed, as it should"

# Run the unit tests
mantidplottests.runTests(MantidPlotPyplotGeneralTest)
