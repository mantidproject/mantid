"""============================================================================
New Python command line interface for plotting in Mantid (ala matplotlib)
============================================================================

The idea behind this new module is to provide a simpler, more
homogeneous command line interface (CLI) to the Mantid plotting
functionality. This new interface is meant to resemble MatPlotLib as
far as possible, and provide a more manageable, limited number of plot
options.

The module is at a very early stage of development and provides
limited functionality. This is very much work in progress at the
moment. Feedback is very much welcome!

To use this new functionality you first need to import the new pyplot module:

    from mantidplot.future.pyplot import *

Please do not forget this step, otherwise you may get arcane error
messages from functions of the old mantidplot Python CLI.

Simple plots can be created and manipulated with a handul of
commands. See the following examples.

Plot an array (python list)
---------------------------

    plot([0.1, 0.3, 0.2, 4])
    # The list of values will be inserted in a workspace named 'array_dummy_workspace'


Plot a Mantid workspace
-----------------------

    # first, load a workspace. You can do this with a Load command or just from the GUI menus
    ws=Load("MAR11060.nxs", OutputWorkspace="foo")
    plot(ws)

The list of values will be inserted in a workspace named
'array_dummy_workspace'


The plot commands accept a list of options (kwargs) as parameters
passed by name. With these options you can modify plot properties,
such as line styles, colors, axis scale, etc. The following example
illustrates the use of a few options. You can refer to the list of
options provided further down in this document. In principle, any
combination of options is supported, as long as it makes sense!

Plot an array with a different style
------------------------------------

    a = [0.1, 0.3, 0.2, 4]
    plot(a)
    import numpy as np
    y = np.sin(np.linspace(-2.28, 2.28, 1000))
    plot(y)


If you have used the traditional Mantid command line interface in
Python you will probably remember the plotSpectrum, plotBin and plotMD
functions. These are supported in this new interface as shown in the
following examples.

Plot spectra using workspace objects and workspace names
--------------------------------------------------------

    # please make sure that you use the right path and file name
    mar=Load('/path/to/MAR11060.raw', , OutputWorkspace="MAR11060")
    plot('MAR11060', [10,100,500])
    plot(mar,[3, 500, 800])

Let's load one more workspace so we can see some examples with list of workspaces

    loq=Load('/path/to/LOQ48097.raw', OutputWorkspace="LOQ48097")

The next lines are all equivalent, you can use workspace objects or
names in the list passed to plot:

    plot([mar, 'LOQ48097'], [800, 900])
    plot([mar, loq], [800, 900])
    plot(['MAR11060', loq], [800, 900])

Here, plot is making a guess and plotting the spectra of these
workspaces. You can make that choice more explicit by specifying the
'tool' argument:

    plot(['MAR11060', loq], [800, 900], tool='plot_spectrum')

Plotting bins
-------------

    plot_bin(ws, [1, 5, 7, 100], linewidth=5):

Ploting MD workspaces
---------------------

    plot_md(md_ws):

Changing style properties
-------------------------

You can modify the style of your plots. For example like this (for a
full list of options currently supported, see below).

    lines=plot(loq, [1, 4], tool='plot_spectrum', linestyle='-.', marker='*')

Notice that the plot function returns a list of lines, which
correspond to the spectra lines. At present the lines have limited
functionality. Essentially, the data underlying these lines data can
be retrieved as follows:

    lines[0].get_xdata()
    lines[0].get_ydata()

If you use plot_spectrum, the number of elements in the output lines
should be equal to the number of bins in the corresponding
workspace. Conversely, if you use plot_bin, the number of elements in
the output lines should be equal to the number of spectra in the
workspace.

Other properties can be modified using different functions, as in
matplotlib's pyplot. For example:

    title('Test plot of LOQ')
    xlabel('ToF')
    ylabel('ToF')
    ylim(0, 8)
    xlim(1e3, 4e4)
    grid('on')


Style options supported as keyword arguments
--------------------------------------------

Unless otherwise stated, these options are in principle supported in
all the plot variants. These options have the same (or as closed as
possible) meaning as in matplotlib.

============  ================
Option name   Values supported
------------  ----------------
linewidth     real values
linestyle     '-', '--', '-.' '.'
marker        'o', 'v', '^', '<', '>', 's', '*', 'h', '|', '_'
color         color character or string ('b', 'blue', 'g', 'green', 'k', 'black', etc.)
============  ================

Functions that modify plot properties
-------------------------------------

Here is a list of the functions supported at the moment. The offer the
same functionality as their counterparts in matplotlib's pyplot.

- title
- xlabel
- ylabel
- ylim
- xlim
- axis
- grid
- savefig

"""
try:
    import _qti
