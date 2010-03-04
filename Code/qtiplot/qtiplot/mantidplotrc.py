#------------------------------------------------
# mantidplotrc.py
#
# Load Mantid Python API into MantidPlot by default.
#
# Author: Martyn Gigg, Tessella Support Services plc
#
#----------------------------------------------
# Make Mantid available
from MantidFramework import *

# Make MantidPlot Python API available to main user scripts.
# For modules imported into a main script users will need to do this too
from mantidplot import *

# Initialize the Mantid framework
FrameworkSingleton().initialise(GUI = True)
