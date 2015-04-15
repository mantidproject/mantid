"""============================================================================
New Python command line interface for plotting in Mantid (a la matplotlib)
============================================================================

The idea behind this new module is to provide a simpler, more
homogeneous command line interface (CLI) to the Mantid plotting
functionality. This new interface is meant to resemble matplotlib as
far as possible, and to provide a more manageable, limited number of
plot options.

The module is at a very early stage of development and provides
limited functionality. This is very much work in progress at the
moment. The module is subject to changes and feedback is very much
welcome!

Simple plots can be created and manipulated with a handul of
commands. See the following examples.

Plot an array (python list)
---------------------------

.. code-block:: python

    # plot array
    plot([0.1, 0.3, 0.2, 4])
    # plot x-y
    plot([0.1, 0.2, 0.3, 0.4], [1.2, 1.3, 0.2, 0.8])

Plot an array with a different style
------------------------------------

The plot commands that are described here accept a list of options
(kwargs) as parameters passed by name. With these options you can
modify plot properties, such as line styles, colors, axis scale,
etc. The following example illustrates the use of a few options. You
can refer to the list of options provided further down in this
document. In principle, any combination of options is supported, as
long as it makes sense!

.. code-block:: python

    a = [0.1, 0.3, 0.2, 4]
    plot(a)
    import numpy as np
    y = np.sin(np.linspace(-2.28, 2.28, 1000))
    plot(y, linestyle='-.', marker='o', color='red')

If you have used the traditional Mantid command line interface in
Python you will probably remember the plotSpectrum, plotBin and plotMD
functions. These are supported in this new interface as shown in the
following examples.

Plot a Mantid workspace
-----------------------

You can pass one or more workspaces to the plot function. By default
it will plot the spectra of the workspace(s), selecting them by the
indices specified in the second argument. This behavior is similar to
the plotSpectrum function of the traditional mantidplot module. This is
a simple example that produces plots of spectra:

.. code-block:: python

    # first, load a workspace. You can do this with a Load command or just from the GUI menus
    ws = Load("/path/to/MAR11060.raw", OutputWorkspace="foo")
    # 1 spectrum plot
    plot(ws, 100)
    # 3 spectra plot
    plot(ws, [100, 101, 102])

========================
Different types of plots
========================

The plot() function provides a unified interface to different types of
plots, including specific graphs of spectra, bins, multidimensional
workspaces, etc. These specific types of plots are explained in the
next sections. plot() makes a guess as to what tool to use to plot a
workspace. For example, if you pass an MD workspace it will make an MD
plot. But you can request a specific type of plot by specifying a
keyword argument ('tool'). The following tools (or different types of
plots) are supported:

+------------------------+------------------------------------------------------------+-----------------------+
| Tool                   | tool= parameter values (all are equivalent aliases)        | Old similar function  |
+========================+============================================================+=======================+
| plot spectra (default) | 'plot_spectrum', 'spectrum', 'plot_sp', 'sp'               | plotSpectrum          |
+------------------------+------------------------------------------------------------+-----------------------+
| plot bins              | 'plot_bin', 'bin'                                          | plotBin               |
+------------------------+------------------------------------------------------------+-----------------------+
| plot MD                | 'plot_md', 'md'                                            | plotMD                |
+------------------------+------------------------------------------------------------+-----------------------+

The last column of the table lists the functions that produce similar
plots in the traditional MantidPlot Python plotting interface. For the
time being this module only supports these types of specific
plots. Note that the traditional plotting interface of MantidPlot
provides support for many more specific types of plots. These or
similar ones will be added in this module in future releases:

* plot2D
* plot3D
* plotSlice
* instrumentWindow
* waterFallPlot
* mergePlots
* stemPlot

Plot spectra using workspace objects and workspace names
--------------------------------------------------------

It is also possible to pass workspace names to plot, as in the
following example where we plot a few spectra:

.. code-block:: python

    # please make sure that you use the right path and file name
    mar = Load('/path/to/MAR11060.raw', OutputWorkspace="MAR11060")
    plot('MAR11060', [10,100,500])
    plot(mar,[3, 500, 800])

Let's load one more workspace so we can see some examples with list of
workspaces

.. code-block:: python

    loq=Load('/path/to/LOQ48097.raw', OutputWorkspace="LOQ48097")

The next lines are all equivalent, you can use workspace objects or
names in the list passed to plot:

.. code-block:: python

    plot([mar, 'LOQ48097'], [800, 900])
    plot([mar, loq], [800, 900])
    plot(['MAR11060', loq], [800, 900])

Here, the plot function is making a guess and plotting the spectra of
these workspaces (instead of the bins or anything else). You can make
that choice more explicit by specifying the 'tool' argument. In this
case we use 'plot_spectrum' (which also has shorter aliases:
'spectrum', or simply 'sp' as listed in the table above):

.. code-block:: python

    plot(['MAR11060', loq], [800, 900], tool='plot_spectrum')
    plot(['MAR11060', loq], [801, 901], tool='sp')

Alternatively, you can use the plot_spectrum command, which is
equivalent to the plot command with the keyword argument
tool='plot_spectrum':

.. code-block:: python

    plot_spectrum(['MAR11060', loq], [800, 900])

Plotting bins
-------------

To plot workspace bins you can use the keyword 'tool' with the value
'plot_bin' (or equivalent 'bin'), like this:

.. code-block:: python

    ws = Load('/path/to/HRP39182.RAW', OutputWorkspace="HRP39182")
    plot(ws, [1, 5, 7, 100], tool='plot_bin')

or, alternatively, you can use the plot_bin command:

.. code-block:: python

    plot_bin(ws, [1, 5, 7, 100], linewidth=4, linestyle=':')

Plotting MD workspaces
----------------------

Similarly, to plot MD workspaces you can use the keyword 'tool' with
the value 'plot_md' (or 'md' as a short alias), like this:

.. code-block:: python

    simple_md_ws = CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace=MDWWorkspaceName)
    plot(simple_md_ws, tool='plot_md')

or a specific plot_md command:

.. code-block:: python

    plot_md(simple_md_wsws)

For simplicity, these examples use a dummy MD workspace. Please refer
to the Mantid (http://www.mantidproject.org/MBC_MDWorkspaces) for a
more real example, which necessarily gets more complicated and data
intensive.

=========================
Changing style properties
=========================

You can modify the style of your plots. For example like this (for a
full list of options currently supported, see below).

.. code-block:: python

    lines = plot(loq, [100, 104], tool='plot_spectrum', linestyle='-.', marker='*', color='red')

Notice that the plot function returns a list of lines, which
correspond to the spectra lines. At present the lines have limited
functionality. Essentially, the data underlying these lines can be
retrieved as follows:

.. code-block:: python

    lines[0].get_xdata()
    lines[0].get_ydata()

If you use plot_spectrum, the number of elements in the output lines
should be equal to the number of bins in the corresponding
workspace. Conversely, if you use plot_bin, the number of elements in
the output lines should be equal to the number of spectra in the
workspace.

To modify the figure, you first need to obtain the figure object
that represents the figure where the lines are displayed. Once you do
so you can for example set the title of the figure like this:

.. code-block:: python

    fig = lines[0].figure()
    fig.suptitle('Example figure title')

Other properties can be modified using different functions, as in
matplotlib's pyplot. For example:

.. code-block:: python

    title('Test plot of LOQ')
    xlabel('ToF')
    ylabel('Counts')
    ylim(0, 8)
    xlim(1e3, 4e4)
    xscale('log')
    grid('on')

By default, these functions manipulate the current figure (the last or
most recently shown figure). You can also save the current figure into
a file like this:

.. code-block:: python

    savefig('example_saved_figure.png')

where the file format is guessed from the file extension. The same
extensions as in the MantidPlot figure export dialog are supported,
including jpg, png, tif, ps, and svg.

The usage of these functions very similar to the matlab and/or
pyplot functions with the same names. The list of functions
currently supported is provided further below.

Additional options supported as keyword arguments (kwargs):
-----------------------------------------------------------

There is a couple of important plot options that are set as keyword
arguments:


+------------+------------------------+
|Option name | Values supported       |
+============+========================+
|error_bars  | True, False (default)  |
+------------+------------------------+
|hold        | on, off                |
+------------+------------------------+

error_bars has the same meaning as in the traditional mantidplot plot
functions: it defines whether error bars should be added to the
plots. hold has the same behavior as in matplotlib and pyplot. If the
value of hold is 'on' in a plot command, the new plot will be drawn on
top of the current plot window, without clearing it. This makes it
possible to make plots incrementally.

For example, one can add two spectra from a workspace using the
following command:

.. code-block:: python

    lines = plot(loq, [100, 102], linestyle='-.', color='red')

But similar results can be obtained by plotting one of the spectra by
a first command, and then plotting the second spectra in a subsequent
command with the hold parameter enabled:

.. code-block:: python

    lines = plot(loq, 100, linestyle='-.', color='red')
    lines = plot(loq, 102, linestyle='-.', color='blue', hold='on')

After the two commands above, any subsequent plot command that passes
hold='on' as a parameter would add new spectra into the same plot. An
alternative way of doing this is explained next. Note however that
using the hold property to combine different types of plots
(plot_spectrum, plot_bin, etc.) will most likely produce useless
results.

Multi-plot commands
-------------------

In this version of pyplot there is limited support for multi-plot
commands (as in pyplot and matlab). For example, you can type commands
like the following:

.. code-block:: python

    plot(ws, [100, 101], 'r', ws, [200, 201], 'b', tool='plot_spectrum')

This command will plot spectra 100 and 101 in red and spectra 200 and
201 in blue on the same figure. You can also combine different
workspaces, for example:

.. code-block:: python

    plot(ws, [100, 101], 'r', mar, [50, 41], 'b', tool='plot_spectrum')


Style options supported as keyword arguments
--------------------------------------------

Unless otherwise stated, these options are in principle supported in
all the plot variants. These options have the same (or as closed as
possible) meaning as in matplotlib.

+------------+---------------------------------------------------------+
|Option name | Values supported                                        |
+============+=========================================================+
|linewidth   | real values                                             |
+------------+---------------------------------------------------------+
|linestyle   | '-', '--', '-.' '.'                                     |
+------------+---------------------------------------------------------+
|marker      |  'o', 'v', '^', '<', '>', 's', '*', 'h', '|', '_'       |
+------------+---------------------------------------------------------+
|color       |  color character or string ('b', 'blue', 'g', 'green',  |
|            |  'k', 'black', 'y', 'yellow', 'c', 'cyan', 'r', 'red'.  |
|            |  'm', 'magenta', etc.). RGB colors are not supported at |
|            |  the moment.                                            |
+------------+---------------------------------------------------------+

Modifying the plot axes
-----------------------

You can modify different properties of the plot axes via functions, as
seen before. This includes the x and y axis titles, limits and scale
(linear or logarithmic). For example:

.. code-block:: python

    ylabel('Counts')
    ylim(0, 8)
    yscale('log')

An alternative is to use equivalent methods provided by the Figure and
Axes objects. For this you first need to retrieve the figure and axes
where a plot (or line) has been shown.

.. code-block:: python

    lines = plot(mar,[3, 500, 800])
    fig = lines[0].figure()
    all_ax = fig.axes()    # fig.axes() returns in principle a list
    ax = all_ax[0]         #  but we only use one axes
    ax.set_ylabel('Counts')
    ax.set_xlabel('ToF')
    ax.set_ylim(0, 8)
    ax.set_xlim(1e2, 4e4)
    ax.set_xscale('log')

Functions that modify plot properties
-------------------------------------

Here is a list of the functions supported at the moment. They offer
the same functionality as their counterparts in matplotlib's
pyplot.

- title
- xlabel
- ylabel
- ylim
- xlim
- axis
- xscale
- yscale
- grid
- savefig

This is a limited list of functions that should be sufficient for
basic plots. These functions are presently provided as an example of
this type of interface, and some of them provide functionality similar
or equivalent to several of the keyword arguments for plot commands
detailed in this documentation. Some others produce results equivalent
to the more object oriented methods described above. For example, the
function xlabel is equivalent to the method set_xlabel applied on the
Axes object for the current figure.

This module is by default imported into the standard MantidPlot
namespace. You can use the functions and classes included here without
any prefix or adding this module name prefix (pymantidplot.pyplot), as
in the following example:

.. code-block:: python

    # Two equivalent lines:
    pymantidplot.pyplot.plot([1, 3, 2])
    plot([1, 3, 2])

Note that the plot() function of this module has replaced the
traditional plot() function of MantidPlot which has been moved into a
package called qtiplot. To use it you can do as follows:

.. code-block:: python

    pymantidplot.qtiplot.plot('MAR11060', [800, 801])
    # or if you prefer shorter prefixes:
    import pymantidplot.qtiplot as qtiplt
    qtiplt.plot('MAR11060', [800, 801])


Below is the reference documentation of the classes and functions
included in this module.

"""
# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>

