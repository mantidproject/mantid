"""
MantidPlot module to gain access to plotting functions etc.
Requires that the main script be run from within MantidPlot 
"""
# Requires MantidPlot
try:
    import qti
except ImportError:
    raise ImportError('The "mantidplot" module can only be used from within MantidPlot.')


import mantidplotpy.proxies as proxies

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import Qt

# Import into the global namespace qti classes that:
#   (a) don't need a proxy & (b) can be constructed from python
from qti import PlotSymbol, ImageSymbol, ArrowMarker, ImageMarker

#-------------------------- Mantid Python access functions----------------
# Grab a few Mantid things so that we can recognise workspace variables
# While we have 2 APIs we need to figure out which to use so add a little bit of indirection
def get_analysis_data_service():
    """Returns an object that can be used to get a workspace by name from Mantid
    
    Returns:
        A object that acts like a dictionary to retrieve workspaces
    """
    import sys
    if 'mantid.api' in sys.modules:
        import mantid
        return mantid.AnalysisDataService.Instance()
    else:
        import MantidFramework
        return MantidFramework.mtd

#-------------------------- Wrapped MantidPlot functions -----------------

# Overload for consistency with qtiplot table(..) & matrix(..) commands
def workspace(name):
    """Get a handle on a workspace.
    
    Args:
        name: The name of the workspace in the Analysis Data Service.
    """
    return get_analysis_data_service()[name]

def table(name):
    """Get a handle on a table.
    
    Args:
        name: The name of the table.
        
    Returns:
        A handle to the table.
    """
    return proxies.MDIWindow(qti.app.table(name))

def newTable(name=None,rows=30,columns=2):
    """Create a table.
    
    Args:
        name: The name to give to the table (if None, a unique name will be generated).
        row: The number of rows in the table (default: 30).
        columns: The number of columns in the table (default: 2).
        
    Returns:
        A handle to the created table.
    """
    if name is None:
        return proxies.MDIWindow(qti.app.newTable())
    else:
        return proxies.MDIWindow(qti.app.newTable(name,rows,columns))

def matrix(name):
    """Get a handle on a matrix.
    
    Args:
        name: The name of the matrix.
        
    Returns:
        A handle to the matrix.
    """
    return proxies.MDIWindow(qti.app.matrix(name))

def newMatrix(name=None,rows=32,columns=32):
    """Create a matrix (N.B. This is not the same as a 'MantidMatrix').
    
    Args:
        name: The name to give to the matrix (if None, a unique name will be generated).
        row: The number of rows in the matrix (default: 32).
        columns: The number of columns in the matrix (default: 32).
        
    Returns:
        A handle to the created matrix.
    """
    if name is None:
        return proxies.MDIWindow(qti.app.newMatrix())
    else:
        return proxies.MDIWindow(qti.app.newMatrix(name,rows,columns))

def graph(name):
    """Get a handle on a graph widget.
    
    Args:
        name: The name of the graph window.
        
    Returns:
        A handle to the graph.
    """
    return proxies.Graph(qti.app.graph(name))

def newGraph(name=None,layers=1,rows=1,columns=1):
    """Create a graph window.
    
    Args:
        name: The name to give to the graph (if None, a unique name will be generated).
        layers: The number of plots (a.k.a. layers) to put in the graph window (default: 1).
        rows: The number of rows of to put in the graph window (default: 1).
        columns: The number of columns of to put in the graph window (default: 1).
        
    Returns:
        A handle to the created graph widget.
    """
    if name is None:
        return proxies.Graph(qti.app.newGraph())
    else:
        return proxies.Graph(qti.app.newGraph(name,layers,rows,columns))

def note(name):
    """Get a handle on a note.
    
    Args:
        name: The name of the note.
        
    Returns:
        A handle to the note.
    """
    return proxies.MDIWindow(qti.app.note(name))

def newNote(name=None):
    """Create a note.
    
    Args:
        name: The name to give to the note (if None, a unique name will be generated).
        
    Returns:
        A handle to the created note.
    """
    if name is None:
        return proxies.MDIWindow(qti.app.newNote())
    else:
        return proxies.MDIWindow(qti.app.newNote(name))

#-----------------------------------------------------------------------------
# Intercept qtiplot "plot" command and forward to plotSpectrum for a workspace
def plot(source, *args, **kwargs):
    """Create a new plot given a workspace, table or matrix.
    
    Args:
        source: what to plot; if it is a Workspace, will
                call plotSpectrum()
    
    Returns:
        A handle to the created Graph widget.
    """
    if hasattr(source, '_getHeldObject') and isinstance(source._getHeldObject(), QtCore.QObject):
        return proxies.Graph(qti.app.plot(source._getHeldObject(), *args, **kwargs))
    else:
        return plotSpectrum(source, *args, **kwargs)
        
