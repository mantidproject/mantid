from mantid.simpleapi import *
import tempfile
import os

dir = tempfile.gettempdir()
name = "recoverytest.py"
full_path = os.path.join(dir, name)

# Trivial history
ws = CreateSampleWorkspace(OutputWorkspace='sds')
ws_rename = RenameWorkspace(InputWorkspace=ws)
ws_rebin = Rebin(InputWorkspace=ws_rename, params="0, 0.5, 100")

plot2D(ws_rebin)
plotSpectrum(ws_rebin, 1)

write_workspace_history(ws_rebin, full_path)
