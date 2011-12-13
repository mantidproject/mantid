import sys
import sys
import os
import unittest
import time
from PyQt4 import Qt
from PyQt4 import QtTest
from PyQt4.QtTest import QTest
import libmantidqtpython

# Create a test data set
CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', 
    Units='m,m,m',SplitInto='5',SplitThreshold=100, MaxRecursionDepth='20',OutputWorkspace='mdw')
FakeMDEventData("mdw",  UniformParams="1e4")
FakeMDEventData("mdw",  PeakParams="1e3, 1, 2, 3, 1.0")
BinMD("mdw", "uniform",  AxisAligned=1, AlignedDimX="x,0,10,30",  AlignedDimY="y,0,10,30",  AlignedDimZ="z,0,10,30", IterateEvents="1", Parallel="0")


class SliceViewerPythonInterfaceTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""
    
    def setUp(self):
        """ Set up and create a SliceViewer widget """
        global libmantidqtpython
        self.sv = libmantidqtpython.MantidQt.SliceViewer.SliceViewer()
        pass
    
    def tearDown(self):
        """ Close the created widget """
        self.sv.close()
	
    def test_setWorkspace(self):
        print "test_setWorkspace"
        sv = self.sv
        sv.setWorkspace('uniform')
        sv.show()
        global QTest
        #QTest.mouseClick(sv)
    
    def test_other(self):
        print "test_other"
        sv = self.sv
        sv.setWorkspace('mdw')
        sv.show()
    
    

# ----- Create and run the unit test ----------------------    
sys.path.append("/home/8oz/Code/Mantid/Code/Mantid/TestingTools/unittest-xml-reporting/src")
import xmlrunner
suite = unittest.makeSuite(SliceViewerPythonInterfaceTest)
runner = xmlrunner.XMLTestRunner(output='Testing')
runner.run(suite)
print "Done!"

