import __main__

def import_to_global(modname, attrs=None, math=False):
    """
        import_to_global(modname, (a,b,c,...), math): like "from modname import a,b,c,...",
        but imports to global namespace (__main__).
        If math==True, also registers functions with QtiPlot's math function list.
    """
    mod = __import__(modname)
    for submod in modname.split(".")[1:]:
        mod = getattr(mod, submod)
    if attrs==None: attrs=dir(mod)
    for name in attrs:
        if name.startswith('__'):
            continue
        f = getattr(mod, name)
        setattr(__main__, name, f)
        # make functions available in QtiPlot's math function list
        if math and callable(f): qti.mathFunctions[name] = f

# Import math module
import math

# Import scipy.special functions (if available)
# See www.scipy.org for information on SciPy and how to get it.
have_scipy = False
try:
    import scipy.special
    have_scipy = True
    print "Imported scipy.special. For more information type help(scipy.special)."
except ImportError:
    pass          

# Import pygsl.sf (if available and scipy is not) 
# See pygsl.sourceforge.net for information on pygsl and how to get it.
try:
    if not have_scipy:
        import pygsl.sf
        print "Imported pygsl.sf. For more information type help(pygsl.sf)."
except ImportError: 
    pass

# Import PyQt4's QtCore and QtGui packages into our namespace
from PyQt4 import QtCore
from PyQt4 import QtGui

# import QtiPlot's classes to the global namespace (particularly useful for fits)
from qti import *

# import selected methods of ApplicationWindow into the global namespace
appImports = (
    "table", "newTable",
    "matrix", "newMatrix",
    "graph", "newGraph",
    "note", "newNote",
    "newPlot3D",
    "tableToMatrix", "matrixToTable",
    "openTemplate", "saveAsTemplate",
    "clone", "setWindowName",
    "importImage",
    "setPreferences",
    "plot", "plot3D",
    "activeFolder", "rootFolder",
    "addFolder", "deleteFolder", "changeFolder", "copyFolder",
    "saveFolder", "appendProject", "saveProjectAs"
    )
for name in appImports:
    setattr(__main__,name,getattr(qti.app,name))

# Set some aliases for Layer enumerations so that old code will still work
Layer.Log10 = GraphOptions.Log10
Layer.Linear = GraphOptions.Linear
Layer.Left = GraphOptions.Left
Layer.Right = GraphOptions.Right
Layer.Bottom = GraphOptions.Bottom
Layer.Top = GraphOptions.Top

try:
    import_to_global("qtiUtil")
    print "qtiUtil file successfully imported!"
except (ImportError, SyntaxError), ex:
    print "Failed to import qtiUtil file:",str(ex)