try:
    import _qti
except ImportError:
    raise ImportError('The \'mantidplot\' and \'pymantidplot.pyplot\' modules can only be used from within MantidPlot.')

import numpy as np
from PyQt4 import Qt, QtGui, QtCore
from mantid.api import (IMDWorkspace as IMDWorkspace, MatrixWorkspace as MatrixWorkspace, AlgorithmManager as AlgorithmManager, AnalysisDataService as ADS)
from mantid.api import mtd
#    return __is_workspace(arg) or (mantid.api.mtd.doesExist(arg) and isinstance(mantid.api.mtd[arg], mantid.api.IMDWorkspace))
from mantid.simpleapi import CreateWorkspace as CreateWorkspace
import mantidplot  

print ("You are loading '" + __name__ + "', which is an experimental module." +
"""
Please note: this module is at a very early stage of development and
provides limited functionality. It is work in progress and is subject
to change. Feedback is very much welcome! Please let us know any wishes
and suggestions.""")

class Line2D():
    """
    A very minimal replica of matplotlib.Line.Line2D. The true Line2D
    is a sublcass of matplotlib.artist and provides tons of
    functionality. At the moment this just provides get_xdata(),
    get_ydata(), and figure() methods.  It also holds its Graph
    object and through it it would be possible to provide
    additional selected functionality. Keep in mind that providing
    GUI line/plot manipulation functionality would require a proxy
    for this class.
    """

    def __init__(self, graph, index, x_data, y_data, fig=None):
        self._graph = graph
        self._index = index   # will (may) be needed to change properties of this line
        self._xdata = x_data
        self._ydata = y_data
        self._fig = fig

    def get_xdata(self):
        return self._xdata

    def get_ydata(self):
        return self._ydata

    def figure(self):
        return self._fig

