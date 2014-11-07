"""
Module containing classes that act as proxies to the various MantidPlot gui objects that are
accessible from python. They listen for the QObject 'destroyed' signal and set the wrapped
reference to None, thus ensuring that further attempts at access do not cause a crash.
"""

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import Qt, pyqtSlot
import __builtin__
import mantid

#-----------------------------------------------------------------------------
#--------------------------- MultiThreaded Access ----------------------------
#-----------------------------------------------------------------------------

class CrossThreadCall(QtCore.QObject):
    """
    Defines a dispatch call that marshals
    function calls between threads
    """
    __callable = None
    __args = []
    __kwargs = {}
    __func_return = None
    __exception = None

    def __init__(self, callable):
        """ Construct the object
        """
        QtCore.QObject.__init__(self)
        self.moveToThread(QtGui.qApp.thread())
        self.__callable = callable
        self.__call__.__func__.__doc__ = callable.__doc__

    def dispatch(self, *args, **kwargs):
        """Dispatches a call to callable with
        the given arguments using QMetaObject.invokeMethod
        to ensure the call happens in the object's thread
        """
        self.__args = args
        self.__kwargs = kwargs
        self.__func_return = None
        self.__exception = None
        return self._do_dispatch()

    def __call__(self, *args, **kwargs):
        """
        Calls the dispatch method
        """
        return self.dispatch(*args, **kwargs)

    @pyqtSlot()
    def _do_dispatch(self):
        """Perform a call to a GUI function across a
        thread and return the result
        """
        if QtCore.QThread.currentThread() != QtGui.qApp.thread():
            QtCore.QMetaObject.invokeMethod(self, "_do_dispatch", Qt.BlockingQueuedConnection)
        else:
            try:
                self.__func_return = self.__callable(*self.__args, **self.__kwargs)
            except Exception, exc:
                self.__exception = exc

        if self.__exception is not None:
            raise self.__exception # Ensures this happens in the right thread
        return self.__func_return

    def _get_argtype(self, argument):
        """
            Returns the argument type that will be passed to
            the QMetaObject.invokeMethod call.

            Most types pass okay, but enums don't so they have
            to be coerced to ints. An enum is currently detected
            as a type that is not a bool and inherits from __builtin__.int
        """
        argtype = type(argument)
        return argtype
        if isinstance(argument, __builtin__.int) and argtype != bool:
            argtype = int
        return argtype

def threadsafe_call(callable, *args, **kwargs):
    """
        Calls the given function with the given arguments
        by passing it through the CrossThreadCall class. This
        ensures that the calls to the GUI functions
        happen on the correct thread.
    """
    caller = CrossThreadCall(callable)
    return caller.dispatch(*args, **kwargs)

def new_proxy(classType, callable, *args, **kwargs):
    """
    Calls the callable object with the given args and kwargs dealing
    with possible thread-safety issues.
    If the returned value is not None it is wrapped in a new proxy of type classType

        @param classType :: A new proxy class for the return type
        @param callable :: A python callable object, i.e. a function/method
        @param \*args :: The positional arguments passed on as given
        @param \*kwargs :: The keyword arguments passed on as given
    """
    obj = threadsafe_call(callable, *args, **kwargs)
    if obj is None:
        return obj
    return classType(obj)

#-----------------------------------------------------------------------------
#--------------------------- Proxy Objects -----------------------------------
#-----------------------------------------------------------------------------