#-----------------------------------------------------------------------------
def plotSpectrum(source, indices, error_bars = False, type = -1):
    """Open a 1D Plot of a spectrum in a workspace.
    
    This plots one or more spectra, with X as the bin boundaries,
    and Y as the counts in each bin.
    
    Args:
        source: workspace or name of a workspace
        indices: workspace index, or tuple or list of workspace indices to plot
        error_bars: bool, set to True to add error bars.
    """
    workspace_names = __getWorkspaceNames(source)
    index_list = __getWorkspaceIndices(indices)
    if len(workspace_names) > 0 and len(index_list) > 0:
        return __tryPlot(workspace_names, index_list, error_bars, type)
    else:
        return None
    

#-----------------------------------------------------------------------------
def plotBin(source, indices, error_bars = False, type = 0):
    """Create a 1D Plot of bin count vs spectrum in a workspace.
    
    This puts the spectrum number as the X variable, and the
    count in the particular bin # (in 'indices') as the Y value.
    
    If indices is a tuple or list, then several curves are created, one
    for each bin index.
    
    Args:
        source: workspace or name of a workspace
        indices: bin number(s) to plot
        error_bars: bool, set to True to add error bars.

    Returns:
        A handle to the created Graph widget.
    """
    return __doPlotting(source,indices,error_bars,type)

#-----------------------------------------------------------------------------
def stemPlot(source, index, power=None, startPoint=None, endPoint=None):
    """Generate a stem-and-leaf plot from an input table column or workspace spectrum
    
    Args:
        source: A reference to a workspace or a table.
        index: For a table, the column number or name. For a workspace, the workspace index.
        power: The stem unit as a power of 10. If not provided, a dialog will appear with a
            suggested value.
        startPoint: The first point (row or bin) to use (Default: the first one).
        endPoint: The last point (row or bin) to use (Default: the last one).
        
    Returns:
        A string representation of the stem plot
    """
    # Turn the optional arguments into the magic numbers that the C++ expects
    if power==None:
        power=1001
    if startPoint==None:
        startPoint=0
    if endPoint==None:
        endPoint=-1
    
    if isinstance(source,proxies.QtProxyObject):
        source = source._getHeldObject()
    elif hasattr(source, 'getName'):
        # If the source is a workspace, create a table from the specified index
        wsName = source.getName()
        source = qti.app.mantidUI.workspaceToTable(wsName,wsName,[index],False,True)
        # The C++ stemPlot method takes the name of the column, so get that
        index = source.colName(2)
    # Get column name if necessary
    if isinstance(index, int):
        index = source.colName(index)
    # Call the C++ method
    return qti.app.stemPlot(source,index,power,startPoint,endPoint)

#-----------------------------------------------------------------------------
def waterfallPlot(table, columns):
    """Create a waterfall plot from data in a table.
    
    Args:
        table: A reference to the table containing the data to plot
        columns: A tuple of the column numbers whose data to plot
        
    Returns:
        A handle to the created plot (Layer).
    """
    return proxies.Graph(qti.app.waterfallPlot(table._getHeldObject(),columns))

#-----------------------------------------------------------------------------
def importImage(filename):
    """Load an image file into a matrix.
    
    Args:
        filename: The name of the file to load.
        
    Returns:
        A handle to the matrix containing the image data.
    """
    return proxies.MDIWindow(qti.app.importImage(filename))

#-----------------------------------------------------------------------------
def newPlot3D():
    return proxies.Graph3D(qti.app.newPlot3D())

def plot3D(*args):
    if isinstance(args[0],str):
        return proxies.Graph3D(qti.app.plot3D(*args))
    else:
        return proxies.Graph3D(qti.app.plot3D(args[0]._getHeldObject(),*args[1:]))

#-----------------------------------------------------------------------------
def selectMultiPeak(source, showFitPropertyBrowser = True):
    """Switch on the multi-peak selecting tool for fitting with the Fit algorithm.
    
    Args:
        source: A reference to a MultiLayer with the data to fit.
        showFitPropertyBrowser: Whether to show the FitPropertyBrowser or not.
    """
    qti.app.selectMultiPeak(source._getHeldObject(), showFitPropertyBrowser)

#-----------------------------------------------------------------------------
#-------------------------- Project/Folder functions -----------------------
#-----------------------------------------------------------------------------
def windows():
    """Get a list of the open windows."""
    f = qti.app.windows()
    ret = []
    for item in f:
        ret.append(proxies.MDIWindow(item))
    return ret

