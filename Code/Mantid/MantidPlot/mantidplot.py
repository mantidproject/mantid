"""
MantidPlot module to gain access to plotting functions etc.
Requires that the main script be run from within MantidPlot 
"""
# Requires MantidPlot
try:
    import qti
except ImportError:
    raise ImportError('The "mantidplot" module can only be used from within MantidPlot.')

# Grab a few Mantid things so that we can recognise workspace variables
from MantidFramework import WorkspaceProxy, WorkspaceGroup, MatrixWorkspace, mtd

#-------------------------- Wrapped MantidPlot functions -----------------

def plotSpectrum(source, indices, error_bars = False):
    workspace_names = __getWorkspaceNames(source)
    index_list = __getWorkspaceIndices(indices)
    if len(workspace_names) > 0 and len(index_list) > 0:
        return __tryPlot(workspace_names, index_list, error_bars)
    else:
        return None

def plotBin(source, indices, error_bars = False):
    return __doPlotting(source,indices,error_bars)

# Legacy function
plotTimeBin = plotBin

# Ensure these functions are available as without the qti.app.mantidUI prefix
MantidUIImports = [
    'importMatrixWorkspace',
    'importTableWorkspace',
    'getMantidMatrix',
    'getInstrumentView', 
    'getSelectedWorkspaceName',
    'mergePlots'
    ]
# Update globals
for name in MantidUIImports:
    globals()[name] = getattr(qti.app.mantidUI,name)
    
# Set some aliases for Layer enumerations so that old code will still work
Layer = qti.Layer
Layer.Log10 = qti.GraphOptions.Log10
Layer.Linear = qti.GraphOptions.Linear
Layer.Left = qti.GraphOptions.Left
Layer.Right = qti.GraphOptions.Right
Layer.Bottom = qti.GraphOptions.Bottom
Layer.Top = qti.GraphOptions.Top

#--------------------------- "Private" functions -----------------------

def __getWorkspaceNames(source):
    ws_names = []
    if isinstance(source, list):
        for w in source:
            names = __getWorkspaceNames(w)
            for n in names:
                ws_names.append(n)
    elif isinstance(source,WorkspaceProxy):
        wspace = source._getHeldObject()
        if wspace == None:
            return []
        if isinstance(wspace,WorkspaceGroup):
            grp_names = source.getNames()
            for n in grp_names:
                if n != wspace.getName():
                    ws_names.append(n)
        elif isinstance(wspace,MatrixWorkspace):
            ws_names.append(wspace.getName())
        else:
            pass
    elif isinstance(source,str):
        w = mtd[source]
        if w != None:
            names = __getWorkspaceNames(w)
            for n in names:
                ws_names.append(n)
    else:
        raise TypeError('Incorrect type passed as workspace argument "' + str(source) + '"')
    return ws_names
    
def __getWorkspaceIndices(source):
    index_list = []
    if isinstance(source,list):
        for i in source:
            nums = __getWorkspaceIndices(i)
            for j in nums:
                index_list.append(j)
    elif isinstance(source, int):
        index_list.append(source)
    elif isinstance(source, str):
        elems = source.split(',')
        for i in elems:
            try:
                index_list.append(int(i))
            except ValueError:
                pass
    else:
        raise TypeError('Incorrect type passed as index argument "' + str(source) + '"')
    return index_list

# Try plotting, raising an error if no plot object is created
def __tryPlot(workspace_names, indices, error_bars):
    graph = qti.app.mantidUI.pyPlotSpectraList(workspace_names, indices, error_bars)
    if graph == None:
        raise RuntimeError("Cannot create graph, see log for details.")
    else:
        return graph
        

# Refactored functions for common code
def __doPlotting(source, indices, error_bars):
    if isinstance(source, list):
        return __PlotList(source, indices, error_bars)
    elif isinstance(source, str) or isinstance(source, WorkspaceProxy):
        return __PlotSingle(source, indices, error_bars)
    else:
        raise TypeError("Source is not a workspace name or a workspace variable")
    
def __PlotSingle(workspace, indices, error_bars):
    if isinstance(indices, list):
        master_graph = __CallPlotFunction(workspace, indices[0], error_bars)
        for index in indices[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, index, error_bars))
        return master_graph
    else:
        return __CallPlotFunction(workspace, indices, error_bars)
    
def __PlotList(workspace_list, indices, error_bars):
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
    if isinstance(workspace, str):
        wkspname = workspace
    else:
        wkspname = workspace.getName()
    return qti.app.mantidUI.plotBin(wkspname, index, error_bars)
