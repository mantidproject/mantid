#------------------------------
# A simple header file to import the Python API for Mantid
#------------------------------
import MantidPythonAPI
MantidPythonAPI.createPythonSimpleAPI(False)
from mantidsimple import *
mtd = MantidPythonAPI.FrameworkManager()
mantid = mtd