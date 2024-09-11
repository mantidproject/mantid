# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments,non-parent-init-called,R0901,R0902,too-many-branches,C0302
import os
import numpy as np
from qtpy.QtWidgets import QWidget, QVBoxLayout, QSizePolicy
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from mantidqt.MPLwidgets import NavigationToolbar2QT as NavigationToolbar2
from matplotlib.figure import Figure
import matplotlib.image
from matplotlib import pyplot as plt


class Mpl2dGraphicsView(QWidget):
    """A combined graphics view including matplotlib canvas and
    a navigation tool bar for 2D image specifically
    """

    def __init__(self, parent):
        """Initialization"""
        # Initialize parent
        QWidget.__init__(self, parent)

        # set up canvas
        self._myCanvas = Qt4Mpl2dCanvas(self)
        self._myToolBar = MyNavigationToolbar(self, self._myCanvas)

        # set up layout
        self._vBox = QVBoxLayout(self)
        self._vBox.addWidget(self._myCanvas)
        self._vBox.addWidget(self._myToolBar)

        # auto line's maker+color list
        self._myImageIndex = 0
        self._myImageDict = dict()

        # current 2D plot
        self._2dPlot = None

        return

    @property
    def array2d(self):
        """
        return the matrix (2d-array) plot on the canvas
        :return:
        """
        return self._myCanvas.array2d

    def add_plot_2d(self, array2d, x_min, x_max, y_min, y_max, hold_prev_image=True, y_tick_label=None):
        """
        Add a 2D image to canvas
        :param array2d: numpy 2D array
        :param x_min:
        :param x_max:
        :param y_min:
        :param y_max:
        :param hold_prev_image:
        :param y_tick_label:
        :return:
        """
        self._2dPlot = self._myCanvas.add_2d_plot(array2d, x_min, x_max, y_min, y_max, hold_prev_image, y_tick_label)

        return self._2dPlot

    def add_image(self, imagefilename):
        """Add an image to canvas from an image file"""
        # check
        if os.path.exists(imagefilename) is False:
            raise NotImplementedError("Image file %s does not exist." % (imagefilename))

        self._myCanvas.addImage(imagefilename)

        return

    @property
    def canvas(self):
        """Get the canvas
        :return:
        """
        return self._myCanvas

    def clear_canvas(self):
        """Clear canvas"""
        return self._myCanvas.clear_canvas()

    def draw(self):
        """Draw to commit the change"""
        return self._myCanvas.draw()

    @staticmethod
    def evt_view_updated():
        """Event handling as canvas size updated
        :return:
        """
        # There is no operation that is defined now
        return

    def remove_last_plot(self):
        """

        :return:
        """
        if self._2dPlot is not None:
            self._2dPlot.remove()
            self._2dPlot = None

        return

    def save_figure(self, file_name):
        """
        save the current on-canvas figure to a file
        :param file_name:
        :return:
        """
        self._myCanvas.fig.savefig(file_name)

        return

    def set_title(self, title):
        """
        set title to image
        :param title:
        :return:
        """
        self._myCanvas.axes.set_title(title)

    @property
    def x_min(self):
        """
        minimum x of the canvas
        :return:
        """
        return self._myCanvas.x_min

    @property
    def x_max(self):
        """
        maximum x of the canvas
        :return:
        """
        return self._myCanvas.x_max

    @property
    def y_min(self):
        """minimum y of the canvas
        :return:
        """
        return self._myCanvas.y_min

    @property
    def y_max(self):
        """maximum y of the canvas
        :return:
        """
        return self._myCanvas.y_max