except ImportError:
    raise ImportError('The \'mantidplot\' module can only be used from within MantidPlot.')

import numpy as np
from PyQt4 import Qt, QtGui, QtCore
from mantid.api import (MatrixWorkspace as MatrixWorkspace, AlgorithmManager as AlgorithmManager, AnalysisDataService as ADS)
from mantid.simpleapi import CreateWorkspace as CreateWorkspace
import mantidplot  

class Line2D():
    """
        A very minimal replica of matplotlib.Line.Line2D. The true Line2D
        is a sublcass of matplotlib.artist and provides tons of
        functionality. At the moment this just provides get_xdata()
        and get_ydata().  It also holds its Graph object and through
        it it would be possible to provide additional selected
        functionality. Keep in mind that providing GUI line/plot
        manipulation functionality would require a proxy for this
        class.
    """

    def __init__(self, graph, index, x_data, y_data):
        self._graph = graph
        self._index = index   # will (may) be needed to change properties of this line
        self._xdata = x_data
        self._ydata = y_data

    def get_xdata(self):
        return self._xdata

    def get_ydata(self):
        return self._ydata

__hold_status = False

__last_shown_graph = None

def __empty_graph():
    """
        Helper function, especially for the functional interface.
    """
    lines = plot([0])

def __last_graph():
    """
        Helper function, especially for the functional interface.
    """
    global __last_shown_graph
    if not __last_shown_graph:
        __last_shown_graph = __empty_graph()
    return __last_shown_graph


def __is_array(arg):
    """
        Is the argument a python or numpy list?
        @param arg :: argument
        
        Returns :: True if the argument a python or numpy list
    """
    return isinstance(arg, list) or isinstance(arg, np.ndarray) 

def __is_workspace(arg):
    """
        Is the argument a Mantid MatrixWorkspace?
        @param arg :: argument
        
        Returns :: True if the argument a MatrixWorkspace
    """
    return isinstance(arg, MatrixWorkspace)

def __is_array_of_workspaces(arg):
    """
        Is the argument a sequence of Mantid MatrixWorkspaces?
        @param arg :: argument
        
        Returns :: True if the argument is a sequence of  MatrixWorkspace
    """
    return __is_array(arg) and len(arg) > 0 and __is_workspace(arg[0])


def __create_workspace(x, y, name="array_dummy_workspace"):
    """
        Create a workspace. Also puts it in the ADS
        @param x :: x array
        @param y :: y array
        @param name :: workspace name
        
        Returns :: Workspace
    """    
    alg = AlgorithmManager.create("CreateWorkspace")
    alg.setChild(True) 
    alg.initialize()
    # fake empty workspace (when doing plot([]), cause setProperty needs non-empty data )
    if [] == x:
        x = [0]
    if [] == y:
        y = [0]
    alg.setProperty("DataX", x)
    alg.setProperty("DataY", y)
    alg.setPropertyValue("OutputWorkspace", name) 
    alg.execute()
    ws = alg.getProperty("OutputWorkspace").value
    ADS.addOrReplace(name, ws) # Cannot plot a workspace that is not in the ADS
    return ws


def __list_of_lines_from_graph(g):
    """
        Produces a python list of line objects, with one object per line plotted on the passed graph
        Note: at the moment these objects are of class PlotCurve != matplotlib.lines.Line2D

        @param g :: graph (with several plot layers = qti Multilayer)

        Returns :: List of line objects
    """
    if None == g:
        raise ValueError("Got empty Graph object, cannot get its lines." )
    # assume we use a single layer
    active = g.activeLayer()
    res = []
    for i in range(0, active.numCurves()):
        x_data = []
        y_data = []
        d = active.curve(i).data()
        for i in range(0, active.curve(i).data().size()):
            x_data.append(d.x(i))
            y_data.append(d.y(i))
        res.append(Line2D(g, i, x_data, y_data))

    global __last_shown_graph
    __last_shown_graph = g

    return res;