class Axes():
    """
    A very minimal replica of matplotlib.axes.Axes. The true Axes is a
    sublcass of matplotlib.artist and provides tons of functionality.
    At the moment this just provides a few set methods for properties
    such as labels and axis limits.
    """

    """Many plot manipulation functions that are provided in
    matplolib through Axes objects, for example to manipulate the x/y
    ticks, are not currently supported. Objects of this class hold
    their Figure object.  Presently every figure has a single Axes
    object, and there is no support for multiple axes (as in
    fig.add_axes() or fix.axes()).
    """

    def __init__(self, fig, xscale='linear', yscale='linear'):
        self._fig = fig
        # state of x and y scale kept here. C++ Graph.isLog() not yet exposed
        self._xscale = xscale
        self._yscale = yscale

    def axis(self, lims):
        """
        Set the boundaries or limits of the x and y axes

        @param lims :: list or vector specifying min x, max x, min y, max y
        """
        l = __last_fig()._graph.activeLayer()
        if 4 != len(lims):
            raise ValueError("Error: 4 real values are required for the x and y axes limits")
        l.setScale(*lims)

    def set_xlabel(self, lbl):
        """
        Set the label or title of the x axis

        @param lbl :: x axis lbl
        """
        l = self.get_figure()._graph.activeLayer()
        l.setXTitle(lbl)

    def set_ylabel(self, lbl):
        """
        Set the label or title of the y axis

        @param lbl :: y axis lbl
        """
        l = self.get_figure()._graph.activeLayer()
        l.setYTitle(lbl)

    def set_xlim(self, xmin, xmax):
        """
        Set the boundaries of the x axis

        @param xmin :: minimum value
        @param xmax :: maximum value
        """
        l = self.get_figure()._graph.activeLayer()
        l.setAxisScale(2, xmin, xmax)

    def set_ylim(self, ymin, ymax):
        """
        Set the boundaries of the y axis

        @param ymin :: minimum value
        @param ymax :: maximum value
        """
        l = self.get_figure()._graph.activeLayer()
        l.setAxisScale(0, ymin, ymax)

    def set_xscale(self, scale_str):
        """
        Set the type of scale of the x axis

        @param scale_str :: either 'linear' for linear scale or 'log' for logarithmic scale
        """
        if 'log' != scale_str and 'linear' != scale_str:
            raise ValueError("You need to specify either 'log' or 'linear' type of scale for the x axis." )

        l = self.get_figure()._graph.activeLayer()
        if scale_str == 'log':
            if 'log' == self._yscale:
                l.logLogAxes()
            else:
                l.logXLinY()
        elif scale_str == 'linear':
            if 'log' == self._yscale:
                l.logYlinX()
            else:
                l.linearAxes()
        self._xscale = scale_str

    def set_yscale(self, scale_str):
        """
        Set the type of scale of the y axis

        @param scale_str :: either 'linear' for linear scale or 'log' for logarithmic scale
        """
        if 'log' != scale_str and 'linear' != scale_str:
            raise ValueError("You need to specify either 'log' or 'linear' type of scale for the y axis." )

        l = self.get_figure()._graph.activeLayer()
        if scale_str == 'log':
            if 'log' == self._xscale:
                l.logLogAxes()
            else:
                l.logYlinX()
        elif scale_str == 'linear':
            if 'log' == self._xscale:
                l.logXLinY()
            else:
                l.linearAxes()
        self._yscale = scale_str

    def get_figure(self, ):
        """
        Get the figure where this Axes object is included

        Returns :: figure object for the figure that contains this Axes
        """
        return self._fig


