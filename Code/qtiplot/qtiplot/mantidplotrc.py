#----------------------------------
# mantidplotrc.py
#
# Load Mantid python API into qtiplot
# by default.
#
# Author Martyn Gigg, Tessella Support Services
#
#----------------------------------
import os

## Check OS
if os.name == 'nt':
    from MantidPythonAPI import *
else:
    from libMantidPythonAPI import *

# Now create simple API (makes mantidsimple.py file in cwd)
createPythonSimpleAPI()
# Import definitions
from mantidsimple import *
