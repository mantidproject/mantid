#------------------------------------------------
# mantidplotrc.py
#
# Load Mantid python API into qtiplot
# by default.
#
# Author Martyn Gigg, Tessella Support Services
#
#----------------------------------------------
#
# A tracing function to report the currently executing line number  
#
def traceit(frame, event, arg):
    if event == 'line' and frame.f_globals['__name__'] == '__main__':
        lineno = frame.f_lineno
        qti.app.scriptingInformation(lineno)
    return traceit

# Set the trace function
sys.settrace(traceit)

## Make these functions available globally 
# (i.e. so that the qti.app.mantidUI prefix is not needed)
MantidUIImports = [
    'importMatrixWorkspace',
    'importTableWorkspace',
    'plotSpectrum',
    'plotTimeBin',
    'getMantidMatrix',
    'getInstrumentView', 
    'getSelectedWorkspaceName',
    'mergePlots'
    ]

for name in MantidUIImports:
    setattr(__main__,name,getattr(qti.app.mantidUI,name))

import os
## Linux shared libraries have the 'lib' prefix
if os.name == 'nt':
    from MantidPythonAPI import FrameworkManager
else:
    from libMantidPythonAPI import FrameworkManager

# Create simple API (makes mantidsimple.py file in cwd)
FrameworkManager().createPythonSimpleAPI(True)
# Import definitions to global symbol table
from mantidsimple import *