class Figure():
    """
    A very minimal replica of matplotlib.figure.Figure. This class is
    here to support manipulation of multiple figures from the command
    line.
    """

    """For the moment this is just a very crude wrapper for Graph
    (proxy to qti Multilayer), and it is here just to hide (the rather
    obscure) Graph from users.
    """
    # Holds the set of figure ids (integers) as they're being created and/or destroyed
    __figures = {}
    # Always increasing seq number, not necessarily the number of figures in the dict
    __figures_seq = 0

    def __init__(self, num):
        if isinstance(num, int):
            # normal matplotlib use, like figure(2)
            missing = -1
            fig = Figure.__figures.get(num, missing)
            if missing == fig:
                self._graph = Figure.__empty_graph()
                Figure.__figures[num] = self
                if num > Figure.__figures_seq:
                    Figure.__figures_seq = num
                self._axes = Axes(self)
            else:
                if None == fig._graph._getHeldObject():
                    # has been destroyed!
                    self._graph = Figure.__empty_graph()
                    self._axes = Axes(self)
                else:
                    self._graph = fig._graph
                    self._axes = fig._axes
        elif isinstance(num, mantidplot.proxies.Graph):
            if None == num._getHeldObject():
                # deleted Graph!
                self._graph = Figure.__empty_graph()
            else:
                self._graph = num
            num = Figure.__make_new_fig_number()
            Figure.__figures[num] = self
            self._axes = Axes(self)
        else:
            raise ValueError("To create a Figure you need to specify a figure number or a Graph object." )

    def suptitle(self, title):
        """
        Set a title for the figure

        @param title :: title string
        """
        l = self._graph.activeLayer()
        l.setTitle(title)

    def axes(self):
        """
        Obtain the list of axes in this figure.

        Returns :: list of axes. Presently only one Axes object is supported
                   and this method returns a single object list
        """
        return [self._axes]

    def savefig(self, name):
        """
        Save current plot into a file. The format is guessed from the file extension (.eps, .png, .jpg, etc.)

        @param name :: file name
        """
        if not name:
            raise ValueError("Error: you need to specify a non-empty file name")
        l = _graph.activeLayer()
        l.saveImage(name);

    @classmethod
    def fig_seq(cls):
        """ Helper method, returns the current sequence number for figures"""
        return cls.__figures_seq

    @classmethod
    def __make_new_fig_number(cls):
        """ Helper method, creates and return a new figure number"""
        num = cls.__figures_seq
        avail = False
        while not avail:
            missing = -1
            fig = cls.__figures.get(num, missing)
            if missing == fig:
                avail = True   # break
            else:
                num += 1
        cls.__figures_seq = num
        return num

    @staticmethod
    def __empty_graph():
        """Helper method, just create a new Graph with an 'empty' plot"""
        lines = plot([0])
        return lines._graph


def __empty_fig():
    """Helper function, for the functional interface. Makes a blank/empty figure"""
    lines = plot([0]) # for the very first figure, this would generate infinite recursion!
    return Figure(lines[0]._graph)

# TODO/TOTHINK: no 'hold' function support for now. How to handle multi-plots with different types/tools? Does it make sense at all?
__hold_status = False

__last_shown_fig = None

def __last_fig():
    """
        Helper function, especially for the functional interface.
        Avoid using it inside the plot_spectrum, plot_bin, etc. as there's risk of infinite recursion
        Returns :: last figure, creating new one if there was none
    """
    global __last_shown_fig
    if not __last_shown_fig:
        f = __empty_fig()
        __last_shown_fig = f
    return __last_shown_fig

def __update_last_shown_fig(g):
    """
        Helper function, especially for the functional interface.
        @param g :: graph object
        Returns :: new last fig
    """
    global __last_shown_fig
    __last_shown_fig = Figure(g)
    return __last_shown_fig

def __is_array(arg):
    """
        Is the argument a python or numpy list?
        @param arg :: argument
        
        Returns :: True if the argument is a python or numpy list
    """
    return isinstance(arg, list) or isinstance(arg, np.ndarray) 

def __is_array_or_int(arg):
    """
        Is the argument a valid workspace index/indices, which is to say:
        Is the argument an int, or python or numpy list?
        @param arg :: argument

        Returns :: True if the argument is an integer, or a python or numpy list
    """
    return isinstance(arg, int) or __is_array(arg)


def __is_registered_workspace_name(arg):
    """"
        Check whether the argument passed is the name of a registered workspace

        @param arg :: argument (supposedly a workspace name)

        Returns :: True if arg is a correct workspace name
    """
    return (isinstance(arg, basestring) and mtd.doesExist(arg) and isinstance(mtd[arg], IMDWorkspace))

def __is_valid_single_workspace_arg(arg):
    """"
        Check whether the argument passed can be used as a workspace input. Note that this differs from
        __is_workspace() in that workspace names are also accepted. Throws ValueError with informative
        message if arg is not a valid workspace object or name.

        @param arg :: argument (supposedly one workspace, possibly given by name)

        Returns :: True if arg can be accepted as a workspace
    """
    if __is_workspace(arg) or __is_registered_workspace_name(arg):
        return True
    else:
        return False

def __is_valid_workspaces_arg(arg):
    """"
        Check whether the argument passed can be used as a workspace(s) input. Note that this differs from
        __is_workspace() in that lists of workspaces and workspace names are also accepted.

        @param arg :: argument (supposedly one or more workspaces, possibly given by name)

        Returns :: True if arg can be accepted as a workspace or a list of workspaces
    """
    if __is_valid_single_workspace_arg(arg):
        return True
    else:
        if 0 == len(arg):
            return False
        for name in arg:
            # name can be a workspace name or a workspace object
            try:
                __is_valid_single_workspace_arg(name)
            except:
                raise ValueError("This parameter passed in a list of workspaces is not a valid workspace: " + str(name))
    return True

def __is_data_pair(a, b):
    """
        Are the two arguments passed (a and b) a valid data pair for plotting, like in plot(x, y) or
        plot(ws, [0, 1, 2])?
        @param a :: first argument passed (supposedly array or workspace(s))
        @param b :: second argument (supposedly an array of values, or indices)

        Returns :: True if the arguments can be used to plot something is an integer, or a python or numpy list
    """
    res = (__is_array(a) and __is_array(b)) or (__is_valid_workspaces_arg(a) and __is_array_or_int(b))
    return res

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


