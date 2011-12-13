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
        #sv.show()
    
    def test_setWorkspace_MDEventWorkspace(self):
        sv = self.sv
        sv.setWorkspace('mdw')
        #sv.show()
    
    def test_setXYDim(self):
        sv = self.sv
        sv.setWorkspace('uniform')
        sv.setXYDim(0,2)
        self.assertEqual( sv.getDimX(), 0, "X dimension was set")
        self.assertEqual( sv.getDimY(), 2, "Y dimension was set")
        #sv.show()
        #app.exec_()
        
    def test_setSlicePoint(self):
        sv = self.sv
        sv.setWorkspace('uniform')
        # Set the slice point and got back the value?
        sv.setSlicePoint(2, 7.6)
        self.assertAlmostEqual( sv.getSlicePoint(2), 7.6, 2)
        # Go to too small a value
        sv.setSlicePoint(2, -12.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 0.0, 2)
        # Go to too big a value
        sv.setSlicePoint(2, 22.3)
        self.assertAlmostEqual( sv.getSlicePoint(2), 10.0, 2)
                
    def test_setSlicePoint_throwsOnBadInputs(self):
        sv = self.sv
        sv.setWorkspace('uniform')
        sv.setSlicePoint(-3, 37.6)
        try:
            sv.setSlicePoint(-1, 7.6)
        except:
            print "error caught"
#        with self.assertRaises(ValueError): 
#            sv.setSlicePoint(-1, 7.6)
        #self.assertRaises(Exception, sv.setSlicePoint, (3, 7.6))
        
#        sv.show()
#        app.exec_()
    
    
    
    