class QtProxyObject(QtCore.QObject):
    """Generic Proxy object for wrapping Qt C++ QObjects.
    This holds the QObject internally and passes methods to it.
    When the underlying object is deleted, the reference is set
    to None to avoid segfaults.
    """
    def __init__(self, toproxy):
        QtCore.QObject.__init__(self)
        self.__obj = toproxy
        # Connect to track the destroyed
        if self.__obj is not None:
            self.connect(self.__obj, QtCore.SIGNAL("destroyed()"),
                         self._kill_object, Qt.DirectConnection)

    def __del__(self):
        """
        Disconnect the signal
        """
        self._disconnect_from_destroyed()

    def close(self):
        """
        Reroute a method call to the the stored object
        """
        self._disconnect_from_destroyed()
        if hasattr(self.__obj, 'closeDependants'):
            threadsafe_call(self.__obj.closeDependants)
        if hasattr(self.__obj, 'close'):
            threadsafe_call(self.__obj.close)
        self._kill_object()

    def inherits(self, className):
        """
        Reroute a method call to the stored object
        """
        return threadsafe_call(self.__obj.inherits, className)

    def _disconnect_from_destroyed(self):
        """
        Disconnects from the wrapped object's destroyed signal
        """
        if self.__obj is not None:
            self.disconnect(self.__obj, QtCore.SIGNAL("destroyed()"),
                            self._kill_object)

    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object via
        the threadsafe call mechanism. Essentially this guarantees
        that when the method is called it wil be on the GUI thread
        """
        callable = getattr(self._getHeldObject(), attr)
        return CrossThreadCall(callable)

    def __dir__(self):
        return dir(self._getHeldObject())

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

#-----------------------------------------------------------------------------
class MDIWindow(QtProxyObject):
    """Proxy for the _qti.MDIWindow object.
    Also used for subclasses that do not need any methods intercepted (e.g. Table, Note, Matrix)
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def folder(self):
        return new_proxy(Folder, self._getHeldObject().folder)

#-----------------------------------------------------------------------------
class Graph(MDIWindow):
    """Proxy for the _qti.Graph object.
    """
    # When checking the SIP interface, remember the following name mappings (PyName):
    # C++ 'Multilayer' class => Python 'Graph' class
    # C++ 'Graph' class => Python 'Layer' class

    def __init__(self, toproxy):
        MDIWindow.__init__(self,toproxy)

    def activeLayer(self):
        """Get a handle to the presently active layer """
        return new_proxy(Layer, self._getHeldObject().activeLayer)

    def setActiveLayer(self, layer):
        """Set the active layer to that specified.

        Args:
            layer: A reference to the Layer to make the active one. Must belong to this Graph.
        """
        threadsafe_call(self._getHeldObject().setActiveLayer, layer._getHeldObject())

    def layer(self, num):
        """ Get a handle to a specified layer

        Args:
            num: The index of the required layer
        """
        return new_proxy(Layer, self._getHeldObject().layer, num)

    def addLayer(self, x=0, y=0, width=None, height=None):
        """Add a layer to the graph.

        Args:
            x: The coordinate in pixels (from the graph's left) of the top-left of the new layer (default: 0).
            y: The coordinate in pixels (from the graph's top) of the top-left of the new layer (default: 0).
            width: The width of the new layer (default value if not specified).
            height: The height of the new layer (default value if not specified).

        Returns:
            A handle to the newly created layer.
        """
        # Turn the optional arguments into the magic numbers that the C++ expects
        if width is None:
            width=0
        if height is None:
            height=0
        return new_proxy(Layer, self._getHeldObject().addLayer, x,y,width,height)

    def insertCurve(self, graph, index):
        """Add a curve from another graph to this one.

        Args:
            graph: A reference to the graph from which the curve is coming (does nothing if this argument is the present Graph).
            index: The index of the curve to add (counts from zero).
        """
        threadsafe_call(self._getHeldObject().insertCurve, graph._getHeldObject(), index)