def __create_workspace(x, y, name="__array_dummy_workspace"):
    """
        Create a workspace. Also puts it in the ADS with __ name
        @param x :: x array
        @param y :: y array
        @param name :: workspace name
        
        Returns :: Workspace
    """    
    alg = AlgorithmManager.create("CreateWorkspace")
    alg.setChild(True) 
    alg.initialize()
    # fake empty workspace (when doing plot([]), cause setProperty needs non-empty data)
    if [] == x:
        x = [0]
    if [] == y:
        y = [0]
    alg.setProperty("DataX", x)
    alg.setProperty("DataY", y)
    name = name + "_" + str(Figure.fig_seq())
    alg.setPropertyValue("OutputWorkspace", name) 
    alg.execute()
    ws = alg.getProperty("OutputWorkspace").value
    ADS.addOrReplace(name, ws) # Cannot plot a workspace that is not in the ADS
    return ws


def __list_of_lines_from_graph(g, first_line=0):
    """
        Produces a python list of line objects, with one object per line plotted on the passed graph
        Note: at the moment these objects are of class Line2D which is much simpler than matplotlib.lines.Line2D
        This function should always be part of the process of creating a new figure/graph, and it guarantees
        that this figure being created is registered as the last shown figure.

        @param g :: graph (with several plot layers = qti Multilayer)
        @param first_line :: index to start from (useful for hold='on', multi-plots, etc.)

        Returns :: List of line objects
    """
    if None == g:
        raise ValueError("Got empty Graph object, cannot get its lines." )
    # assume we use a single layer
    active = g.activeLayer()
    res = []
    for i in range(first_line, active.numCurves()):
        x_data = []
        y_data = []
        d = active.curve(i).data()
        for i in range(0, active.curve(i).data().size()):
            x_data.append(d.x(i))
            y_data.append(d.y(i))
        res.append(Line2D(g, i, x_data, y_data))

    fig = __update_last_shown_fig(g)
    for lines in res:
        lines._fig = fig

    return res;

def __matplotlib_defaults(l):
    """
        Tries to (approximately) mimic the default plot properties of a pylab.plot()
        @param l :: layer (plot) from a mantidplot Graph object

        Returns :: nothing, just modifies properties of the layer passed
    """
    if None == l:
        raise ValueError("Got empty Layer object, cannot modify its properties." )
    l.removeLegend()
    for i in range(0, l.numCurves()):
        l.setCurveLineColor(i, __color_char_to_color_idx['b'])
    l.setTitle(' ')
    l.setXTitle(' ')
    l.setYTitle(' ')

__marker_to_plotsymbol = {
    'o': _qti.PlotSymbol.Ellipse, 'v': _qti.PlotSymbol.DTriangle, '^': _qti.PlotSymbol.UTriangle,
    '<': _qti.PlotSymbol.LTriangle, '>': _qti.PlotSymbol.RTriangle, 's': _qti.PlotSymbol.Rect,
    '*': _qti.PlotSymbol.Star1, 'h': _qti.PlotSymbol.Hexagon, '|': _qti.PlotSymbol.VLine,
    '_': _qti.PlotSymbol.HLine
}

"""Contains all the supported line styles"""
__linestyle_to_qt_penstyle = {
    '-': QtCore.Qt.SolidLine, '--': QtCore.Qt.DashLine,
    '-.': QtCore.Qt.DashDotLine, ':': QtCore.Qt.DotLine
} # other available: Qt.DashDotDotLine, Qt.CustomDashLine

def __apply_linestyle(graph, linestyle, first_line=0):
    """
        Sets the linestyle of lines/curves of the active layer of the graph passed

        @param graph :: mantidplot graph (figure)
        @param linestyle :: linestyle string
        @param first_line :: index of first line to which the linestyle will apply
                             (useful when in hold mode / adding lines)

        Returns :: nothing, just modifies the line styles of the active layer of the graph passed
    """
    global __linestyle_to_qt_penstyle
    wrong = 'inexistent'
    penstyle = __linestyle_to_qt_penstyle.get(linestyle, wrong)
    if wrong == penstyle:
        raise ValueError("Wrong linestyle given, unrecognized: " + linestyle)
    l = graph.activeLayer()
    for i in range(first_line, l.numCurves()):
        l.setCurveLineStyle(i, penstyle)

# beware this is not Qt.Qt.color_name (black, etc.)
__color_char_to_color_idx = {
    'k': 0, 'r': 1, 'g': 2, 'b': 3, 'c': 4, 'm': 5, 'y': 18,
    'black': 0, 'red': 1, 'green': 2, 'blue': 3, 'cyan': 4, 'magenta': 5, 'orange': 6,
    'purple': 7, 'darkGreen': 8, 'darkBlue': 9, 'brown': 10, 'gray': 17, 'yellow': 18
}

def __apply_line_color(graph, c, first_line=0):
    """
        Sets the color of curves of the active layer of the graph passed

        @param graph :: mantidplot graph (figure)
        @param c :: color string
        @param first_line :: index of first line to which the color will apply
                             (useful when in hold mode / adding lines)

        Returns :: nothing, just modifies the line styles of the active layer of the graph passed
    """
    inex = 'inexistent'
    col_idx = __color_char_to_color_idx.get(c, inex)
    if inex == col_idx:
        col_idx = QtGui.QColor(c)
    l = graph.activeLayer()
    for i in range(first_line, l.numCurves()):
        l.setCurveLineColor(i, col_idx) # beware this is not Qt.Qt.black, but could be faked with QtGui.QColor("orange")

def __apply_marker(graph, marker, first_line=0):
    """
        Sets the marker of curves of the active layer of the graph passed

        @param graph :: mantidplot graph (figure)
        @param marker :: line marker character
        @param first_line :: index of first line to which the color will apply
                             (useful when in hold mode / adding lines)

        Returns :: nothing
    """
    wrong = 'inexistent'
    sym_code = __marker_to_plotsymbol.get(marker, wrong)
    if wrong == sym_code:
        raise ValueError("Warning: unrecognized marker: " + str(marker))
    sym = _qti.PlotSymbol(sym_code, QtGui.QBrush(), QtGui.QPen(), QtCore.QSize(5,5))
    l = graph.activeLayer()
    for idx in range(first_line, l.numCurves()):
        l.setCurveSymbol(idx, sym)

def __is_marker(char):
    """ Is it a marker character
        @param char :: suspected marker character coming from a linestyle string
        Returns :: True if it's a marker character
    """
    inex = 'inexistent'
    m = __marker_to_plotsymbol.get(char, inex)
    return m != inex

