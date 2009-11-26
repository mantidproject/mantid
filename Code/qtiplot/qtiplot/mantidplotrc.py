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

def plotSpectrum(source, indices, error_bars = False):
    if isinstance(source, list):
        workspace_names = __getWorkspaceNames(source)
        if isinstance(indices, list):
            return qti.app.mantidUI.pyPlotSpectraList(workspace_names, indices, error_bars)
        else:
            return qti.app.mantidUI.pyPlotSpectraList(workspace_names, [indices], error_bars)
    else:
        workspace_name  = __getWorkspaceNames([source])
        if isinstance(indices, list):
            return qti.app.mantidUI.pyPlotSpectraList(workspace_name, indices, error_bars)
        else:
            return qti.app.mantidUI.pyPlotSpectraList(workspace_name, [indices], error_bars)

def plotBin(source, indices, error_bars = False):
    return __doPlotting(source,indices,error_bars)

# Legacy function
plotTimeBin = plotBin
#-----------------------------------------------------------------
# Returns a list of workspace names for the given list that could contain workspace proxies
def __getWorkspaceNames(source_list):
    ws_names = []
    for w in source_list:
        if isinstance(w,WorkspaceProxy):
            ws_names.append(w.getName())
        else:
            ws_names.append(w)
    
    return ws_names

# Refactored functions for common code
def __doPlotting(source, indices, error_bars):
    if isinstance(source, list):
        return __PlotList(source, indices, error_bars)
    elif isinstance(source, str) or isinstance(source, WorkspaceProxy):
        return __PlotSingle(source, indices, error_bars)
    else:
        raise TypeError("Source is not a workspace name or a workspace variable")
    
def __PlotSingle(workspace, indices, error_bars):
    '''
    Plot several workspace indices for the given workspace
    '''
    if isinstance(indices, list):
        master_graph = __CallPlotFunction(workspace, indices[0], error_bars)
        for index in indices[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, index, error_bars))
        return master_graph
    else:
        return __CallPlotFunction(workspace, indices, error_bars)
    
def __PlotList(workspace_list, indices, error_bars):
    '''
    Plot the given indices across multiple workspaces on the same graph
    '''
    if isinstance(indices, list):
        master_graph = __CallPlotFunction(workspace_list[0], indices[0], error_bars)
        start = 1
        for workspace in workspace_list:
            for index in indices[start:]:
                mergePlots(master_graph, __CallPlotFunction(workspace, index, error_bars))
                start = 0
                
        return master_graph
    else:
        master_graph = __CallPlotFunction(workspace_list[0], indices, error_bars)
        for workspace in workspace_list[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, indices, error_bars))
        return master_graph

def __CallPlotFunction(workspace, index, error_bars):
    '''Perform a call to the MantidPlot plotSpectrum function
    '''
    if isinstance(workspace, str):
        wkspname = workspace
    else:
        wkspname = workspace.getName()
    return qti.app.mantidUI.plotTimeBin(wkspname, index, error_bars)

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
