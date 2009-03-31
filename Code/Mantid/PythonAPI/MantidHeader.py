#---------------------------------------
# A header file to create and 
# import the Python API for Mantid
#--------------------------------------
import os
if os.name == 'nt':
    from MantidPythonAPI import FrameworkManager
else:
    from libMantidPythonAPI import FrameworkManager
# False means that no GUI stuff is needed
FrameworkManager().createPythonSimpleAPI(False)
from mantidsimple import *