__linestyle_to_qt_penstyle = {
    '-': QtCore.Qt.SolidLine, '--': QtCore.Qt.DashLine,
    '-.': QtCore.Qt.DashDotLine, ':': QtCore.Qt.DotLine
} # other available: Qt.DashDotDotLine, Qt.CustomDashLine

def __is_linestyle(stl, i):
    """
        Check if we have a linestyle string in string s at position i
        @param stl :: input (style) string, for example: '-.g', 'r', ':b'
        @param i :: index where to start checking in string s

        Returns :: 0 if no linestyle string is identified, length of the string (1 or 2) otherwise
    """
    global __linestyle_to_qt_penstyle

    if len(stl) <= i:
        return 0

    if len(stl) > i+1:
        if '-' == stl[i+1] or '.' == stl[i+1]:
            # can check 2 chars
            wrong = 'inexistent'
            penstyle = __linestyle_to_qt_penstyle.get(stl[i:i+2], wrong)
            if wrong != penstyle:
                return 2

    if '-'==stl[i] or ':'==stl[i]:
        return 1
    else:
        return 0

def __apply_plot_args(graph, first_line, *args):
    """
        Applies args, like '-r' etc.
        @param graph :: a graph (or figure) that can contain multiple layers
        @param first_line :: first line to which the options will apply (useful when in hold mode / adding lines)
        @param args :: plot arguments

        Returns :: nothing, just uses kwargs to modify properties of the layer passed
    """
    if None==graph or len(args) < 1 or ((),) == args:
        return

    for a in args:
        if isinstance(a, basestring):
            # this will eat characters as they come, without minding much the past/previous characters
            # users can chain as many modifiers as they wish. It could be modified to be more strict/picky
            i = 0
            while i < len(a):
                linestyle_len = __is_linestyle(a,i)
                if linestyle_len > 0:
                    __apply_linestyle(graph, a[i:i+linestyle_len], first_line)
                    i += linestyle_len
                elif __is_marker(a[i]):
                    __apply_marker(graph, a[i:], first_line)
                    i += 1
                elif a[i].isalpha():
                    __apply_line_color(graph, a[i], first_line)
                    i += 1
                else:
                    # TOTHINK - error here? like this? sure? or just a warning?
                    raise ValueError("Unrecognized character in input string: " + str(a[i]))
        else:
            raise ValueError("Expecting style string, but got an unrecognized input parameter: " + str(a) + ", of type: " + str(type(a)))

def __apply_plot_kwargs(graph, first_line=0, **kwargs):
    """
        Applies kwargs
        @param graph :: a graph (or figure) that can contain multiple layers

        Returns :: nothing, just uses kwargs to modify properties of the layer passed
    """
    if None==graph or None==kwargs or ((),) == kwargs:
        return

    for key in kwargs:
        if 'linestyle' == key:
            __apply_linestyle(graph, kwargs[key])

        elif 'linewidth' == key:
            l = graph.activeLayer()
            for i in range(first_line, l.numCurves()):
                l.setCurveLineWidth(i, kwargs[key])

        elif 'color' == key:
            __apply_line_color(graph, kwargs[key], first_line)

        elif 'marker' == key:
            __apply_marker(graph, kwargs[key], first_line)

def __is_multiplot_command(*args, **kwargs):
    """
        Finds out if the passed *args make a valid multi-plot command. At the same time, splits the
        multi-plot command line into individual plot commands.

        @param args :: curve data and options.
        @param kwargs :: plot keyword options

        Returns :: tuple: (boolean: whether it is a multiplot command, list of single plot commands as tuples)
    """
    # A minimum multi-plot command would be plot(x, y, z, w) or plot(ws1, idx1, ws2, idx2)
    nargs = len(args)
    # this will be a list with the sequence of individual plots (a tuples, each describing a single plot)
    plots_seq = []
    if nargs < 4:
        return (False, [])
    i = 0
    while i < nargs:
        a = []
        b = []
        style = ''
        if (nargs-i) >= 3:
            if __is_data_pair(args[i], args[i+1]):
                a = args[i]
                b = args[i+1]
                i += 2
            else:
                return (False, []);
            # can have style string, but don't get confused with single workspace name strings!
            if (not __is_registered_workspace_name(args[i])) and isinstance(args[i], basestring):
                style = args[i]
                i += 1
            plots_seq.append((a,b,style))

        elif (nargs-i) >= 2:
            if __is_data_pair(args[i], args[i+1]):
                a = args[i]
                b = args[i+1]
                i += 2
            else:
                return (False, [])
            plots_seq.append((a, b, ''))

        elif (nargs-i) > 0:
            raise ValueError("Not plottable. I do not know what to do with this last parameter: " + args[i] + ", of type " + str(type(args)))

    return (i == nargs, plots_seq)

def __process_multiplot_command(plots_seq, **kwargs):
    """
        Make one plot at a time when given a multi-plot command.

        @param plots_seq :: list of individual plot parameters
        @param kwargs :: plot style options

        Returns :: the list of curves included in the plot
    """
    lines = []
    if len(plots_seq) >= 1:
        if not 'hold' in kwargs:
            kwargs['hold'] = 'off'
        lines = plot(*(plots_seq[0]), **kwargs)
    for i in range(1, len(plots_seq)):
        kwargs['hold'] = 'on'
        lines.extend(plot(*(plots_seq[i]), **kwargs))
    return lines

def __translate_hold_kwarg(**kwargs):
    """
    Helper function to translate from hold='on'/'off' kwarg to a True/False value for the 
    mantidplot window and window error_bars

    @param kwargs :: keyword arguments passed to a plot function, this function only cares about hold. Any
                     value different from 'on' will be considered as 'off'

    Returns :: tuple with a couple of values: True/False value for window, and True/False for clearWindow, 
               to be used with plotSpectrum, plotBin, etc.
    """
    # window and clearWindow
    window_val = None
    clearWindow_val = False
    hold_name = 'hold'
    missing_off = -1
    str_val = kwargs.get(hold_name, missing_off)
    if str_val != missing_off and str_val == 'on':
        if None == __last_shown_fig:
            window_val = None
        else:
            window_val = __last_fig()._graph
        clearWindow_val = False

    return window_val, clearWindow_val