#-----------------------------------------------------------------------------
class Layer(QtProxyObject):
    """Proxy for the _qti.Layer object.
    """
    # These methods are used for the new matplotlib-like CLI
    # These ones are provided by the C++ class Graph, which in the SIP declarations is renamed as Layer
    # The only purpose of listing them here is that these will be returned by this class' __dir()__, and
    #    shown interactively, while the ones not listed and/or overloaded here may not be shown in ipython, etc.
    additional_methods = ['logLogAxes', 'logXLinY', 'logXLinY',
                          'removeLegend', 'saveImage', 'setAxisScale', 'setCurveLineColor', 'setCurveLineStyle',
                          'setCurveLineWidth', 'setCurveSymbol', 'setScale', 'setTitle', 'setXTitle', 'setYTitle']

    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def insertCurve(self, *args):
        """Add a curve from a workspace, table or another Layer to the plot

        Args:
            The first argument should be a reference to a workspace, table or layer, or a workspace name.
            Subsequent arguments vary according to the type of the first.

        Returns:
            A boolean indicating success or failure.
        """
        if isinstance(args[0],str):
            return threadsafe_call(self._getHeldObject().insertCurve, *args)
        elif hasattr(args[0], 'getName'):
            return threadsafe_call(self._getHeldObject().insertCurve, args[0].getName(),*args[1:])
        else:
            return threadsafe_call(self._getHeldObject().insertCurve, args[0]._getHeldObject(),*args[1:])

    def addCurves(self, table, columns, style=0, lineWidth=1, symbolSize=3, startRow=0, endRow=-1):
        """Add curves based on table columns to the plot.

        Args:
            table: A reference to the table containing the data to plot.
            columns: A tuple of column indices.
            style: The curve style (default: Line).
            lineWidth: The width of the curve line (default: 1).
            symbolSize: The curve's symbol size (default: 3).
            startRow: The first row to include in the curve's data (default: the first one)
            endRow: The last row to include in the curve's data (default: the last one)

        Returns:
            A boolean indicating success or failure.
        """
        return threadsafe_call(self._getHeldObject().addCurves, table._getHeldObject(),columns,style,lineWidth,symbolSize,startRow,endRow)

    def addCurve(self, table, columnName, style=0, lineWidth=1, symbolSize=3, startRow=0, endRow=-1):
        """Add a curve based on a table column to the plot.

        Args:
            table: A reference to the table containing the data to plot.
            columns: The name of the column to plot.
            style: The curve style (default: Line).
            lineWidth: The width of the curve line (default: 1).
            symbolSize: The curve's symbol size (default: 3).
            startRow: The first row to include in the curve's data (default: the first one)
            endRow: The last row to include in the curve's data (default: the last one)

        Returns:
            A boolean indicating success or failure.
        """
        return threadsafe_call(self._getHeldObject().addCurve, table._getHeldObject(),columnName,style,lineWidth,symbolSize,startRow,endRow)

    def addErrorBars(self, yColName, errTable, errColName, type=1, width=1, cap=8, color=Qt.black, through=False, minus=True, plus=True):
        """Add error bars to a plot that was created from a table column.

        Args:
            yColName: The name of the column pertaining to the curve's data values.
            errTable: A reference to the table holding the error values.
            errColName: The name of the column containing the error values.
            type: The orientation of the error bars - horizontal (0) or vertical (1, the default).
            width: The line width of the error bars (default: 1).
            cap: The length of the cap on the error bars (default: 8).
            color: The color of error bars (default: black).
            through: Whether the error bars are drawn through the symbol (default: no).
            minus: Whether these errors should be shown as negative errors (default: yes).
            plus: Whether these errors should be shown as positive errors (default: yes).
        """
        threadsafe_call(self._getHeldObject().addErrorBars, yColName,errTable._getHeldObject(),errColName,type,width,cap,color,through,minus,plus)

    def errorBarSettings(self, curveIndex, errorBarIndex=0):
        """Get a handle to the error bar settings for a specified curve.

        Args:
            curveIndex: The curve to get the settings for
            errorBarIndex: A curve can hold more than one set of error bars. Specify which one (default: the first).
                           Note that a curve plotted from a workspace can have only one set of error bars (and hence settings).

        Returns: A handle to the error bar settings object.
        """
        return new_proxy(QtProxyObject, self._getHeldObject().errorBarSettings, curveIndex,errorBarIndex)

    def addHistogram(self, matrix):
        """Add a matrix histogram  to the graph"""
        threadsafe_call(self._getHeldObject().addHistogram, matrix._getHeldObject())

    def newLegend(self, text):
        """Create a new legend.

        Args:
            text: The text of the legend.

        Returns:
            A handle to the newly created legend widget.
        """
        return new_proxy(QtProxyObject, self._getHeldObject().newLegend, text)

    def legend(self):
        """Get a handle to the layer's legend widget."""
        return new_proxy(QtProxyObject, self._getHeldObject().legend)

    def grid(self):
        """Get a handle to the grid object for this layer."""
        return new_proxy(QtProxyObject, self._getHeldObject().grid)

    def spectrogram(self):
        """If the layer contains a spectrogram, get a handle to the spectrogram object."""
        return new_proxy(QtProxyObject, self._getHeldObject().spectrogram)


    def __dir__(self):
        """Returns the list of attributes of this object."""
        # The first part (explicitly defined ones) are here for the traditional Mantid CLI,
        # the additional ones have been added for the matplotlib-like CLI (without explicit
        # declaration/documentation here in the proxies layer.
        return ['insertCurve', 'addCurves', 'addCurve', 'addErrorBars', 'errorBarSettings', 'addHistogram',
                'newLegend', 'legend', 'grid', 'spectrogram' ] + self.additional_methods


