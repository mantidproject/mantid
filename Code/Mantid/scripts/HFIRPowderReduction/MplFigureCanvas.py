#pylint: disable=invalid-name
from PyQt4 import QtGui

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

MplLineStyles = ['-' , '--' , '-.' , ':' , 'None' , ' ' , '']
MplLineMarkers = [
        "o (circle        )",
        "s (square        )",
        "D (diamond       )",
        ", (pixel         )",
        ". (point         )",
        "v (triangle_down )",
        "^ (triangle_up   )",
        "< (triangle_left )",
        "> (triangle_right)",
        "1 (tri_down      )",
        "2 (tri_up        )",
        "3 (tri_left      )",
        "4 (tri_right     )",
        "8 (octagon       )",
        "p (pentagon      )",
        "* (star          )",
        "h (hexagon1      )",
        "H (hexagon2      )",
        "+ (plus          )",
        "x (x             )",
        "d (thin_diamond  )",
        "| (vline         )",
        "_ (hline         )",
        "None (nothing    )"]

MplBasicColors = [
        "black",
        "red",
        "blue",
        "green",
        "cyan",
        "magenta",
        "yellow",
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

    def addPlot(self, x, y, color=None, label="", xlabel=None, ylabel=None, marker=None, linestyle=None, linewidth=1):
        """ Plot a set of data
        Argument:
        - x: numpy array X
        - y: numpy array Y
        """
        # process inputs and defaults
        self.x = x
        self.y = y
        
        if color is None:
            color = (0,1,0,1)
        if marker is None:
            marker = 'o'
        if linestyle is None:
            linestyle = '-'
            
        # color must be RGBA (4-tuple)
        r = self.axes.plot(x, y, color=color, marker=marker, linestyle=linestyle,
                label=label, linewidth=1) # return: list of matplotlib.lines.Line2D object

        # set x-axis and y-axis label
        if xlabel is not None:
            self.axes.set_xlabel(xlabel, fontsize=20)  
        if ylabel is not None:
            self.axes.set_ylabel(ylabel, fontsize=20)

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
        
    def getDefaultColorMarkerComboList(self):
        """ Get a list of line/marker color and marker style combination 
        as default to add more and more line to plot
        """
        combolist = []
        nummarkers = len(MplLineMarkers)
        numcolors = len(MplBasicColors)
        
        for i in xrange(nummarkers):
            marker = MplLineMarkers[i]
            for j in xrange(numcolors):
                color = MplBasicColors[j]
                combolist.append( (marker, color) )
            # ENDFOR (j)
        # ENDFOR(i)
        
        return combolist


class MyNavigationToolbar(NavigationToolbar):
    """ A customized navigation tool bar attached to canvas
    """
    def __init__(self, parent, canvas, direction='h'):
        """ Initialization
        """
        self.canvas = canvas
        QtGui.QWidget.__init__(self, parent)

        #if direction=='h' :
        #    self.layout = QtGui.QHBoxLayout(self)
        #else :
        #    self.layout = QtGui.QVBoxLayout(self)

        #self.layout.setMargin(2)
        #self.layout.setSpacing(0)

        NavigationToolbar.__init__( self, canvas, canvas )

        return
