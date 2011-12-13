import sys
import sys
import os
import unittest
import time
from PyQt4 import Qt
#from PyQt4.QtTest import QTest
        
# Import the Mantid framework
import MantidFramework
from MantidFramework import mtd
from mantidsimple import *
import libmantidqtpython

# Create the application only once per test; otherwise I get a segfault
app = Qt.QApplication(sys.argv)


class SliceViewerPythonInterfaceTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""
    
    def __init__(self, *args):
        """ Constructor: builda QApplication """
        unittest.TestCase.__init__(self, *args)

        # Create a test data set
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', 
            Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='mdw')
        FakeMDEventData("mdw",  UniformParams="1e4")
        FakeMDEventData("mdw",  PeakParams="1e3, 1, 2, 3, 1.0")
        BinMD("mdw", "uniform",  AxisAligned=1, AlignedDimX="x,0,10,30",  AlignedDimY="y,0,10,30",  AlignedDimZ="z,0,10,30", IterateEvents="1", Parallel="0")

    
    def setUp(self):
        """ Set up and create a SliceViewer widget """
        self.sv = libmantidqtpython.MantidQt.SliceViewer.SliceViewer()
        pass
    
    def tearDown(self):
        """ Close the created widget """
        self.sv.close()
	
    def test_setWorkspace(self):
        sv = self.sv
        sv.setWorkspace('uniform')
        sv.show()
    
    def test_set_MDEventWorkspace(self):
        sv = self.sv
        sv.setWorkspace('mdw')
        sv.show()
    
    
    