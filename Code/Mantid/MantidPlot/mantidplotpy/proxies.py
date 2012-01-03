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


