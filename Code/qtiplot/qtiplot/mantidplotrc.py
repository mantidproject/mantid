#------------------------------------------------
# mantidplotrc.py
#
# Load Mantid python API into qtiplot
# by default.
#
# Author Martyn Gigg, Tessella Support Services
#
#----------------------------------------------
import os
from MantidFramework import MantidPyFramework, WorkspaceProxy

#-----------------------------------------------------------------

def plotSpectrum(source, indices):
    return __doPlotting(source,indices, binplot = False)
        
def plotBin(source, indices):
    return __doPlotting(source,indices, binplot = True)

# Legacy function
def plotTimeBin(source, indices):
    return plotBin(source,indices)

#-----------------------------------------------------------------
# Refactored functions for common code
def __doPlotting(source, indices, binplot = False):
    if isinstance(source, list):
        return __PlotList(source, indices, binplot)
    elif isinstance(source, str) or isinstance(source, WorkspaceProxy):
        return __PlotSingle(source, indices, binplot)
    else:
        raise TypeError("Source is not a workspace name or a workspace variable")
    
def __PlotSingle(workspace, indices, binplot):
    '''
    Plot several workspace indices for the given workspace
    '''
    if isinstance(indices, list):
        master_graph = __CallPlotFunction(workspace, indices[0], binplot)
        for index in indices[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, index, binplot))
        return master_graph
    else:
        return __CallPlotFunction(workspace, indices, binplot)
    
def __PlotList(workspace_list, indices, binplot):
    '''
    Plot the given indices across multiple workspaces on the same graph
    '''
    if isinstance(indices, list):
        master_graph = __CallPlotFunction(workspace_list[0], indices[0], binplot)
        start = 1
        for workspace in workspace_list:
            for index in indices[start:]:
                mergePlots(master_graph, __CallPlotFunction(workspace, index, binplot))
                start = 0
                
        return master_graph
    else:
        master_graph = __CallPlotFunction(workspace_list[0], indices, binplot)
        for workspace in workspace_list[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, indices, binplot))
        return master_graph

def __CallPlotFunction(workspace, index, binplot):
    '''Perform a call to the MantidPlot plotSpectrum function
    '''
    if isinstance(workspace, str):
        wkspname = workspace
    else:
        wkspname = workspace.getName()
    if binplot:
        return qti.app.mantidUI.plotTimeBin(wkspname, index)
    else:
        return qti.app.mantidUI.plotSpectrum(wkspname, index)

#-----------------------------------------------------------------


## Make these functions available globally 
# (i.e. so that the qti.app.mantidUI prefix is not needed)
MantidUIImports = [
    'importMatrixWorkspace',
    'importTableWorkspace',
    'getMantidMatrix',
    'getInstrumentView', 
    'getSelectedWorkspaceName',
    'mergePlots'
    ]

for name in MantidUIImports:
    setattr(__main__,name,getattr(qti.app.mantidUI,name))

# Create simple API (makes mantidsimple.py file in cwd)
MantidPyFramework().createPythonSimpleAPI(True)
# Import definitions to global symbol table
from mantidsimple import *
