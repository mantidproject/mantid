import sys
import os
from PyQt4 import Qt
 
# We instantiate a QApplication passing the arguments of the script to it:
a = Qt.QApplication(sys.argv)

sys.path.append( os.getcwd() )
import MantidFramework
from mantidsimple import *

# Create a test data set
CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z', 
    Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace='mdw')
FakeMDEventData("mdw",  UniformParams="1e6")
BinMD("mdw", "uniform",  AxisAligned=1, AlignedDimX="x,0,10,30",  AlignedDimY="y,0,10,30",  AlignedDimZ="z,0,10,30", IterateEvents="1", Parallel="0")

print "CREATED!"

# Create the widget
import libmantidqtpython
sv = libmantidqtpython.MantidQt.SliceViewer.SliceViewer()
sv.setWorkspace('uniform')
sv.show()

# Run the app.
a.exec_()