def activeFolder():
    """Get a handle to the currently active folder."""
    return proxies.Folder(qti.app.activeFolder())

# These methods don't seem to work
#def appendProject(filename, parentFolder=None):
#    if parentFolder is not None:
#        parentFolder = parentFolder._getHeldObject()
#    return proxies.Folder(qti.app.appendProject(filename,parentFolder))
#
#def saveFolder(folder, filename, compress=False):
#    qti.app.saveFolder(folder._getHeldObject(),filename,compress)

def rootFolder():
    """Get a handle to the top-level folder."""
    return proxies.Folder(qti.app.rootFolder())

def addFolder(name,parentFolder=None):
    """Create a new folder.
    
    Args:
        name: The name of the folder to create.
        parentFolder: If given, make the new folder a subfolder of this one.
        
    Returns:
        A handle to the newly created folder.
    """
    if parentFolder is not None:
        parentFolder = parentFolder._getHeldObject()
    return proxies.Folder(qti.app.addFolder(name,parentFolder))

def deleteFolder(folder):
    """Delete the referenced folder"""
    return qti.app.deleteFolder(folder._getHeldObject())

def changeFolder(folder, force=False):
    """Changes the current folder.
    
    Args:
        folder: A reference to the folder to change to.
        force: Whether to do stuff even if the new folder is already the active one (default: no).
    
    Returns:
        True on success.
    """
    return qti.app.changeFolder(folder._getHeldObject(),force)

def copyFolder(source, destination):
    """Copy a folder (and its contents) into another.
    
    Returns:
        True on success.
    """
    return qti.app.copyFolder(source._getHeldObject(),destination._getHeldObject())

def openTemplate(filename):
    """Load a previously saved window template"""
    return proxies.MDIWindow(qti.app.openTemplate(filename))

def saveAsTemplate(window, filename):
    """Save the characteristics of the given window to file"""
    qti.app.saveAsTemplate(window._getHeldObject(), filename)

def setWindowName(window, name):
    """Set the given window to have the given name"""
    qti.app.setWindowName(window._getHeldObject(), name)

def setPreferences(layer):
    qti.app.setPreferences(graph._getHeldObject())

def clone(window):
    return proxies.MDIWindow(qti.app.clone(window._getHeldObject()))

def tableToMatrix(table):
    return proxies.MDIWindow(qti.app.tableToMatrix(table._getHeldObject()))

def matrixToTable(matrix, conversionType=qti.app.Direct):
    return proxies.MDIWindow(qti.app.matrixToTable(matrix._getHeldObject(),conversionType))

#-----------------------------------------------------------------------------
#-------------------------- Wrapped MantidUI functions -----------------------
#-----------------------------------------------------------------------------

def mergePlots(graph1,graph2):
    """Combine two graphs into a single plot"""
    return proxies.Graph(qti.app.mantidUI.mergePlots(graph1._getHeldObject(),graph2._getHeldObject()))

def convertToWaterfall(graph):
    """Convert a graph (containing a number of plotted spectra) to a waterfall plot"""
    qti.app.mantidUI.convertToWaterfall(graph._getHeldObject())

def getMantidMatrix(name):
    """Get a handle to the named Mantid matrix"""
    return proxies.MantidMatrix(qti.app.mantidUI.getMantidMatrix(name))

def getInstrumentView(name, tab=-1):
    """Create an instrument view window based on the given workspace.
    
    Args:
        name: The name of the workspace.
        tab: The index of the tab to display initially.
        
    Returns:
        A handle to the created instrument view widget.
    """
    return proxies.MDIWindow(qti.app.mantidUI.getInstrumentView(name,tab))

def importMatrixWorkspace(name, firstIndex=None, lastIndex=None, showDialog=False, visible=False):
    """Create a MantidMatrix object from the named workspace.
    
    Args:
        name: The name of the workspace.
        firstIndex: The workspace index to start at (default: the first one).
        lastIndex: The workspace index to stop at (default: the last one).
        showDialog: Whether to bring up a dialog to allow options to be entered manually (default: no).
        visible: Whether to initially show the created matrix (default: no).
        
    Returns:
        A handle to the created matrix.
    """
    # Turn the optional arguments into the magic numbers that the C++ expects
    if firstIndex is None:
        firstIndex = -1
    if lastIndex is None:
        lastIndex = -1
    return proxies.MantidMatrix(qti.app.mantidUI.importMatrixWorkspace(name,firstIndex,lastIndex,showDialog,visible))

