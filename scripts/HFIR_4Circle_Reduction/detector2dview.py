import mplgraphicsview

import numpy as np
from matplotlib import pyplot as plt


class Detector2DView(mplgraphicsview.MplGraphicsView):
    """
    Customized 2D detector view
    """
    class MousePress(object):
        RELEASED = 0
        LEFT = 1
        RIGHT = 3

    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        mplgraphicsview.MplGraphicsView.__init__(self, parent)

        # connect the mouse motion to interact with the canvas
        self._myCanvas.mpl_connect('button_press_event', self.on_mouse_press_event)
        self._myCanvas.mpl_connect('button_release_event', self.on_mouse_release_event)
        self._myCanvas.mpl_connect('motion_notify_event', self.on_mouse_motion)

        # class variables
        self._myPolygon = None

        # class status variables
        self._roiSelectMode = False
        # region of interest. None or 2 tuple of 2-tuple for upper left corner and lower right corner
        self._myROI = None
        self._roiStart = None
        self._roiEnd = None

        # mouse
        self._mousePressed = Detector2DView.MousePress.RELEASED

        # mouse position and resolution
        self._currX = 0.
        self._currY = 0.
        self._resolutionX = 0.005
        self._resolutionY = 0.005

        return

    def add_roi(self):
        """ Add region of interest
        :return:
        """
        vertex_array = np.ndarray(shape=(4, 2))
        vertex_array[0][0] = 10.
        vertex_array[0][1] = 10.
        vertex_array[1][0] = 10.
        vertex_array[1][1] = 20.
        vertex_array[2][0] = 20.
        vertex_array[2][1] = 20.
        vertex_array[3][0] = 20.
        vertex_array[3][1] = 10.

        # TODO - Refactor to Mpl2DGraphicsview as draw_polygon()
        # TODO - create an Art_ID system in Mpl2dGraphicsView to manage artists.
        p = plt.Polygon(vertex_array, fill=False, color='w')
        self._myCanvas.axes.add_artist(p)

        # register
        self._myPolygon = p

        # Flush...
        self._myCanvas._flush()

        return

    def remove_roi(self):
        """
        Remove the rectangular for region of interest
        :return:
        """
        self._myPolygon.remove()

        self._myCanvas._flush()

        return

    def get_roi(self):
        """
        :return: A list for polygon0
        """
        return

    def enter_roi_mode(self, state):
        """
        Enter the region of interest (ROI) selection mode
        :return:
        """
        assert isinstance(state, bool)

        self._roiSelectMode = state

        return

    def on_mouse_motion(self, event):
        """
        Event handing as mouse is moving
        :param event:
        :return:
        """
        # operation if the displacement is too small
        if abs(event.xdata - self._currX) < self.resolutionX() and abs(event.ydata - self._currY) < self.resolutionY():
            return

        if self._mousePressed == Detector2DView.MousePress.RELEASED:
            # No operation if mouse is not pressed
            pass

        elif self._mousePressed == Detector2DView.MousePress.RIGHT:
            # No operation if mouse' right button is pressed
            pass

        elif self._mousePressed == Detector2DView.MousePress.LEFT:
            if self._roiSelectMode is True:
                # in ROI selection mode, update the size
                self.update_roi(event.xdata, event.ydata)

        # update current mouse' position
        self._currX = event.xdata
        self._currY = event.ydata

        return

    def on_mouse_press_event(self, event):
        """

        :param event:
        :return:
        """
        # update mouse' position
        self._currX = event.xdata
        self._currY = event.ydata

        # update mouse' pressed state
        if event.button == 1:
            self._mousePressed = Detector2DView.MousePress.LEFT
        elif event.button == 3:
            self._mousePressed = Detector2DView.MousePress.RIGHT

        # do something?
        if self._roiSelectMode is True and self._mousePressed == Detector2DView.MousePress.LEFT:
            # start to select a region
            self._roiStart = (self._currX, self._currY)

        return

    def on_mouse_release_event(self, event):
        """

        :param event:
        :return:
        """
        # update mouse' position
        self._currX = event.xdata
        self._currY = event.ydata

        # do something
        if self._roiSelectMode is True and self._mousePressed == Detector2DView.MousePress.LEFT:
            # end the ROI selection mode
            self._roiEnd = (self._currX, self._currY)
            self.update_roi()

            # release the mode
            self._roiStart = self._roiEnd = None

        # update button
        self._mousePressed = Detector2DView.MousePress.RELEASED

        return

    def resolutionX(self):
        """

        :return:
        """
        x_min, x_max = self.getXLimit()
        return (x_max - x_min) * self._resolutionX

    def resolutionY(self):
        """

        :return:
        """
        y_min, y_max = self.getYLimit()
        return (y_max - y_min) * self._resolutionY