def __translate_error_bars_kwarg(**kwargs):
    """
    Helper function to translate from error_bars=True/False kwarg to a True/False value for the 
    mantidplot error_bars argument

    @param kwargs :: keyword arguments passed to a plot function. This function only cares about 'error_bars'.
                     Any value different from 'True' will be considered as 'False'

    Returns :: True/False value for error_bars, to be used with plotSpectrum, plotBin, etc.

    """
    # error_bars param
    bars_val = False
    bars_name = 'error_bars'
    missing_off = -1
    str_val = kwargs.get(bars_name, missing_off)
    if str_val != missing_off and str_val == 'True':
        bars_val = True

    return bars_val

def __plot_as_workspace(*args, **kwargs):
    """
        plot spectrum via qti plotting framework to plot a workspace.

        @param args :: curve data and options.
        @param kwargs :: plot line options

        Returns :: List of line objects
    """
    return plot_spectrum(*args, **kwargs)

def __plot_as_workspaces_list(*args, **kwargs):
    """
        Plot a series of workspaces
        @param args :: curve data and options.
        @param kwargs :: plot line options
        
        Returns :: List of line objects
    """
    # mantidplot.plotSpectrum can already handle 1 or more input workspaces.
    return __plot_as_workspace(*args, **kwargs)


def __plot_as_array(*args, **kwargs):
    """
        Plot from an array
        @param args :: curve data and options.
        @param kwargs :: plot line options
        
        Returns :: the list of curves (1) included in the plot
    """
    y = args[0]
    idx_style = len(args)   # have to guess if we get plot(x,'r'), or plot(x, y, 'r') or no style string
    if len(args) > 1:
        if __is_array(args[1]):
            ws = __create_workspace(y, args[1])
            idx_style = 2
        elif isinstance(args[1], basestring):
            x = range(0, len(y), 1) # 0 to n, incremented by 1.
            ws = __create_workspace(x, y)
            # have to assume that args[1] is a style string
            idx_style = 1
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1)
        ws = __create_workspace(x, y)

    lines = __plot_as_workspace(ws, [0], *args[idx_style:], **kwargs)
    graph = None
    if len(lines) > 0:
        graph = lines[0]._graph
    else:
        raise Exception("Could not plot a workspace: " + ws)
    # something to improve: if the C++ Graph class provided a plot1D that doesn't do show(), so that
    # we could modify properties behind the scene and at the end do the show(). Con: do we really need
    # to load the qti layer with more methods because of outer layers like here?
    if 0 == len(kwargs):
        __matplotlib_defaults(graph.activeLayer())
    return __list_of_lines_from_graph(graph)

def __plot_with_tool(tool, *args, **kwargs):
    bin_tool_names = ['plot_bin', 'bin']
    spectrum_tool_names = ['plot_spectrum', 'plot_sp', 'spectrum', 'sp']
    md_tool_names = ['plot_md', 'md']

    if len(args) < 2:
        if tool in bin_tool_names:
            raise ValueError("To plot bins (using '%s' as tool) you need to give at least two parameters, where the second parameter selects the bins"%tool)
        elif tool in spectrum_tool_names:
            raise ValueError("To plot spectra (using '%s' as tool) you need to give at least two parameters, where the second parameter selects the spectrum(a)"%tool)

    if tool in bin_tool_names:
        return plot_bin(args[0], args[1], *args[2:], **kwargs)
    elif tool in md_tool_names:
        return plot_md(args[0], *args[1:], **kwargs)
    elif tool in spectrum_tool_names:
        return plot_spectrum(args[0], args[1], *args[2:], **kwargs)
    # here you would add slice/spectrum/instrument viewer, etc. and maybe you'll want to put them in a dict
    else:
        raise ValueError("Unrecognized tool specified: '" + tool + ";. Cannot plot this. ")

def __plot_with_best_guess(*args, **kwargs):
    y = args[0]
    if __is_array(y):
        if __is_array_of_workspaces(y):
            return __plot_as_workspaces_list(*args, **kwargs)
        else:
            return __plot_as_array(*args, **kwargs)
    else:
        # mantidplot.plotSpectrum can handle workspace names (strings)
        return __plot_as_workspace(*args, **kwargs)

def plot_bin(workspaces, indices, *args, **kwargs):
    """
    X-Y plot of the bin counts in a workspace.

    Plots one or more bin, selected by indices, using spectra numbers as x-axis and bin counts for 
    each spectrum as y-axis.

    @param workspaces :: workspace or list of workspaces (both workspace objects and names accepted)
    @param indices :: indices of the bin(s) to plot

    Returns :: the list of curves included in the plot
    """
    # Find optional params to plotBin
    bars_val = __translate_error_bars_kwarg(**kwargs)
    window_val, clearWindow_val = __translate_hold_kwarg(**kwargs)

    # to change properties on the new lines being added
    first_line = 0
    if None != window_val:
        first_line = window_val.activeLayer().numCurves()

    graph = mantidplot.plotBin(workspaces, indices, error_bars=bars_val, type=-1, window=window_val, clearWindow=clearWindow_val)

    __apply_plot_args(graph, first_line, *args)
    __apply_plot_kwargs(graph, first_line, **kwargs)

    return __list_of_lines_from_graph(graph, first_line)


def plot_md(workspaces, *args, **kwargs):
    """
    X-Y plot of an MDWorkspace.

    @param workspaces :: workspace or list of workspaces (both workspace objects and names accepted)

    Returns :: the list of curves included in the plot
    """
    # Find optional params to plotBin
    bars_val = __translate_error_bars_kwarg(**kwargs)
    window_val, clearWindow_val = __translate_hold_kwarg(**kwargs)

    # to change properties on the new lines being added
    first_line = 0
    if None != window_val:
        first_line = window_val.activeLayer().numCurves()

    graph = mantidplot.plotMD(workspaces, normalization=mantidplot.DEFAULT_MD_NORMALIZATION, error_bars=bars_val, window=window_val, clearWindow=clearWindow_val)

    __apply_plot_args(graph, first_line, *args)
    __apply_plot_kwargs(graph, first_line, **kwargs)

    return __list_of_lines_from_graph(graph, first_line)