#-----------------------------------------------------------------------------
class Graph3D(QtProxyObject):
    """Proxy for the _qti.Graph3D object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def setData(self, table, colName, type=0):
        """Set a table column to be the data source for this plot.

        Args:
            table: A reference to the table.
            colName: The name of the column to set as the data source.
            type: The plot type.
        """
        threadsafe_call(self._getHeldObject().setData, table._getHeldObject(),colName,type)

    def setMatrix(self, matrix):
        """Set a matrix (N.B. not a MantidMatrix) to be the data source for this plot.

        Args:
            matrix: A reference to the matrix.
        """
        threadsafe_call(self._getHeldObject().setMatrix, matrix._getHeldObject())

#-----------------------------------------------------------------------------
class Spectrogram(QtProxyObject):
    """Proxy for the _qti.Spectrogram object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def matrix(self):
        """Get a handle to the data source."""
        return new_proxy(QtProxyObject, self._getHeldObject().matrix)

#-----------------------------------------------------------------------------
class Folder(QtProxyObject):
    """Proxy for the _qti.Folder object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def windows(self):
        """Get a list of the windows in this folder"""
        f = self._getHeldObject().windows()
        ret = []
        for item in f:
            ret.append(MDIWindow(item))
        return ret

    def folders(self):
        """Get a list of the subfolders of this folder"""
        f = self._getHeldObject().folders()
        ret = []
        for item in f:
            ret.append(Folder(item))
        return ret

    def folder(self, name, caseSensitive=True, partialMatch=False):
        """Get a handle to a named subfolder.

        Args:
            name: The name of the subfolder.
            caseSensitive: Whether to search case-sensitively or not (default: yes).
            partialMatch: Whether to return a partial match (default: no).

        Returns:
            A handle to the requested folder, or None if no match found.
        """
        return new_proxy(Folder, self._getHeldObject().folder, name,caseSensitive,partialMatch)

    def findWindow(self, name, searchOnName=True, searchOnLabel=True, caseSensitive=False, partialMatch=True):
        """Get a handle to the first window matching the search criteria.

        Args:
            name: The name of the window.
            searchOnName: Whether to search the window names (takes precedence over searchOnLabel).
            searchOnLabel: Whether to search the window labels.
            caseSensitive: Whether to search case-sensitively or not (default: no).
            partialMatch: Whether to return a partial match (default: yes).

        Returns:
            A handle to the requested window, or None if no match found.
        """
        return new_proxy(MDIWindow, self._getHeldObject().findWindow, name,searchOnName,searchOnLabel,caseSensitive,partialMatch)

    def window(self, name, cls='MdiSubWindow', recursive=False):
        """Get a handle to a named window of a particular type.

        Args:
            name: The name of the window.
            cls: Search only for windows of type inheriting from this class (N.B. This is the C++ class name).
            recursive: If True, do a depth-first recursive search (default: False).

        Returns:
            A handle to the window, or None if no match found.
        """
        return new_proxy(MDIWindow, self._getHeldObject().window, name,cls,recursive)

    def table(self, name, recursive=False):
        """Get a handle to the table with the given name.

        Args:
            name: The name of the table to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return new_proxy(MDIWindow, self._getHeldObject().table, name,recursive)

    def matrix(self, name, recursive=False):
        """Get a handle to the matrix with the given name.

        Args:
            name: The name of the matrix to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return  new_proxy(MDIWindow, self._getHeldObject().matrix, name,recursive)

    def graph(self, name, recursive=False):
        """Get a handle to the graph with the given name.

        Args:
            name: The name of the graph to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return new_proxy(Graph, self._getHeldObject().graph, name,recursive)

    def rootFolder(self):
        """Get the folder at the root of the hierarchy"""
        return new_proxy(Folder, self._getHeldObject().rootFolder)

