#------------------------------
# A simple header file to import the Python API for Mantid
#------------------------------
import MantidPythonAPI
mtd = MantidPythonAPI.FrameworkManager()
mantid = mtd
mantid.createPythonSimpleAPI(False)
from mantidsimple import *
