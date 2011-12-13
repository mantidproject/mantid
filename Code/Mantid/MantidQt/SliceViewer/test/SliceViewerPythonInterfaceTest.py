import sys
import sys
import os
import unittest
import time

# Import the Mantid framework
sys.path.append( os.getcwd() )
import MantidFramework
from MantidFramework import mtd
from mantidsimple import *

# Create a test data set
CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', 
    Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace='mdw')
FakeMDEventData("mdw",  UniformParams="1e6")
BinMD("mdw", "uniform",  AxisAligned=1, AlignedDimX="x,0,10,30",  AlignedDimY="y,0,10,30",  AlignedDimZ="z,0,10,30", IterateEvents="1", Parallel="0")

w = mtd['uniform']
print "CREATED!", w


from PyQt4 import Qt
import libmantidqtpython


class SliceViewerPythonInterfaceTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""
    def setUp(self):
        self.app =  Qt.QApplication(sys.argv)
        self.sv = libmantidqtpython.MantidQt.SliceViewer.SliceViewer()
        pass
	
    def test_creating(self):
        print "test_creating"
    	import time
        sv = self.sv
        #sv.setWorkspace('mdw')
        sv.show()
        print sv
        #sv.close()
    
    def test_creating2(self):
        print "test_creating2"
    
    
if __name__=="__main__":
    unittest.main()

## ----- Create and run the unit test ----------------------    
#sys.path.append("/home/8oz/Code/Mantid/Code/Mantid/TestingTools/unittest-xml-reporting/src")
#import xmlrunner
#suite = unittest.makeSuite(SliceViewerPythonInterfaceTest)
#runner = xmlrunner.XMLTestRunner(output='Testing')
#runner.run(suite)
#print "Done!"

#
## Run the app.
#a.exec_()