#-----------------------------------------------------------------------------
class MantidMatrix(MDIWindow):
    """Proxy for the _qti.MantidMatrix object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def plotGraph3D(self, style=3):
        """Create a 3D plot of the workspace data.

        Args:
            style: The qwt3d plotstyle of the generated graph (default: filled mesh)

        Returns:
            A handle to the newly created graph (a Graph3D object)
        """
        return new_proxy(Graph3D, self._getHeldObject().plotGraph3D, style)

    def plotGraph2D(self, type=16):
        """Create a spectrogram from the workspace data.

        Args:
            type: The style of the plot (default: ColorMap)

        Returns:
            A handle the newly created graph (a Graph object)
        """
        return new_proxy(Graph, self._getHeldObject().plotGraph2D, type)

#-----------------------------------------------------------------------------
class InstrumentWindow(MDIWindow):
    """Proxy for the instrument window
    """

    def __init__(self, toproxy):
        """Creates a proxy object around an instrument window

        Args:
            toproxy: The raw C object to proxy
        """
        QtProxyObject.__init__(self, toproxy)

    def getTab(self, name_or_tab):
        """Retrieve a handle to the given tab

        Args:
            name_or_index: A string containing the title or tab type

        Returns:
            A handle to a tab widget
        """
        handle = new_proxy(QtProxyObject, self._getHeldObject().getTab, name_or_tab)
        if handle is None:
            raise ValueError("Invalid tab title '%s'" % str(name_or_tab))
        return handle

    # ----- Deprecated functions -----
    def changeColormap(self, filename=None):
        import warnings
        warnings.warn("InstrumentWindow.changeColormap has been deprecated. Use the render tab method instead.")
        callable = QtProxyObject.__getattr__(self, "changeColormap")
        if filename is None:
            callable()
        else:
            callable(filename)

    def setColorMapMinValue(self, value):
        import warnings
        warnings.warn("InstrumentWindow.setColorMapMinValue has been deprecated. Use the render tab setMinValue method instead.")
        QtProxyObject.__getattr__(self, "setColorMapMinValue")(value)

    def setColorMapMaxValue(self, value):
        import warnings
        warnings.warn("InstrumentWindow.setColorMapMaxValue has been deprecated. Use the render tab setMaxValue method instead.")
        QtProxyObject.__getattr__(self, "setColorMapMaxValue")(value)

    def setColorMapRange(self, minvalue, maxvalue):
        import warnings
        warnings.warn("InstrumentWindow.setColorMapRange has been deprecated. Use the render tab setRange method instead.")
        QtProxyObject.__getattr__(self, "setColorMapRange")(minvalue,maxvalue)

    def setScaleType(self, scale_type):
        import warnings
        warnings.warn("InstrumentWindow.setScaleType has been deprecated. Use the render tab setScaleType method instead.")
        QtProxyObject.__getattr__(self, "setScaleType")(scale_type)

    def setViewType(self, view_type):
        import warnings
        warnings.warn("InstrumentWindow.setViewType has been deprecated. Use the render tab setSurfaceType method instead.")
        QtProxyObject.__getattr__(self, "setViewType")(view_type)

    def selectComponent(self, name):
        import warnings
        warnings.warn("InstrumentWindow.selectComponent has been deprecated. Use the tree tab selectComponentByName method instead.")
        QtProxyObject.__getattr__(self, "selectComponent")(name)

#-----------------------------------------------------------------------------
class SliceViewerWindowProxy(QtProxyObject):
    """Proxy for a C++ SliceViewerWindow object.

    It will pass-through method calls that can be applied to the
    SliceViewer widget contained within.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self, toproxy)

    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        if self._getHeldObject() is None:
            raise Exception("Error! The SliceViewerWindow has been deleted.")

        # Pass-through to the contained SliceViewer widget.
        sv = self.getSlicer()
        # But only those attributes that are methods on the SliceViewer
        if attr in SliceViewerProxy.slicer_methods:
            return getattr(sv, attr)
        else:
            # Otherwise, pass through to the stored object
            return getattr(self._getHeldObject(), attr)

    def __str__(self):
        """
        Return a string representation of the proxied object
        """
        if self._getHeldObject() is None:
            return "None"
        else:
            return 'SliceViewerWindow(workspace="%s")' % self._getHeldObject().getSlicer().getWorkspaceName()

    def __repr__(self):
        """
        Return a string representation of the proxied object
        """
        return `self._getHeldObject()`

    def __dir__(self):
        """
        Returns the list of attributes for this object.
        Might allow tab-completion to work under ipython
        """
        return SliceViewerProxy.slicer_methods + ['showLine']

    def getLiner(self):
        """
        Returns the LineViewer widget that is part of this
        SliceViewerWindow
        """
        return LineViewerProxy(self._getHeldObject().getLiner())

    def getSlicer(self):
        """
        Returns the SliceViewer widget that is part of this
        SliceViewerWindow
        """
        return SliceViewerProxy(self._getHeldObject().getSlicer())

    def showLine(self, start, end, width=None, planar_width=0.1, thicknesses=None,
                 num_bins=100):
        """Opens the LineViewer and define a 1D line along which to integrate.

        The line is created in the same XY dimensions and at the same slice
        point as is currently shown in the SliceViewer.

        Args:
            start :: (X,Y) coordinates of the start point in the XY dimensions
                of the current slice.
            end :: (X,Y) coordinates of the end point in the XY dimensions
                of the current slice.
            width :: if specified, sets all the widths (planar and other
                dimensions) to this integration width.
            planar_width :: sets the XY-planar (perpendicular to the line)
                integration width. Default 0.1.
            thicknesses :: list with one thickness value for each dimension in the
                workspace (including the XY dimensions, which are ignored).
                e.g. [0,1,2,3] in a XYZT workspace.
            num_bins :: number of bins by which to divide the line.
                Default 100.

        Returns:
            The LineViewer object of the SliceViewerWindow. There are methods
            available to modify the line drawn.
        """
        # First show the lineviewer
        self.getSlicer().toggleLineMode(True)
        liner = self.getLiner()

        # Start and end point
        liner.setStartXY(start[0], start[1])
        liner.setEndXY(end[0], end[1])

        # Set the width.
        if not width is None:
            liner.setThickness(width)
        else:
            liner.setPlanarWidth(planar_width)
            if not thicknesses is None:
                for d in xrange(len(thicknesses)):
                    liner.setThickness(d, thicknesses[i])
        # Bins
        liner.setNumBins(num_bins)
        liner.apply()

        # Return the proxy to the LineViewer widget
        return liner