class Qt4Mpl2dCanvas(FigureCanvas):
    """A customized Qt widget for matplotlib 2D image.
    It can be used to replace GraphicsView
    """

    def __init__(self, parent):
        """Initialization"""
        # Instantiating matplotlib Figure
        self.fig = Figure()
        self.fig.patch.set_facecolor("white")

        self.axes = self.fig.add_subplot(111)  # return: matplotlib.axes.AxesSubplot

        # Initialize parent class and set parent
        FigureCanvas.__init__(self, self.fig)
        self.setParent(parent)

        # Set size policy to be able to expanding and resizable with frame
        FigureCanvas.setSizePolicy(self, QSizePolicy.Expanding, QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)

        # legend and color bar
        self._colorBar = None

        # Buffer of data
        self._currentArray2D = None

        # image management data structure
        self._currIndex = 0
        self._imagePlotDict = dict()

        # image size
        self._xLimit = [0.0, 1.0]
        self._yLimit = [0.0, 1.0]

        return

    @property
    def array2d(self):
        """
        get the matrix plot now
        :return:
        """
        return self._currentArray2D

    def add_2d_plot(self, array2d, x_min, x_max, y_min, y_max, hold_prev, yticklabels=None):
        """Add a 2D plot
        Requirements:
        (1) a valid 2-dimensional numpy.ndarray
        (2) x_min, x_max, y_min, y_max are of right order
        Guarantees: a 2D fill-plot is made
        :param array2d:
        :param x_min:
        :param x_max:
        :param y_min:
        :param y_max:
        :param hold_prev: hold previous image.  If False, all 2D image and polygon patches will be removed
        :param yticklabels: list of string for y ticks
        :return:
        """
        # Check
        assert isinstance(array2d, np.ndarray), "Input array2d must be a numpy array but not %s." % str(type(array2d))
        assert (
            isinstance(x_min, int) and isinstance(x_max, int) and x_min < x_max
        ), f"x_min = {x_min} (of type {type(x_min)}) should be less than x_max = {x_max} (of type {type(x_max)})."
        assert isinstance(y_min, int) and isinstance(y_max, int) and y_min < y_max

        # Release the current image
        self.axes.hold(hold_prev)

        # show image
        img_plot = self.axes.imshow(array2d, extent=[x_min, x_max, y_min, y_max], interpolation="none")
        self._currentArray2D = array2d

        # set y ticks as an option:
        if yticklabels is not None:
            # it will always label the first N ticks even image is zoomed in
            # FUTURE-VZ : The way to set up the Y-axis ticks is wrong!"
            # self.axes.set_yticklabels(yticklabels)
            print("[Warning] The method to set up the Y-axis ticks to 2D image is wrong!")

        # explicitly set aspect ratio of the image
        self.axes.set_aspect("auto")

        # Set color bar.  plt.colorbar() does not work!
        if self._colorBar is None:
            # set color map type
            img_plot.set_cmap("spectral")
            self._colorBar = self.fig.colorbar(img_plot)
        else:
            self._colorBar.update_bruteforce(img_plot)

        # Flush...
        self._flush()

        # Add the image management
        self._currIndex += 1
        self._imagePlotDict[self._currIndex] = img_plot

        return self._currIndex

    def add_patch(self, patch):
        """
        add an artist patch such as polygon
        :param patch:
        :return:
        """
        self.axes.add_artist(patch)

        # Flush...
        self._flush()

        return

    def addImage(self, imagefilename):
        """Add an image by file"""
        # set aspect to auto mode
        self.axes.set_aspect("auto")

        img = matplotlib.image.imread(str(imagefilename))
        # lum_img = img[:,:,0]
        # FUTURE : refactor for image size, interpolation and origin
        imgplot = self.axes.imshow(img, extent=[0, 1000, 800, 0], interpolation="none", origin="lower")

        # Set color bar.  plt.colorbar() does not work!
        if self._colorBar is None:
            # set color map type
            imgplot.set_cmap("spectral")
            self._colorBar = self.fig.colorbar(imgplot)
        else:
            self._colorBar.update_bruteforce(imgplot)

        self._flush()

        return

    def clear_canvas(self):
        """Clear data including lines and image from canvas"""
        # clear the image for next operation
        self.axes.hold(False)

        # clear 2D image
        self.axes.cla()
        # Try to clear the color bar
        if len(self.fig.axes) > 1:
            self.fig.delaxes(self.fig.axes[1])
            self._colorBar = None
            # This clears the space claimed by color bar but destroys sub_plot too.
            self.fig.clear()
            # Re-create subplot
            self.axes = self.fig.add_subplot(111)
        # END-FOR

        # flush/commit
        self._flush()

        return

    def plot_polygon(self, vertex_array, fill=False, color="w"):
        """
        Plot a new polygon
        :param vertex_array:
        :param fill:
        :param color:
        :return:
        """
        # check requirements
        assert isinstance(vertex_array, np.ndarray)
        assert isinstance(fill, bool)
        assert isinstance(color, str)

        # plot polygon
        p = plt.Polygon(vertex_array, fill=fill, color=color)
        self.axes.add_artist(p)

        # Flush...
        self._flush()

        return p

    @property
    def x_min(self):
        """x minimum
        :return:
        """
        return self._xLimit[0]

    @property
    def x_max(self):
        """maximum x
        :return:
        """
        return self._xLimit[1]

    @property
    def y_min(self):
        """minimum y
        :return:
        """
        return self._yLimit[0]

    @property
    def y_max(self):
        """maximum y
        :return:
        """
        return self._yLimit[1]

    def _flush(self):
        """A dirty hack to flush the image"""
        w, h = self.get_width_height()
        self.resize(w + 1, h)
        self.resize(w, h)

        return


# END-OF-CLASS (MplGraphicsView)


class MyNavigationToolbar(NavigationToolbar2):
    """A customized navigation tool bar attached to canvas
    Note:
    * home, left, right: will not disable zoom/pan mode
    * zoom and pan: will turn on/off both's mode

    Other methods
    * drag_pan(self, event): event handling method for dragging canvas in pan-mode
    """

    NAVIGATION_MODE_NONE = 0
    NAVIGATION_MODE_PAN = 1
    NAVIGATION_MODE_ZOOM = 2

    def __init__(self, parent, canvas):
        """Initialization"""
        NavigationToolbar2.__init__(self, canvas, canvas)

        self._myParent = parent
        self._navigationMode = MyNavigationToolbar.NAVIGATION_MODE_NONE

        return

    def get_mode(self):
        """
        :return: integer as none/pan/zoom mode
        """
        return self._navigationMode

    # Overriding base's methods
    def draw(self):
        """
        Canvas is drawn called by pan(), zoom()
        :return:
        """
        NavigationToolbar2.draw(self)

        self._myParent.evt_view_updated()

        return

    def pan(self, *args):
        """

        :param args:
        :return:
        """
        NavigationToolbar2.pan(self, args)

        if self._navigationMode == MyNavigationToolbar.NAVIGATION_MODE_PAN:
            # out of pan mode
            self._navigationMode = MyNavigationToolbar.NAVIGATION_MODE_NONE
        else:
            # into pan mode
            self._navigationMode = MyNavigationToolbar.NAVIGATION_MODE_PAN

        return

    def zoom(self, *args):
        """
        Turn on/off zoom (zoom button)
        :param args:
        :return:
        """
        NavigationToolbar2.zoom(self, args)

        if self._navigationMode == MyNavigationToolbar.NAVIGATION_MODE_ZOOM:
            # out of zoom mode
            self._navigationMode = MyNavigationToolbar.NAVIGATION_MODE_NONE
        else:
            # into zoom mode
            self._navigationMode = MyNavigationToolbar.NAVIGATION_MODE_ZOOM

        return

    def _update_view(self):
        """
        view update called by home(), back() and forward()
        :return:
        """
        NavigationToolbar2._update_view(self)

        self._myParent.evt_view_updated()

        return
