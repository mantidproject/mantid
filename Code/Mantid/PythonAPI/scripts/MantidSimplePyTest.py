import os

if os.name == 'nt':
   import MantidPythonAPI as Mantid
else:
   import libMantidPythonAPI as Mantid

# Creates mantidsimple.py module
Mantid.createPythonSimpleAPI()
from mantidsimple import *

mtd = Mantid.FrameworkManager()

LoadRaw("../../../../Test/Data/HET15869.RAW","test")
ConvertUnits("test","converted","dSpacing")
Rebin("converted","rebinned","0.1,0.001,5")

# clear up intermediate workspaces
mtd.deleteWorkspace("test")
mtd.deleteWorkspace("converted")

# extract the one we want
w = mtd.getWorkspace('rebinned')
print "Rebinned workspace has " + str(w.getNumberHistograms()) + " histograms"
