"""
Module containing classes that act as proxies to the various MantidPlot gui objects that are
accessible from python. They listen for the QObject 'destroyed' signal and set the wrapped
reference to None, thus ensuring that further attempts at access do not cause a crash.
"""

from PyQt4 import QtCore
from PyQt4.QtCore import Qt

#-----------------------------------------------------------------------------
#--------------------------- Proxy Objects -----------------------------------
#-----------------------------------------------------------------------------



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
            QtCore.QObject.connect( self.__obj, QtCore.SIGNAL("destroyed()"),
                                    self._heldObjectDestroyed)

    def __del__(self):
        # Disconnect the signal or you get a segfault on quitting MantidPlot
        if self.__obj is not None:
            QtCore.QObject.disconnect( self.__obj, QtCore.SIGNAL("destroyed()"), self._heldObjectDestroyed )

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

#-----------------------------------------------------------------------------
class MDIWindow(QtProxyObject):
    """Proxy for the _qti.MDIWindow object. 
    Also used for subclasses that do not need any methods intercepted (e.g. Table, Note, Matrix)
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def folder(self):
        return Folder(self._getHeldObject().folder())

#-----------------------------------------------------------------------------
class Graph(MDIWindow):
    """Proxy for the _qti.Graph object.
    """
    def __init__(self, toproxy):
        MDIWindow.__init__(self,toproxy)

    def activeLayer(self):
        """Get a handle to the presently active layer """
        return Layer(self._getHeldObject().activeLayer())

    def setActiveLayer(self, layer):
        """Set the active layer to that specified.
        
        Args:
            layer: A reference to the Layer to make the active one. Must belong to this Graph.
        """
        self._getHeldObject().setActiveLayer(layer._getHeldObject())

    def layer(self, num):
        """ Get a handle to a specified layer
        
        Args:
            num: The index of the required layer
        """
        return Layer(self._getHeldObject().layer(num))

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
        return Layer(self._getHeldObject().addLayer(x,y,width,height))

    def insertCurve(self, graph, index):
        """Add a curve from another graph to this one.
        
        Args:
            graph: A reference to the graph from which the curve is coming (does nothing if this argument is the present Graph).
            index: The index of the curve to add (counts from zero).
        """
        self._getHeldObject().insertCurve(graph._getHeldObject(),index)

#-----------------------------------------------------------------------------
class Layer(QtProxyObject):
    """Proxy for the _qti.Layer object.
    """
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
            return self._getHeldObject().insertCurve(*args)
        elif hasattr(args[0], 'getName'):
            return self._getHeldObject().insertCurve(args[0].getName(),*args[1:])
        else:
            return self._getHeldObject().insertCurve(args[0]._getHeldObject(),*args[1:])
    
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
        return self._getHeldObject().addCurves(table._getHeldObject(),columns,style,lineWidth,symbolSize,startRow,endRow)

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
        return self._getHeldObject().addCurve(table._getHeldObject(),columnName,style,lineWidth,symbolSize,startRow,endRow)

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
        self._getHeldObject().addErrorBars(yColName,errTable._getHeldObject(),errColName,type,width,cap,color,through,minus,plus)

    def errorBarSettings(self, curveIndex, errorBarIndex=0):
        """Get a handle to the error bar settings for a specified curve.
        
        Args:
            curveIndex: The curve to get the settings for
            errorBarIndex: A curve can hold more than one set of error bars. Specify which one (default: the first).
                           Note that a curve plotted from a workspace can have only one set of error bars (and hence settings).
                           
        Returns: A handle to the error bar settings object.
        """
        return QtProxyObject(self._getHeldObject().errorBarSettings(curveIndex,errorBarIndex))

    def addHistogram(self, matrix):
        """Add a matrix histogram  to the graph"""
        self._getHeldObject().addHistogram(matrix._getHeldObject())

    def newLegend(self, text):
        """Create a new legend.
        
        Args:
            text: The text of the legend.
        
        Returns:
            A handle to the newly created legend widget.
        """
        return QtProxyObject(self._getHeldObject().newLegend(text))

    def legend(self):
        """Get a handle to the layer's legend widget."""
        return QtProxyObject(self._getHeldObject().legend())

    def grid(self):
        """Get a handle to the grid object for this layer."""
        return QtProxyObject(self._getHeldObject().grid())

    def spectrogram(self):
        """If the layer contains a spectrogram, get a handle to the spectrogram object."""
        return QtProxyObject(self._getHeldObject().spectrogram())

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
        self._getHeldObject().setData(table._getHeldObject(),colName,type)

    def setMatrix(self, matrix):
        """Set a matrix (N.B. not a MantidMatrix) to be the data source for this plot.
        
        Args:
            matrix: A reference to the matrix.
        """
        self._getHeldObject().setMatrix(matrix._getHeldObject())

