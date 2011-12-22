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
from MantidFramework import WorkspaceProxy, WorkspaceGroup, MatrixWorkspace, mtd, ProxyObject
from PyQt4 import QtCore

#-------------------------- Wrapped MantidPlot functions -----------------

# Overload for consistency with qtiplot table(..) & matrix(..) commands
def workspace(name):
    return mtd[name]


#-----------------------------------------------------------------------------
# Intercept qtiplot "plot" command and forward to plotSpectrum for a workspace
def plot(source, *args, **kwargs):
    """Plot a workspace."""
    if isinstance(source,WorkspaceProxy):
        return plotSpectrum(source, *args, **kwargs)
    else:
        return qti.app.plot(source, *args, **kwargs)


#-----------------------------------------------------------------------------
def plotSpectrum(source, indices, error_bars = False, type = -1):
    """Open a 1D Plot of a spectrum in a workspace
    
    @param source :: workspace or name of a workspace
    @param indices :: workspace index or list of workspace indices to plot
    @param error_bars :: bool, set to True to add error bars.
    """
    workspace_names = __getWorkspaceNames(source)
    index_list = __getWorkspaceIndices(indices)
    if len(workspace_names) > 0 and len(index_list) > 0:
        return __tryPlot(workspace_names, index_list, error_bars, type)
    else:
        return None
    

#-----------------------------------------------------------------------------
def plotBin(source, indices, error_bars = False, type = 0):
    """Create a 1D Plot of a bin or bins in a workspace
    
    @param source :: workspace or name of a workspace
    @param indices :: workspace index or list of workspace indices to plot
    @param error_bars :: bool, set to True to add error bars.
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
    # If the source is a workspace, create a table from the specified index
    if isinstance(source,WorkspaceProxy):
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
def plotSlice(source, label="", xydim=None, slicepoint=None,
                    colormin=None, colormax=None, colorscalelog=False,
                    limits=None, **kwargs):
    """Opens the SliceViewer with the given MDWorkspace(s).
    
    @param source :: one workspace, or a list of workspaces
        
    The following are optional keyword arguments:
    @param label :: label for the window title
    @param xydim :: indexes or names of the dimensions to plot,
            as an (X,Y) list or tuple.
            See SliceViewer::setXYDim()
    @param slicepoint :: list with the slice point in each dimension. Must be the 
            same length as the number of dimensions of the workspace.
            See SliceViewer::setSlicePoint()
    @param colormin :: value of the minimum color in the scale
            See SliceViewer::setColorScaleMin()
    @param colormax :: value of the maximum color in the scale
            See SliceViewer::setColorScaleMax()
    @param colorscalelog :: value of the maximum color in the scale
            See SliceViewer::setColorScaleLog()
    @param limits :: list with the (xleft, xright, ybottom, ytop) limits
            to the view to show.
            See SliceViewer::setXYLimits()
        
    @return a (list of) handle(s) to the SliceViewerWindow widgets that were open.
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
        pxy = QtProxyObject(window)
        out.append(pxy)
        
    # Returh the widget alone if only 1
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
    
    @param source :: name of the workspace that was open
    @param label :: additional label string that was used to identify the window.
    @return a handle to the SliceViewerWindow object that was created before. 
    """
    import mantidqtpython
    workspace_names = __getWorkspaceNames(source)
    if len(workspace_names) != 1:
        raise Exception("Please specify only one workspace.")
    else:
        svw = mantidqtpython.MantidQt.Factory.WidgetFactory.Instance().getSliceViewerWindow(workspace_names[0], label)
        return QtProxyObject(svw)



#-----------------------------------------------------------------------------
# Legacy function
plotTimeBin = plotBin

# Ensure these functions are available as without the qti.app.mantidUI prefix
MantidUIImports = [
    'importMatrixWorkspace',
    'importTableWorkspace',
    'getMantidMatrix',
    'getInstrumentView', 
    'getSelectedWorkspaceName',
    'mergePlots',
    'convertToWaterfall'
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
#--------------------------- "Private" functions -----------------------
#-----------------------------------------------------------------------------



#-------------------------------------------------------------------------------
class QtProxyObject(QtCore.QObject):
    """Generic Proxy object for wrapping Qt C++ Qobjects.
    This holds the QObject internally and passes methods to it.
    When the underlying object is deleted, the reference is set
    to None to avoid segfaults.
    """
    def __init__(self, toproxy):
        QtCore.QObject.__init__(self)
        self.__obj = toproxy
        # Connect to track the destroyed
        QtCore.QObject.connect( self.__obj, QtCore.SIGNAL("destroyed()"),
                                self._heldObjectDestroyed)
        
    def _heldObjectDestroyed(self):
        """Slot called when the held object is destroyed.
        Sets it to None, preventing segfaults """
        self._kill_object()

    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        return getattr(self._getHeldObject(), attr)

    def __str__(self):
        """
        Return a string representation of the proxied object
        """
        return str(self._getHeldObject())

    def __repr__(self):
        """
        Return a string representation of the proxied object
        """
        return `self._getHeldObject()`

    def _getHeldObject(self):
        """
        Returns a reference to the held object
        """
        return self.__obj

    def _kill_object(self):
        """
        Release the stored instance
        """
        self.__obj = None

    def _swap(self, obj):
        """
        Swap an object so that the proxy now refers to this object
        """
        self.__obj = obj



#-------------------------------------------------------------------------------
def __getWorkspaceNames(source):
    """Takes a "source", which could be a WorkspaceGroup, or a list
    of workspaces, or a list of names, and converts
    it to a list of workspace names.
    
        @param source :: input list or workspace group
        @return list of workspace names 
    """
    ws_names = []
    if isinstance(source, list) or isinstance(source,tuple):
        for w in source:
            names = __getWorkspaceNames(w)
            ws_names += names
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
            # Other non-matrix workspaces
            ws_names.append(wspace.getName())
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
    
#-----------------------------------------------------------------------------
def __doSliceViewer(wsname, label="", xydim=None, slicepoint=None,
                    colormin=None, colormax=None, colorscalelog=False,
                    limits=None):
    """Open a single SliceViewerWindow for the workspace, and shows it
    
    @param wsname :: name of the workspace
    See plotSlice() for full list of keyword parameters.

    @return SliceViewerWindow widget
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
    graph = qti.app.mantidUI.pyPlotSpectraList(workspace_names, indices, error_bars, type)
    if graph == None:
        raise RuntimeError("Cannot create graph, see log for details.")
    else:
        return graph
        

# Refactored functions for common code
def __doPlotting(source, indices, error_bars,type):
    if isinstance(source, list) or isinstance(source, tuple):
        return __PlotList(source, indices, error_bars,type)
    elif isinstance(source, str) or isinstance(source, WorkspaceProxy):
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
    return qti.app.mantidUI.plotBin(wkspname, index, error_bars,type)
