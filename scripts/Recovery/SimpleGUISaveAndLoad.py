from mantid.simpleapi import *
import tempfile
import os

# True to save, false to load
save = False

dir = tempfile.gettempdir()
name = "recoverytest.mantid"
full_path = os.path.join(dir, name)

adaptor = get_project_recovery_handle()

if save:
    # Save
    ws = CreateSampleWorkspace(OutputWorkspace='sds')
    plot2D(ws)
    for i in range(0, 10):
        plotSpectrum(ws, i)

    adaptor.saveOpenWindows(full_path)
else:
    # Load
    CreateSampleWorkspace(OutputWorkspace='sds')
    adaptor.loadOpenWindows(full_path)
