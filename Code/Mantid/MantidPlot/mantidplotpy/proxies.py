"""
Module containing classes that act as proxies to the various MantidPlot gui objects that are
accessible from python. They listen for the QObject 'destroyed' signal and set the wrapped
reference to None, thus ensuring that further attempts at access do not cause a crash.
"""

from PyQt4 import QtCore

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

#-----------------------------------------------------------------------------
class Graph(QtProxyObject):
    """Proxy for the qti.Graph object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)

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

#-----------------------------------------------------------------------------
class Layer(QtProxyObject):
    """Proxy for the qti.Layer object.
    """
    def __init__(self, toproxy):
        QtProxyObject.__init__(self,toproxy)
    
    def insertCurve(self, *args):
        """Add a curve from a workspace, table or another Layer to the plot
        
        Args:
            The first argument should be a reference to a table or layer, or a workspace name.
            Subsequent arguments vary according to the type of the first.
        
        Returns:
            A boolean indicating success or failure.
        """
        if isinstance(args[0],str):
            return self._getHeldObject().insertCurve(*args)
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
        return self._getHeldObject().addCurves(table._getHeldObject(),columnName,style,lineWidth,symbolSize,startRow,endRow)

#-----------------------------------------------------------------------------
class Graph3D(QtProxyObject):
    """Proxy for the qti.Graph3D object.
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
class MantidMatrix(QtProxyObject):
    """Proxy for the qti.MantidMatrix object.
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
        # List of methods in slicer to pass-through
        self.slicer_methods = ["setWorkspace", "getWorkspaceName", "showControls", "openFromXML", "saveImage", "setXYDim", "setXYDim", "getDimX", "getDimY", "setSlicePoint", "setSlicePoint", "getSlicePoint", "getSlicePoint", "setXYLimits", "getXLimits", "getYLimits", "zoomBy", "setXYCenter", "resetZoom", "loadColorMap", "setColorScale", "setColorScaleMin", "setColorScaleMax", "setColorScaleLog", "getColorScaleMin", "getColorScaleMax", "getColorScaleLog", "setColorScaleAutoFull", "setColorScaleAutoSlice", "setFastRender", "getFastRender"]

    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        if self._getHeldObject() is None:
            raise Exception("Error! The SliceViewerWindow has been deleted.")
        
        # Pass-through to the contained SliceViewer widget.
        sv = self._getHeldObject().getSlicer()
        if attr in self.slicer_methods:
            return getattr(sv, attr)
        else:
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
        return self.slicer_methods


