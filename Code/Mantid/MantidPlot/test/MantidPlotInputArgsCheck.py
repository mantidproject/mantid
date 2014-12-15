"""
Test robust handling of input arguments to plotSpectrum, plotBin, and plotMD

All these functions should throw a ValueError exception when, for example:
 - the specified workspaces don't exist
 - the index(es) of spectra, bin or dimension is wrong
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

WorkspaceName2D = 'fake ws'
CreateWorkspace(OutputWorkspace=WorkspaceName2D, DataX=list(X), DataY=list(Y), DataE=list(E), NSpec=2,
                UnitX="TOF", YUnitLabel="Counts",  WorkspaceTitle="Faked data Workspace")

MDWWorkspaceName = 'mdw'
CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace=MDWWorkspaceName)

WrongWorkspaceName = 'foo inexistent'

class MantidPlotInputArgsCheck(unittest.TestCase):

    def setUp(self):
        self.g = None

    def tearDown(self):
        """Clean up by closing the created window """
        windows = self.g
        if type(self.g) != list:
            windows = [self.g]
        
        for window in windows:
            if window != None:
                window.confirmClose(False)
                window.close()
                QtCore.QCoreApplication.processEvents()

    def test_plotSpectrum_ok(self):
        self.g = plotSpectrum(WorkspaceName2D,0)
        self.assertTrue(isinstance(self.g, proxies.Graph))

        self.g = plotSpectrum(WorkspaceName2D,1)
        self.assertTrue(isinstance(self.g, proxies.Graph))

    def test_plotSpectrum_fail_WSName(self):
        try:
            self.assertRaises(ValueError, plotSpectrum(WrongWorkspaceName, 0), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotSpectrum_fail_spectrumNeg(self):
        try:
            self.assertRaises(ValueError, plotSpectrum(WrongWorkspaceName, -2), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotSpectrum_fail_spectrumTooBig(self):
        try:
            self.assertRaises(ValueError, plotSpectrum(WrongWorkspaceName, 3), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotBin_ok(self):
        self.g = plotSpectrum(WorkspaceName2D, 0)
        self.assertTrue(isinstance(self.g, proxies.Graph))

    def test_plotBin_fail_WSName(self):
        try:
            self.assertRaises(ValueError, plotBin(WrongWorkspaceName, 0), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotBin_fail_indexNeg(self):
        try:
            self.assertRaises(ValueError, plotBin(WrongWorkspaceName, -5), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotBin_fail_indexTooBig(self):
        try:
            self.assertRaises(ValueError, plotBin(WrongWorkspaceName, 100), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotMD_fail_NonMD(self):
        try:
            self.assertRaises(ValueError, plotMD(WorkspaceName2D, 0), "wont see this tab")
        except:
            print "Failed, as it should"

    def test_plotMD_fail_nonIntegrated(self):
        try:
            self.assertRaises(ValueError, plotMD(MDWWorkspaceName, 100), "wont see this tab")
        except:
            print "Failed, as it should"

# Run the unit tests
mantidplottests.runTests(MantidPlotInputArgsCheck)