def __matplotlib_defaults(l):
    """
        Tries to approximately mimic the default plot properties of a pylab.plot()
        @param l :: layer (plot)

        Returns :: nothing, just modifies properties of the layer passed
    """
    if None == l:
        raise ValueError("Got empty Layer object, cannot modify its properties." )
    l.removeLegend()
    for i in range(0, l.numCurves()):
        l.setCurveLineColor(i, 0)  # beware this is not Qt.Qt.black
    l.setTitle(' ')
    l.setXTitle(' ')
    l.setYTitle(' ')

def __apply_linestyle_kwarg(graph, linestyle):
    """
        Applies a linestyle to all the curves of the active layer of the graph passed
        @param l :: graph (figure)

        Returns :: nothing, just modifies the line styles of the active layer of the graph passed
    """
    linestyle_to_qt_penstyle = {
        '-': QtCore.Qt.SolidLine, '--': QtCore.Qt.DashLine,
        '-.': QtCore.Qt.DashDotLine, '-.': QtCore.Qt.DotLine
    } # other available: Qt.DashDotDotLine, Qt.CustomDashLine
    wrong = 'inexistent'
    penstyle = linestyle_to_qt_penstyle.get(linestyle, wrong)
    if wrong == penstyle:
        raise ValueError("Wrong linestyle given, unrecognized: " + linestyle)
    l = graph.activeLayer()
    for i in range(0, l.numCurves()):
        l.setCurveLineStyle(0, penstyle)

__marker_to_plotsymbol = {
    'o': _qti.PlotSymbol.Ellipse, 'v': _qti.PlotSymbol.DTriangle, '^': _qti.PlotSymbol.UTriangle,
    '<': _qti.PlotSymbol.LTriangle, '>': _qti.PlotSymbol.RTriangle, 's': _qti.PlotSymbol.Rect,
    '*': _qti.PlotSymbol.Star1, 'h': _qti.PlotSymbol.Hexagon, '|': _qti.PlotSymbol.VLine,
    '_': _qti.PlotSymbol.HLine
}

def __apply_marker_kwarg(graph, marker):
    """
        Sets the marker of all the curves of the active layer of the graph passed
        @param graph :: a graph or figure that can hold multiple layers

        Returns :: nothing, just modifies the line markers of the active layer of the graph passed
    """
    wrong = 'inexistent'
    sym_code = __marker_to_plotsymbol.get(marker, wrong)
    if wrong == sym_code:
        raise ValueError("Wrong marker given, unrecognized: " + marker)
    l = graph.activeLayer()
    for i in range(0, l.numCurves()):
        sym = _qti.PlotSymbol(sym_code, QtGui.QBrush(), QtGui.QPen(), QtCore.QSize(5,5))
        l.setCurveSymbol(i, sym)

__color_char_to_color_idx = {
    'k': 0, 'b': 1, 'g': 2, 'r': 3
}

def __apply_line_color(graph, c):
    l = graph.activeLayer()
    col_idx = __color_char_to_color_idx[c]
    for i in range(0, l.numCurves()):
        l.setCurveLineColor(i, col_idx)  # beware this is not Qt.Qt.black

def __apply_linestyle(graph, linestyle):
    l = graph.activeLayer()
    idx = l.numCurves()-1
    l.setCurveLineStyle(idx, linestyle)

def __apply_marker(graph, marker):
    l = graph.activeLayer()
    wrong = 'inexistent'
    sym_code = __marker_to_plotsymbol.get(marker, wrong)
    if wrong == sym_code:
        raise ValueError("Warning: ignoring unrecognized marker: " + marker)
    sym = PlotSymbol(sym_code, QtGui.QBrush(), QtGui.QPen(), QtCore.QSize(5,5))
    idx = l.numCurves()-1
    l.setCurveSymbol(idx, sym)

def __is_marker(c):
    inex = 'inexistent'
    m = __marker_to_plotsymbol.get(marker, inex)
    return m != inex

def __is_linestyle(c):
    return '.'==c or '-'==c

