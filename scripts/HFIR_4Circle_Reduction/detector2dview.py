import mpl2dgraphicsview

import numpy as np


class Detector2DView(mpl2dgraphicsview.Mpl2dGraphicsView):
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
        mpl2dgraphicsview.Mpl2dGraphicsView.__init__(self, parent)

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
        # check
        assert self._roiStart is not None
        assert self._roiEnd is not None

        print '[DB] Polygon corner = [%s, %s]' % (str(self._roiStart), str(self._roiEnd))

        # create a vertex list of a rectangular
        vertex_array = np.ndarray(shape=(4, 2))
        # upper left corner
        vertex_array[0][0] = self._roiStart[0]
        vertex_array[0][1] = self._roiStart[1]

        # lower right corner
        vertex_array[2][0] = self._roiEnd[0]
        vertex_array[2][1] = self._roiEnd[1]

        # upper right corner
        vertex_array[1][0] = self._roiEnd[0]
        vertex_array[1][1] = self._roiStart[1]

        # lower left corner
        vertex_array[3][0] = self._roiStart[0]
        vertex_array[3][1] = self._roiEnd[1]

        self._myCanvas.plot_polygon(vertex_array, fill=False, color='w')

        return

    def enter_roi_mode(self, state):
        """
        Enter the region of interest (ROI) selection mode
        :return:
        """
        assert isinstance(state, bool)

        self._roiSelectMode = state

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
                self.update_roi_poly(event.xdata, event.ydata)

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
            self.update_roi_poly()

            # release the mode
            self._roiStart = self._roiEnd = None

        # update button
        self._mousePressed = Detector2DView.MousePress.RELEASED

        return

    def resolutionX(self):
        """

        :return:
        """
        return (self.x_max - self.x_min) * self._resolutionX

    def resolutionY(self):
        """

        :return:
        """
        return (self.y_max - self.y_min) * self._resolutionY

    def update_roi_poly(self, cursor_x, cursor_y):
        """ Update region of interest
        :return:
        """
        # remove the original polygon
        if self._myPolygon is not None:
            self._myPolygon.remove()

        # set RIO end
        self._roiEnd = [cursor_x, cursor_y]

        # plot the new polygon
        self.add_roi()

        return



