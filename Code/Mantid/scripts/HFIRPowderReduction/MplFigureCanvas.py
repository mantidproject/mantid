#pylint: disable=invalid-name
from PyQt4 import QtGui

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

MplLineStyles = ['-' , '--' , '-.' , ':' , 'None' , ' ' , '']
MplLineMarkers = [ ". (point)",
        ", (pixel         )",
        "o (circle        )",
        "v (triangle_down )",
        "^ (triangle_up   )",
        "< (triangle_left )",
        "> (triangle_right)",
        "1 (tri_down      )",
        "2 (tri_up        )",
        "3 (tri_left      )",
        "4 (tri_right     )",
        "8 (octagon       )",
        "s (square        )",
        "p (pentagon      )",
        "* (star          )",
        "h (hexagon1      )",
        "H (hexagon2      )",
        "+ (plus          )",
        "x (x             )",
        "D (diamond       )",
        "d (thin_diamond  )",
        "| (vline         )",
        "_ (hline         )",
        "None (nothing    )"]

MplBasicColors = [
        "blue",
        "green",
        "red",
        "cyan",
        "magenta",
        "yellow",
        "black",
        "white"]

class Qt4MplCanvas(FigureCanvas):
    """  A customized Qt widget for matplotlib figure.
    It can be used to replace GraphicsView of QtGui
    """
    def __init__(self, parent):
        """  Initialization
        """
        # Instantialize matplotlib Figure
        self.fig = Figure()
        self.axes = self.fig.add_subplot(111) # return: matplotlib.axes.AxesSubplot

        # Initialize parent class and set parent
        FigureCanvas.__init__(self, self.fig)
        self.setParent(parent)

        # Set size policy to be able to expanding and resizable with frame
        FigureCanvas.setSizePolicy(self, QtGui.QSizePolicy.Expanding,\
                QtGui.QSizePolicy.Expanding)

        FigureCanvas.updateGeometry(self)

        # Variables to manage all lines/subplot
        self._lineDict = {}
        self._lineIndex = 0

        return

    def addPlot(self, x, y):
        """ Plot a set of data
        Argument:
        - x: numpy array X
        - y: numpy array Y
        """
        self.x = x
        self.y = y

        # color must be RGBA (4-tuple)
        r = self.axes.plot(x, y, color=(0,1,0,1), marker='o', linestyle='--',
                label='X???X', linewidth=2) # return: list of matplotlib.lines.Line2D object

        # set label
        self.axes.set_xlabel(r"$2\theta$", fontsize=20)  

        # set/update legend
        self.axes.legend()

        # Register
        if len(r) == 1: 
            self._lineDict[self._lineIndex] = r[0]
        else:
            print "Impoooooooooooooooosible!"
        self._lineIndex += 1

        return

    def getLastPlotIndexKey(self):
        """ Get the index/key of the last added line
        """
        return self._lineIndex-1


    def getPlot(self):
        """ reture figure's axes to expose the matplotlib figure to PyQt client
        """
        return self.axes


    def removePlot(self, ikey):
        """ Remove the line with its index as key
        """
        # self._lineDict[ikey].remove()
        lines = self.axes.lines
        print str(type(lines)), lines
        print "ikey = ", ikey, self._lineDict[ikey]
        self.axes.lines.remove(self._lineDict[ikey])
        #self.axes.remove(self._lineDict[ikey])
        print self._lineDict[ikey]
        self._lineDict[ikey] = None

        return

    def updateLine(self, ikey, vecx, vecy, linestyle=None, linecolor=None, marker=None, markercolor=None):
        """
        """
        line = self._lineDict[ikey]

        if vecx is not None and vecy is not None:
            line.set_xdata(vecx)
            line.set_ydata(vecy)

        if linecolor is not None:
            line.set_color(linecolor)

        if linestyle is not None:
            line.set_linestyle(linestyle)

        if marker is not None:
            line.set_marker(marker)

        if markercolor is not None:
            line.set_markerfacecolor(markercolor)

        oldlabel = line.get_label()
        line.set_label(oldlabel)

        self.axes.legend()

        return

    def getLineStyleList(self):
        """
        """
        return MplLineStyles


    def getLineMarkerList(self):
        """
        """
        return MplLineMarkers

    def getLineBasicColorList(self):
        """
        """
        return MplBasicColors 