def __apply_plot_args(graph, *args):
    """
        Applies args, like '-r' etc.
        @param graph :: a graph (or figure) that can contain multiple layers

        Returns :: nothing, just uses kwargs to modify properties of the layer passed
    """
    if None==graph or None==args or ((),) == args:
        return

    print "got args: ", args
    print "args len: ", len(args)
    for a in range(0, len(args)):
        if isinstance(a, basestring):
            for i, c in enumeraterange(0,len(s)):
                if __is_linestyle(c):   # TODO: this will be fooled!!! <- FIX
                    __apply_linestyle(a[i:])
                elif c.isalpha():
                    __apply_line_color(graph, c)
                elif __is_marker(c):
                    __apply_marker(graph, a[i:])
                else:
                    # TODO - decide - error here? like this? sure?
                    raise ValueError("Unrecognized character in input string: " + c)
        else:
            raise ValueError("Unrecognized input parameter: " + str(args[a]) + ", of type: " + str(type(a)))

def __apply_plot_kwargs(graph, **kwargs):
    """
        Applies kwargs
        @param graph :: a graph (or figure) that can contain multiple layers

        Returns :: nothing, just uses kwargs to modify properties of the layer passed
    """
    if None==graph or None==kwargs or ((),) == kwargs:
        return

    for key in kwargs:
        print "kwarg, keyword arg: %s: %s" % (key, kwargs[key])
        if 'linestyle' == key:
            __apply_linestyle_kwarg(graph, kwargs[key])

        elif 'linewidth' == key:
            print 'linewidth'
            l = graph.activeLayer()
            for i in range(0, l.numCurves()):
                l.setCurveLineWidth(i, kwargs[key])

        elif 'marker' == key:
            __apply_marker_kwarg(graph, kwargs[key])

def __plot_as_workspace(*args, **kwargs):
    """
        plotSpectrum via qti plotting framework to plot a workspace.

        @param args :: curve data and options.
        @param kwargs :: plot line options

        Returns :: List of line objects
    """
    # normally expects: args[0]: workspace(s), args[1]: one or more spectra indices
    if len(args) < 2:
        args = args + ([0],)
    return plot_spectrum(args[0], args[1], *args[2:], **kwargs)

def __plot_as_workspaces_list(*args, **kwargs):
    """
        Plot a series of workspaces
        @param args :: curve data and options.
        @param kwargs :: plot line options
        
        Returns :: List of line objects
    """
    # plotSpectrum can already handle 1 or more input workspaces.
    return __plot_as_workspace(*args, **kwargs)


def __plot_as_array(*args, **kwargs):
    """
        Plot from an array
        @param args :: curve data and options.
        @param kwargs :: plot line options
        
        Returns :: the list of curves (1) included in the plot
    """
    y = args[0]
    if len(args) > 1:
        if __is_array(args[1]):
            ws = __create_workspace(args[1], y)
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1) # 0 to n, incremented by 1.
        ws = __create_workspace(x, y)
    lines = __plot_as_workspace(ws, **kwargs)
    graph = None
    if len(lines) > 0:
        graph = lines[0]._graph
    else:
        raise Exception("Could not plot a workspace: " + ws)
    # something to improve: if the C++ Graph class provided a plot1D that doesn't do show(), so that
    # we could modify properties behind the scene and at the end do the show(). Con: do we really need
    # to load the qti layer with more methods because of outer layers like here?
    __matplotlib_defaults(graph.activeLayer())
    __apply_plot_args(graph, *args[1:])
    __apply_plot_kwargs(graph, **kwargs)
    return __list_of_lines_from_graph(graph)

def __plot_with_tool(tool, *args, **kwargs):
    if 'plot_bin' == tool:
        return plot_bin(args[0], args[1], args[2:], **kwargs)
    elif 'plot_md' == tool:
        return plot_md(args[0], args[1:], **kwargs)
    elif 'plot_spectrum' == tool:
        return plot_spectrum(args[0], args[1], args[2:], **kwargs)
    # here you would add slice/spectrum/instrument viewer, etc. and maybe you'll want to put them in a dict
    else:
        raise ValueError("Unrecognized tool specified: '" + tool + ";. Cannot plot this. ")

def __plot_with_best_guess(*args, **kwargs):
    y = args[0]
    print type(y)
    if __is_array(y):
        if __is_array_of_workspaces(y):
            return __plot_as_workspaces_list(*args, **kwargs)
        else:
            return __plot_as_array(*args, **kwargs)
    else:
        # mantidplot.plotSpectrum can handle workspace names (strings)
        return __plot_as_workspace(*args, **kwargs)

