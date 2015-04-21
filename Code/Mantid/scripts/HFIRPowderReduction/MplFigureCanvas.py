#pylint: disable=invalid-name
import sys
import os

from PyQt4 import QtGui, QtCore

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
        
        
class Qt4MplPlotView(QtGui.QWidget):
    """ A combined graphics view including matplotlib canvas and 
    a navigation tool bar
    """
    def __init__(self, parent):
        """ Initialization
        """
        # instantianize parent
        QtGui.QWidget.__init__(self, parent)
        
        # set up canvas
        self.canvas = Qt4MplCanvas(self)
        self.toolbar = MyNavigationToolbar(self.canvas, self.canvas)
        
        # set up layout
        self.vbox = QtGui.QVBoxLayout(self)
        self.vbox.addWidget(self.canvas)
        self.vbox.addWidget(self.toolbar)
        
        # auto line's maker+color list
        self._myLineMarkerColorList = []
        self._myLineMarkerColorIndex = 0
        self.setAutoLineMarkerColorCombo()

        return
        
    def addPlot(self, x, y, color=None, label="", xlabel=None, ylabel=None, marker=None, linestyle=None, linewidth=1):
        """ Add a new plot
        """
        self.canvas.addPlot(x, y, color, label, xlabel, ylabel, marker, linestyle, linewidth)
        
        return


    def addPlot2D(self, array2d, xmin, xmax, ymin, ymax, holdprev=True, yticklabels=None):
        """ Plot a 2D image
        Arguments
         - array2d :: numpy 2D array
        """
        self.canvas.addPlot2D(array2d, xmin, xmax, ymin, ymax, holdprev, yticklabels)

        return


    def addImage(self, imagefilename):
        """ Add an image by file
        """
        # check
        if os.path.exists(imagefilename) is False:
            raise NotImplementedError("Image file %s does not exist." % (imagefilename))

        self.canvas.addImage(imagefilename)

        return


    def clearAllLines(self):
        """
        """
        self.canvas.clearAllLines()

    def clearCanvas(self):
        """ Clear canvas
        """
        return self.canvas.clearCanvas()
        
    def draw(self):
        """ Draw to commit the change
        """
        return self.canvas.draw()

    def getPlot(self):
        """
        """
        return self.canvas.getPlot()
        
    def getLastPlotIndexKey(self):
        """ Get ...
        """
        return self.canvas.getLastPlotIndexKey()
    
    def getXLimit(self):
        """ Get limit of Y-axis
        """
        return self.canvas.getXLimit()
        
    def getYLimit(self):
        """ Get limit of Y-axis
        """
        return self.canvas.getYLimit()
        
    def removePlot(self, ikey):
        """
        """
        return self.canvas.removePlot(ikey)

    def setXYLimits(self, xmin=None, xmax=None, ymin=None, ymax=None):
        """ 
        """
        return self.canvas.setXYLimit(xmin, xmax, ymin, ymax)

        
    def updateLine(self, ikey, vecx, vecy, linestyle=None, linecolor=None, marker=None, markercolor=None):
        """
        """
        return self.canvas.updateLine(ikey, vecx, vecy, linestyle, linecolor, marker, markercolor)


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
        return self.canvas.getDefaultColorMarkerComboList()

    def getNextLineMarkerColorCombo(self):
        """ As auto line's marker and color combo list is used,
        get the NEXT marker/color combo
        """
        # get from list
        marker, color = self._myLineMarkerColorList[self._myLineMarkerColorIndex]
        # process marker if it has information
        if marker.count(' (') > 0:
            marker = marker.split(' (')[0]
        print "[DB] Print line %d: marker = %s, color = %s" % (self._myLineMarkerColorIndex, marker, color)
        
        # update the index
        self._myLineMarkerColorIndex += 1
        if self._myLineMarkerColorIndex == len(self._myLineMarkerColorList):
            self._myLineMarkerColorIndex = 0
            
        return (marker, color)
        
    def setXYLimit(self, xmin, xmax, ymin, ymax):
        """ Set X-Y limit automatically
        """
        self.canvas.axes.set_xlim([xmin, xmax])
        self.canvas.axes.set_ylim([ymin, ymax])
        
        self.canvas.draw()
        
        return
        
    def setAutoLineMarkerColorCombo(self):
        """
        """
        self._myLineMarkerColorList = []
        for marker in MplLineMarkers:
            for color in MplBasicColors:
                self._myLineMarkerColorList.append( (marker, color) )
                
        return

    def setLineMarkerColorIndex(self, newindex):
        """
        """
        self._myLineMarkerColorIndex = newindex
        
        return

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

        self.colorbar = None

        return

    def addPlot(self, x, y, color=None, label="", xlabel=None, ylabel=None, marker=None, linestyle=None, linewidth=1):
        """ Plot a set of data
        Argument:
        - x: numpy array X
        - y: numpy array Y
        """
        # Test... FIXME 
        self.axes.hold(True)

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

        self.axes.set_aspect('auto')

        # set x-axis and y-axis label
        if xlabel is not None:
            self.axes.set_xlabel(xlabel, fontsize=20)  
        if ylabel is not None:
            self.axes.set_ylabel(ylabel, fontsize=20)

        # set/update legend
        self._setupLegend()

        # Register
        if len(r) == 1: 
            self._lineDict[self._lineIndex] = r[0]
        else:
            print "Impoooooooooooooooosible!"
        self._lineIndex += 1

        # Flush/commit
        self.draw()

        return


    def addPlot2D(self, array2d, xmin, xmax, ymin, ymax, holdprev, yticklabels=None):
        """ Add a 2D plot

        Arguments:
         - yticklabels :: list of string for y ticks
        """
        # Release the current image
        self.axes.hold(holdprev)

        # Do plot
        # y ticks will be shown on line 1, 4, 23, 24 and 30 
        # yticks = [1, 4, 23, 24, 30]
        # self.axes.set_yticks(yticks)

        # show image
        imgplot = self.axes.imshow(array2d, extent=[xmin,xmax,ymin,ymax], interpolation='none')
        # set y ticks as an option: 
        if yticklabels is not None: 
            # it will always label the first N ticks even image is zoomed in
            self.axes.set_yticklabels(yticklabels)
        # explicitly set aspect ratio of the image
        self.axes.set_aspect('auto')

        # Set color bar.  plt.colorbar() does not work!
        if self.colorbar is None:
            # set color map type
            imgplot.set_cmap('spectral')
            self.colorbar = self.fig.colorbar(imgplot)
        else:
            self.colorbar.update_bruteforce(imgplot)

        # Flush...
        self._flush() 

        return

    def addImage(self, imagefilename):
        """ Add an image by file
        """
        import matplotlib.image as mpimg 

        # set aspect to auto mode
        self.axes.set_aspect('auto')

        img = mpimg.imread(str(imagefilename))
        lum_img = img[:,:,0] 
        # TODO : refactor for image size, interpolation and origin
        imgplot = self.axes.imshow(img, extent=[0, 1000, 800, 0], interpolation='none', origin='lower')

        # Set color bar.  plt.colorbar() does not work!
        if self.colorbar is None:
            # set color map type
            imgplot.set_cmap('spectral')
            self.colorbar = self.fig.colorbar(imgplot)
        else:
            self.colorbar.update_bruteforce(imgplot)

        self._flush()

        return

        
    def clearAllLines(self):
        """ Remove all lines from the canvas
        """
        for ikey in self._lineDict.keys():
            plot = self._lineDict[ikey]
            if plot is not None:
                self.axes.lines.remove(plot)
                self._lineDict[ikey] = None
            # ENDIF(plot)
        # ENDFOR
        
        self.draw()

        return


    def clearCanvas(self):
        """ Clear data from canvas
        """
        # clear the image for next operation
        self.axes.hold(False)

        # clear image
        self.axes.cla()

        # flush/commit
        self._flush()

        return


    def getLastPlotIndexKey(self):
        """ Get the index/key of the last added line
        """
        return self._lineIndex-1


    def getPlot(self):
        """ reture figure's axes to expose the matplotlib figure to PyQt client
        """
        return self.axes

    def getXLimit(self):
        """ Get limit of Y-axis
        """
        return self.axes.get_xlim()

    def getYLimit(self):
        """ Get limit of Y-axis
        """
        return self.axes.get_ylim()

    def setXYLimit(self, xmin, xmax, ymin, ymax):
        """
        """
        # for X
        xlims = self.axes.get_xlim() 
        xlims = list(xlims)
        if xmin is not None:
            xlims[0] = xmin
        if xmax is not None:
            xlims[1] = xmax
        self.axes.set_xlim(xlims)

        # for Y 
        ylims = self.axes.get_ylim() 
        ylims = list(ylims)
        if ymin is not None:
            ylims[0] = ymin
        if ymax is not None:
            ylims[1] = ymax
        self.axes.set_ylim(ylims)

        # try draw
        self.draw()

        return

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

        # commit
        self.draw()

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


    def _flush(self):
        """ A dirty hack to flush the image
        """
        w, h = self.get_width_height()
        self.resize(w+1,h)
        self.resize(w,h)

        return


    def _setupLegend(self):
        """ Set up legend
        self.axes.legend()
        Handler is a Line2D object. Lable maps to the line object
        """
        loclist = [
            "best",
            "upper right",
            "upper left",
            "lower left",
            "lower right",
            "right",
            "center left",
            "center right",
            "lower center",
            "upper center", 
            "center"] 

        handles, labels = self.axes.get_legend_handles_labels()
        self.axes.legend(handles, labels, loc='best')    
        print handles
        print labels
        #self.axes.legend(self._myLegendHandlers, self._myLegentLabels)

        return



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
