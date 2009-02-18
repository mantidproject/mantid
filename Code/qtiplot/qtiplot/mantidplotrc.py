#----------------------------------
# mantidplotrc.py
#
# Load Mantid python API into qtiplot
# by default.
#
# Author Martyn Gigg, Tessella Support Services
#
#----------------------------------
MantidUIImports = ['newMantidMatrix','plotSpectrum','plotTimeBin','getMantidMatrix','getInstrumentView']

for name in MantidUIImports:
    setattr(__main__,name,getattr(qti.app.mantidUI,name))

import os

## Check OS
if os.name == 'nt':
    from MantidPythonAPI import *
else:
    from libMantidPythonAPI import *

# Ensure all algorithm libraries are loaded and get the FrameworkManager object
mtd = FrameworkManager()
# Have an alias
mantid = mtd
# Now create simple API (makes mantidsimple.py file in cwd)
createPythonSimpleAPI(True)
# Import definitions
from mantidsimple import *