def plot_spectrum(workspaces, indices, *args, **kwargs):
    """X-Y Plot of spectra in a workspace.

    Plots one or more spectra, selected by indices, using bin boundaries as x-axis
    and the spectra values in each bin as y-axis.

    @param workspaces :: workspace or list of workspaces (both workspace objects and names accepted)
    @param indices :: indices of the spectra to plot, given as a single integer or a list of integers

    Returns :: the list of curves included in the plot

    """
    # Find optional params to plotSpectrum
    bars_val = __translate_error_bars_kwarg(**kwargs)
    window_val, clearWindow_val = __translate_hold_kwarg(**kwargs)

    # to change properties on the new lines being added
    first_line = 0
    if None != window_val:
        first_line = window_val.activeLayer().numCurves()

    graph = mantidplot.plotSpectrum(workspaces, indices, error_bars=bars_val, type=-1, window=window_val, clearWindow=clearWindow_val)

    __apply_plot_args(graph, first_line, *args)
    __apply_plot_kwargs(graph, first_line, **kwargs)

    return __list_of_lines_from_graph(graph, first_line)


def plot(*args, **kwargs):
    """
    Plot the data in various forms depending on what arguments are passed.  Currently supported
    inputs: arrays (as Python lists or numpy arrays) and workspaces (by name or workspace objects).

    @param args :: curve data and options
    @param kwargs :: plot line options

    Returns :: the list of curves included in the plot

    args can take different forms depending on what you plot. You can plot:

    * a python list or array (x) for example like this: plot(x)

    * a workspace (ws) for example like this: plot(ws, [100,101])  # this will plot spectra 100 and 101

    * a list of workspaces (ws, ws2, ws3, etc.) for example like this: plot([ws, ws2, ws3], [100,101])

    * workspaces identified by their names: plot(['HRP39182', 'MAR11060.nxs'], [100,101])

    You can also pass matplotlib/pyplot style strings as arguments, for example: plot(x, '-.')

    As keyword arguments (kwargs) you can specify multiple
    parameters, for example: linewidth, linestyle, marker, color.

    An important keyword argument is tool. At the moment the
    following values are supported (they have long and short aliases):

    * To plot spectra: 'plot_spectrum' OR 'spectrum' OR 'plot_sp' OR 'sp'  (default for workspaces).
    * To plot bins: 'plot_bin' OR 'bin'
    * To do an MD plot: 'plot_md' OR 'md'

    Please see the documentation of this module (use help()) for more details.

    """
    nargs = len(args)
    if nargs < 1:
        raise ValueError("You did not pass any argument. You must provide data to plot.")

    # TOTHINK: should there be an exception if it's plot_md (tool='plot_md')
    (is_it, plots_seq) = __is_multiplot_command(*args, **kwargs)
    if is_it:
        return __process_multiplot_command(plots_seq, **kwargs)
    elif len(args) > 3:
        raise ValueError("Could not interpret the arguments passed. You passed more than 3 positional arguments but this does not seem to be a correct multi-plot command. Please check your command and make sure that the workspaces given are correct.")

    # normally guess; exception if e.g. a parameter tool='plot_bin' is given
    try:
        tool_val = kwargs['tool']
        del kwargs['tool']
        return __plot_with_tool(tool_val, *args, **kwargs)
    except KeyError:
        return __plot_with_best_guess(*args, **kwargs)


#=============================================================================
# Functions, for pyplot / old matlab style manipulation of figures
#=============================================================================

def xlim(xmin, xmax):
    """
    Set the boundaries of the x axis

    @param xmin :: minimum value
    @param xmax :: maximum value
    """
    l = __last_fig()._graph.activeLayer()
    l.setAxisScale(2, xmin, xmax)

def ylim(ymin, ymax):
    """
    Set the boundaries of the y axis

    @param ymin :: minimum value
    @param ymax :: maximum value
    """
    l = __last_fig()._graph.activeLayer()
    l.setAxisScale(0, ymin, ymax)

def xlabel(lbl):
    """
    Set the label or title of the x axis

    @param lbl :: x axis lbl
    """
    l = __last_fig()._graph.activeLayer()
    l.setXTitle(lbl)

def ylabel(lbl):
    """
    Set the label or title of the y axis

    @param lbl :: y axis lbl
    """
    l = __last_fig()._graph.activeLayer()
    l.setYTitle(lbl)

def title(title):
    """
    Set title of the active plot

    @param title :: title string
    """
    l = __last_fig()._graph.activeLayer()
    l.setTitle(title)

def axis(lims):
    """
    Set the boundaries or limits of the x and y axes

    @param lims :: list or vector specifying min x, max x, min y, max y
    """
    l = __last_fig()._graph.activeLayer()
    if 4 != len(lims):
        raise ValueError("Error: 4 real values are required for the x and y axes limits")
    l.setScale(*lims)

def yscale(scale_str):
    """
    Set the type of scale of the y axis

    @param scale_str :: either 'linear' for linear scale or 'log' for logarithmic scale
    """
    ax = __last_fig()._axes
    ax.set_yscale(scale_str)

def xscale(scale_str):
    """
    Set the type of scale of the x axis

    @param scale_str :: either 'linear' for linear scale or 'log' for logarithmic scale
    """
    ax = __last_fig()._axes
    ax.set_xscale(scale_str)

def grid(opt='on'):
    """
    Enable a grid on the active plot (horizontal and vertical)

    @param title :: 'on' to enable
    """
    l = __last_fig()._graph.activeLayer()
    if None == opt or 'on' == opt:
        l.showGrid()
    elif 'off' == opt:
        # TODO is there support for a 'hideGrid' in qti? Apparently not.
        print "Sorry, hiding/disabling grids is currenlty not supported"

def figure(num=None):
    """
    Return Figure object for a new figure or an existing one (if there is any
    with the number passed as parameter).

    @param num :: figure number (optional). If empty, a new figure is created.
    """
    if not num:
        return __empty_fig()
    else:
        if num < 0:
            raise ValueError("The figure number must be >= 0")

    return Figure(num)

def savefig(name):
    """
    Save current plot into a file. The format is guessed from the file extension (.eps, .png, .jpg, etc.)

    @param name :: file name
    """
    if not name:
        raise ValueError("Error: you need to specify a non-empty file name")
    l = __last_fig()._graph.activeLayer()
    l.saveImage(name);