#-----------------------------------------------------------------------------
def getWorkspaceNames(source):
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
            names = getWorkspaceNames(w)
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
        w = None
        try:
            # for non-existent names this raises a KeyError
            w = mantid.AnalysisDataService.Instance()[source]
        except Exception, exc:
            raise ValueError("Workspace '%s' not found!"%source)

        if w != None:
            names = getWorkspaceNames(w)
            for n in names:
                ws_names.append(n)
    else:
        raise TypeError('Incorrect type passed as workspace argument "' + str(source) + '"')
    return ws_names

#-----------------------------------------------------------------------------
class ProxyCompositePeaksPresenter(QtProxyObject):
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def getPeaksPresenter(self, source):
        to_present = None
        if isinstance(source, str):
            to_present = source
        elif isinstance(source, mantid.api.Workspace):
            to_present = source.getName()
        else:
            raise ValueError("getPeaksPresenter expects a Workspace name or a Workspace object.")
        if not mantid.api.mtd.doesExist(to_present):
                raise ValueError("%s does not exist in the workspace list" % to_present)

        return new_proxy(QtProxyObject, self._getHeldObject().getPeaksPresenter, to_present)

#-----------------------------------------------------------------------------
class SliceViewerProxy(QtProxyObject):
    """Proxy for a C++ SliceViewer widget.
    """
    # These are the exposed python method names
    slicer_methods = ["setWorkspace", "getWorkspaceName", "showControls", "openFromXML", "getImage", "saveImage", "copyImageToClipboard", "setFastRender", "getFastRender", "toggleLineMode", "setXYDim", "setXYDim", "getDimX", "getDimY", "setSlicePoint", "setSlicePoint", "getSlicePoint", "getSlicePoint", "setXYLimits", "getXLimits", "getYLimits", "zoomBy", "setXYCenter", "resetZoom", "loadColorMap", "setColorScale", "setColorScaleMin", "setColorScaleMax", "setColorScaleLog", "getColorScaleMin", "getColorScaleMax", "getColorScaleLog", "setColorScaleAutoFull", "setColorScaleAutoSlice", "setColorMapBackground", "setTransparentZeros", "setNormalization", "getNormalization", "setRebinThickness", "setRebinNumBins", "setRebinMode", "setPeaksWorkspaces", "refreshRebin"]

    def __init__(self, toproxy):
        QtProxyObject.__init__(self, toproxy)

    def __dir__(self):
        """Returns the list of attributes for this object.   """
        return self.slicer_methods()

    def setPeaksWorkspaces(self, source):
        workspace_names = getWorkspaceNames(source)
        if len(workspace_names) == 0:
            raise ValueError("No workspace names given to setPeaksWorkspaces")
        for name in workspace_names:
            if not mantid.api.mtd.doesExist(name):
                raise ValueError("%s does not exist in the workspace list" % name)
        if not isinstance(mantid.api.mtd[name], mantid.api.IPeaksWorkspace):
            raise ValueError("%s is not an IPeaksWorkspace" % name)

        return new_proxy(ProxyCompositePeaksPresenter, self._getHeldObject().setPeaksWorkspaces, workspace_names)