def plot_bin(source, indices, *args, **kwargs):
    """
    1D plot of a MDWorkspace.

    @param source :: workspace or list of workspaces (both workspace objects and names accepted)
    @param indices :: indices of the bin(s) to plot

    Returns :: the list of curves included in the plot
    """
    graph = mantidplot.plotBin(source, indices)  # TODO fix window param
    __apply_plot_args(graph, *args)
    __apply_plot_kwargs(graph, **kwargs)
    return __list_of_lines_from_graph(graph)


def plot_md(source, *args, **kwargs):
    """
    1D plot of an MDWorkspace.

    @param source :: workspace or list of workspaces (both workspace objects and names accepted)

    Returns :: the list of curves included in the plot
    """
    graph = mantidplot.plotMD(source)  # TODO fix window param
    __apply_plot_args(graph, *args)
    __apply_plot_kwargs(graph, **kwargs)
    return __list_of_lines_from_graph(graph)


def plot_spectrum(source, indices, *args, **kwargs):
    """
    1D Plot of a spectrum in a workspace.

    This plots one or more spectra, using X as the bin boundaries,
    and Y as the counts in each bin.

    @param source :: workspace or list of workspaces (both workspace objects and names accepted)
    @param indices :: indices of the spectra to plot, given as a list

    Returns :: the list of curves included in the plot
    """
    graph = mantidplot.plotSpectrum(source, indices)  # TODO fix window param
    __apply_plot_args(graph, *args)
    __apply_plot_kwargs(graph, **kwargs)
    return __list_of_lines_from_graph(graph)


def plot(*args, **kwargs):
    """
        Plot the data in various forms depending on what arguments are passed.
        Currently supported inputs: arrays (as Python lists or numpy arrays) and workspaces
        (by name or workspace objects).

        @param args :: curve data and options
        @param kwargs :: plot line options
        
        Returns :: the list of curves included in the plot
    """
    nargs = len(args)
    if nargs < 1:
        raise ValueError("Must provide data to plot")

    # TODO: multiline

    # normally guess; exception if e.g. a parameter tool='plot_bin' is given
    try:
        tool = kwargs['tool']
        del kwargs['tool']
        return __plot_with_tool(tool, *args, **kwargs)
    except KeyError:
        return __plot_with_best_guess(*args, **kwargs)


# Functions, pyplot / old matlab style

"""
   Set the boundaries of the x axis

   @param xmin :: minimum value
   @param xmax :: maximum value
"""
def xlim(xmin, xmax):
    l = __last_graph().activeLayer()
    l.setAxisScale(0, xmin, xmax)

"""
   Set the boundaries of the y axis

   @param ymin :: minimum value
   @param ymax :: maximum value
"""
def ylim(ymin, ymax):
    l = __last_graph().activeLayer()
    l.setAxisScale(1, ymin, ymax)

"""
   Set the label or title of the x axis

   @param lbl :: x axis lbl
"""
def xlabel(lbl):
    l = __last_graph().activeLayer()
    l.setXTitle(lbl)

"""
   Set the label or title of the y axis

   @param lbl :: y axis lbl
"""
def xlabel(lbl):
    l = __last_graph().activeLayer()
    l.setYTitle(lbl)

"""
   Set title of active plot

   @param title :: title string
"""
def title(title):
    l = __last_graph().activeLayer()
    l.setTitle(title)

"""
   Set the boundaries of the x and y axes

   @param lims :: list or vector specifying min x, max x, min y, max y
"""
def axis(lims):
    l = __last_graph().activeLayer()
    if 4 != len(lims):
        raise ValueError("Error: 4 real values are required for the x and y axes limits")
    l.setScale(lims)

"""
   Set title of active plot

   @param title :: title string
"""
def grid(opt):
    l = __last_graph().activeLayer()
    if None == opt or 'on' == opt:
        l.showGrid()
    elif 'off' == opt:
        # TODO is there support for a 'hideGrid' in qti?
        print "Sorry, disabling grids is currenlty not supported"

"""
   Save current plot into a file. The format is guessed from the file extension (.eps, .png, .jpg, etc.)

   @param name :: file name
"""
def savefig(name):
    if not name:
        raise ValueError("Error: you need to specify a non-empty file name")
    l = __last_graph().activeLayer()
    l.saveImage(name);
