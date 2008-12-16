#----------------------------------
# mantidplotrc.py
#
# Load Mantid python API into qtiplot
# by default.
#
# Author Martyn Gigg, Tessella Support Services
#
#----------------------------------
MantidUIImports = ['newMantidMatrix','plotInstrumentSpectrum','plotTimeBin']

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
# Now create simple API (makes mantidsimple.py file in cwd)
createPythonSimpleAPI(True)
# Import definitions
from mantidsimple import *