#-----------------------------------------------------------------------------
class LineViewerProxy(QtProxyObject):
    """Proxy for a C++ LineViewer widget.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self, toproxy)

    def __dir__(self):
        """Returns the list of attributes for this object.   """
        return ["apply", "showPreview", "showFull", "setStartXY", "setEndXY", "setThickness", "setThickness",
                "setThickness", "setPlanarWidth", "getPlanarWidth", "setNumBins", "setFixedBinWidthMode", "getFixedBinWidth",
                "getFixedBinWidthMode", "getNumBins", "getBinWidth", "setPlotAxis", "getPlotAxis"]


#-----------------------------------------------------------------------------
class FitBrowserProxy(QtProxyObject):
    """
        Proxy for the FitPropertyBrowser object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)


#-----------------------------------------------------------------------------
class TiledWindowProxy(QtProxyObject):
    """
        Proxy for the TiledWindow object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def addWidget(self, tile, row, col):
        """
        Add a new sub-window at a given position in the layout.
        The layout will re-shape itself if necessary to fit in the new tile.

        Args:

            tile :: An MdiSubWindow to add.
            row :: A row index at which to place the new tile.
            col :: A column index at which to place the new tile.
        """
        threadsafe_call(self._getHeldObject().addWidget, tile._getHeldObject(), row, col)

    def insertWidget(self, tile, row, col):
        """
        Insert a new sub-window at a given position in the layout.
        The widgets to the right and below the inserted tile will be shifted
        towards the bottom of the window. If necessary a new row will be appended.
        The number of columns doesn't change.

        Args:

            tile :: An MdiSubWindow to insert.
            row :: A row index at which to place the new tile.
            col :: A column index at which to place the new tile.
        """
        threadsafe_call(self._getHeldObject().insertWidget, tile._getHeldObject(), row, col)

    def getWidget(self, row, col):
        """
        Get a sub-window at a location in this TiledWindow.

        Args:
            row :: A row of a sub-window.
            col :: A column of a sub-window.
        """
        return MDIWindow( threadsafe_call(self._getHeldObject().getWidget, row, col) )

    def clear(self):
        """
        Clear the content this TiledWindow.
        """
        threadsafe_call(self._getHeldObject().clear)