def importTableWorkspace(name, visible=False):
    """Create a MantidPlot table from a table workspace.
    
    Args:
        name: The name of the workspace.
        visible: Whether to initially show the created matrix (default: no).
        
    Returns:
        A handle to the newly created table.
    """
    return proxies.MDIWindow(qti.app.mantidUI.importTableWorkspace(name,False,visible))

#-----------------------------------------------------------------------------
#-------------------------- SliceViewer functions ----------------------------
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
def plotSlice(source, label="", xydim=None, slicepoint=None,
                    colormin=None, colormax=None, colorscalelog=False,
                    limits=None, **kwargs):
    """Opens the SliceViewer with the given MDWorkspace(s).
    
    Args:
        source :: one workspace, or a list of workspaces
        
    Optional Keyword Args:
        label :: label for the window title
        xydim :: indexes or names of the dimensions to plot,
            as an (X,Y) list or tuple.
            See SliceViewer::setXYDim()
        slicepoint :: list with the slice point in each dimension. Must be the 
            same length as the number of dimensions of the workspace.
            See SliceViewer::setSlicePoint()
        colormin :: value of the minimum color in the scale
            See SliceViewer::setColorScaleMin()
        colormax :: value of the maximum color in the scale
            See SliceViewer::setColorScaleMax()
        colorscalelog :: value of the maximum color in the scale
            See SliceViewer::setColorScaleLog()
        limits :: list with the (xleft, xright, ybottom, ytop) limits
            to the view to show.
            See SliceViewer::setXYLimits()
        
    Returns:
        a (list of) handle(s) to the SliceViewerWindow widgets that were open.
            Use SliceViewerWindow.getSlicer() to get access to the functions of the
            SliceViewer, e.g. setting the view and slice point.
    """ 
    workspace_names = __getWorkspaceNames(source)
    try:
        import mantidqtpython
    except:
        print "Could not find module mantidqtpython. Cannot open the SliceViewer."
        return
    
    # Make a list of widgets to return
    out = []
    for wsname in workspace_names:
        window = __doSliceViewer(wsname, label=label,
           xydim=xydim, slicepoint=slicepoint, colormin=colormin,
           colormax=colormax, colorscalelog=colorscalelog, limits=limits, 
           **kwargs) 
        pxy = proxies.SliceViewerWindowProxy(window)
        out.append(pxy)
        
    # Return the widget alone if only 1
    if len(out) == 1:
        return out[0]
    else:
        return out


#-----------------------------------------------------------------------------
def getSliceViewer(source, label=""):
    """Retrieves a handle to a previously-open SliceViewerWindow.
    This allows you to get a handle on, e.g., a SliceViewer that was open
    by the MultiSlice view in VATES Simple Interface.
    Will raise an exception if not found.
    
    Args:
        source :: name of the workspace that was open
        label :: additional label string that was used to identify the window.
        
    Returns:
        a handle to the SliceViewerWindow object that was created before. 
    """
    import mantidqtpython
    workspace_names = __getWorkspaceNames(source)
    if len(workspace_names) != 1:
        raise Exception("Please specify only one workspace.")
    else:
        svw = mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().getSliceViewerWindow(workspace_names[0], label)
        return proxies.SliceViewerWindowProxy(svw)


#-----------------------------------------------------------------------------
def closeAllSliceViewers():
    """
    Closes all currently open SliceViewer windows. This might be useful to 
    clean up your desktop after opening many windows.
    """
    import mantidqtpython
    mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().closeAllSliceViewerWindows()

#-----------------------------------------------------------------------------
# Legacy function
plotTimeBin = plotBin

# import 'safe' methods (i.e. no proxy required) of ApplicationWindow into the global namespace
# Only 1 at the moment!
appImports = [
        'saveProjectAs'
        ]
for name in appImports:
    globals()[name] = getattr(qti.app,name)
        