#-----------------------------------------------------------------------------
class Spectrogram(QtProxyObject):
    """Proxy for the _qti.Spectrogram object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

    def matrix(self):
        """Get a handle to the data source."""
        return QtProxyObject(self._getHeldObject().matrix())

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
        return Folder(self._getHeldObject().folder(name,caseSensitive,partialMatch))

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
        return MDIWindow(self._getHeldObject().findWindow(name,searchOnName,searchOnLabel,caseSensitive,partialMatch))

    def window(self, name, cls='MdiSubWindow', recursive=False):
        """Get a handle to a named window of a particular type.
        
        Args:
            name: The name of the window.
            cls: Search only for windows of type inheriting from this class (N.B. This is the C++ class name).
            recursive: If True, do a depth-first recursive search (default: False).
            
        Returns:
            A handle to the window, or None if no match found.
        """
        return MDIWindow(self._getHeldObject().window(name,cls,recursive))

    def table(self, name, recursive=False):
        """Get a handle to the table with the given name.
        
        Args:
            name: The name of the table to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return MDIWindow(self._getHeldObject().table(name,recursive))

    def matrix(self, name, recursive=False):
        """Get a handle to the matrix with the given name.
        
        Args:
            name: The name of the matrix to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return MDIWindow(self._getHeldObject().matrix(name,recursive))

    def graph(self, name, recursive=False):
        """Get a handle to the graph with the given name.
        
        Args:
            name: The name of the graph to search for.
            recursive: If True, do a depth-first recursive search (default: False).
        """
        return Graph(self._getHeldObject().graph(name,recursive))

    def rootFolder(self):
        """Get the folder at the root of the hierarchy"""
        return Folder(self._getHeldObject().rootFolder())

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
        return Graph3D(self._getHeldObject().plotGraph3D(style))

    def plotGraph2D(self, type=16):
        """Create a spectrogram from the workspace data.
        
        Args:
            type: The style of the plot (default: ColorMap)
            
        Returns:
            A handle the newly created graph (a Graph object)
        """
        return Graph(self._getHeldObject().plotGraph2D(type))

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
        sv = self._getHeldObject().getSlicer()
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
    
    def showLine(self, start, end, width=None, planar_width=0.1, dim_widths=None,
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
            dim_widths :: list with one width value for each dimension in the
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
            liner.setWidth(width)
        else:
            liner.setPlanarWidth(planar_width)
            if not dim_widths is None:
                for d in xrange(len(dim_widths)):
                    liner.setWidth(d, dim_widths[i])
        # Bins
        liner.setNumBins(num_bins)
        liner.apply()

        # Return the proxy to the LineViewer widget        
        return liner
        



#-----------------------------------------------------------------------------
class SliceViewerProxy(QtProxyObject):
    """Proxy for a C++ SliceViewer widget.
    """
    # These are the exposed python method names
    slicer_methods = ["setWorkspace", "getWorkspaceName", "showControls", "openFromXML", "getImage", "saveImage", "copyImageToClipboard", "setFastRender", "getFastRender", "toggleLineMode", "setXYDim", "setXYDim", "getDimX", "getDimY", "setSlicePoint", "setSlicePoint", "getSlicePoint", "getSlicePoint", "setXYLimits", "getXLimits", "getYLimits", "zoomBy", "setXYCenter", "resetZoom", "loadColorMap", "setColorScale", "setColorScaleMin", "setColorScaleMax", "setColorScaleLog", "getColorScaleMin", "getColorScaleMax", "getColorScaleLog", "setColorScaleAutoFull", "setColorScaleAutoSlice"]
    
    def __init__(self, toproxy):
        QtProxyObject.__init__(self, toproxy)
        
    def __dir__(self):
        """Returns the list of attributes for this object.   """
        return self.slicer_methods()
    

#-----------------------------------------------------------------------------
class LineViewerProxy(QtProxyObject):
    """Proxy for a C++ LineViewer widget.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self, toproxy)
        
    def __dir__(self):
        """Returns the list of attributes for this object.   """
        return ["apply", "showPreview", "showFull", "setStartXY", "setEndXY", "setWidth", "setWidth", "setPlanarWidth", "getPlanarWidth", "setNumBins", "setFixedBinWidthMode", "getFixedBinWidth", "getFixedBinWidthMode", "getNumBins", "getBinWidth"]
    
    