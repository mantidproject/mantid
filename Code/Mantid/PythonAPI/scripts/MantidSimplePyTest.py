#------------------------------
# Import Mantid API
import os

if os.name == 'nt':
   from MantidPythonAPI import *
else:
   from libMantidPythonAPI import *

# Get the FrameworkManager object
mtd = FrameworkManager()
createPythonSimpleAPI()
from mantidsimple import *
#-------------------------------

# Print the available algorithms
mtdHelp()

# Print information on a specific algorithm
mtdHelp("LoadRaw")

# Perform some algorithms
LoadRaw("../../../../Test/Data/HET15869.RAW","test")
ConvertUnits("test","converted","dSpacing")
Rebin("converted","rebinned","0.1,0.001,5")

# clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")

# extract the one we want
w = mtd.getWorkspace('rebinned')

print "Rebinned workspace has " + str(w.getNumberHistograms()) + " histograms"