# Ensure these functions are available as without the qti.app.mantidUI prefix
MantidUIImports = [
    'getSelectedWorkspaceName'
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





#-----------------------------------------------------------------------------
#--------------------------- "Private" functions -----------------------------
#-----------------------------------------------------------------------------


#-----------------------------------------------------------------------------
def __getWorkspaceNames(source):
    """Takes a "source", which could be a WorkspaceGroup, or a list
    of workspaces, or a list of names, and converts
    it to a list of workspace names.
    
    Args:
        source :: input list or workspace group
        
    Returns:
        list of workspace names 
    """
    ws_names = []
    if isinstance(source, list) or isinstance(source,tuple):
        for w in source:
            names = __getWorkspaceNames(w)
            ws_names += names
    elif hasattr(source, 'getName'):
        if hasattr(source, '_getHeldObject'):
            wspace = source._getHeldObject()
        else:
            wspace = source
        if wspace == None:
            return []
        if hasattr(wspace, 'getNames'):
            grp_names = wspace.getNames()
            for n in grp_names:
                if n != wspace.getName():
                    ws_names.append(n)
        else:
            ws_names.append(wspace.getName())
    elif isinstance(source,str):
        w = get_analysis_data_service()[source]
        if w != None:
            names = __getWorkspaceNames(w)
            for n in names:
                ws_names.append(n)
    else:
        raise TypeError('Incorrect type passed as workspace argument "' + str(source) + '"')
    return ws_names
    
#-----------------------------------------------------------------------------
def __doSliceViewer(wsname, label="", xydim=None, slicepoint=None,
                    colormin=None, colormax=None, colorscalelog=False,
                    limits=None):
    """Open a single SliceViewerWindow for the workspace, and shows it
    
    Args:
        wsname :: name of the workspace
    See plotSlice() for full list of keyword parameters.
        
    Returns:
        A handle to the created SliceViewerWindow widget
    """
    import mantidqtpython
    from PyQt4 import QtCore
    
    svw = mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().createSliceViewerWindow(wsname, label)
    svw.show()
    
    # -- Connect to main window's shut down signal ---
    QtCore.QObject.connect(qti.app, QtCore.SIGNAL("shutting_down()"),
                    svw, QtCore.SLOT("close()"))
    
    sv = svw.getSlicer()
    # --- X/Y Dimensions ---
    if (not xydim is None):
        if len(xydim) != 2:
            raise Exception("You need to specify two values in the 'xydim' parameter")
        else:
            sv.setXYDim(xydim[0], xydim[1])
    
    # --- Slice point ---
    if not slicepoint is None:
        for d in xrange(len(slicepoint)): 
            try:
                val = float(slicepoint[d])
            except ValueError:
                raise ValueError("Could not convert item %d of slicepoint parameter to float (got '%s'" % (d, slicepoint[d]))
            sv.setSlicePoint(d, val)     
    
    # --- Color scale ---
    if (not colormin is None) and (not colormax is None):
        sv.setColorScale(colormin, colormax, colorscalelog)
    else:
        if (not colormin is None): sv.setColorScaleMin(colormin)
        if (not colormax is None): sv.setColorScaleMax(colormax)
    sv.setColorScaleLog(colorscalelog)
    
    # --- XY limits ---
    if not limits is None:
        sv.setXYLimits(limits[0], limits[1], limits[2], limits[3])
    
    return svw


    
    
    
#-----------------------------------------------------------------------------
def __getWorkspaceIndices(source):
    index_list = []
    if isinstance(source,list) or isinstance(source,tuple):
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
def __tryPlot(workspace_names, indices, error_bars, type):
    graph = proxies.Graph(qti.app.mantidUI.pyPlotSpectraList(workspace_names, indices, error_bars, type))
    if graph._getHeldObject() == None:
        raise RuntimeError("Cannot create graph, see log for details.")
    else:
        return graph
        

# Refactored functions for common code
def __doPlotting(source, indices, error_bars,type):
    if isinstance(source, list) or isinstance(source, tuple):
        return __PlotList(source, indices, error_bars,type)
    elif isinstance(source, str) or hasattr(source, 'getName'):
        return __PlotSingle(source, indices, error_bars,type)
    else:
        raise TypeError("Source is not a workspace name or a workspace variable")
    
def __PlotSingle(workspace, indices, error_bars,type):
    if isinstance(indices, list) or isinstance(indices, tuple):
        master_graph = __CallPlotFunction(workspace, indices[0], error_bars,type)
        for index in indices[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, index, error_bars,type))
        return master_graph
    else:
        return __CallPlotFunction(workspace, indices, error_bars,type)
    
def __PlotList(workspace_list, indices, error_bars,type):
    if isinstance(indices, list) or isinstance(indices, tuple):
        master_graph = __CallPlotFunction(workspace_list[0], indices[0], error_bars,type)
        start = 1
        for workspace in workspace_list:
            for index in indices[start:]:
                mergePlots(master_graph, __CallPlotFunction(workspace, index, error_bars,type))
                start = 0
                
        return master_graph
    else:
        master_graph = __CallPlotFunction(workspace_list[0], indices, error_bars,type)
        for workspace in workspace_list[1:]:
            mergePlots(master_graph, __CallPlotFunction(workspace, indices, error_bars,type))
        return master_graph

def __CallPlotFunction(workspace, index, error_bars,type):
    if isinstance(workspace, str):
        wkspname = workspace
    else:
        wkspname = workspace.getName()
    return proxies.Graph(qti.app.mantidUI.plotBin(wkspname, index, error_bars,type))
