#pylint: disable=W0403,R0904,R0903
import mplgraphicsview


class IntegratedPeakView(mplgraphicsview.MplGraphicsView):
    """ Extended graphic view for integrated peaks
    """
    class MousePress(object):
        RELEASED = 0
        LEFT = 1
        RIGHT = 2

    def __init__(self, parent):
        """ Init
        :param parent:
        :return:
        """
        mplgraphicsview.MplGraphicsView.__init__(self, parent)

        # define interaction with the canvas
        self._myCanvas.mpl_connect('button_press_event', self.on_mouse_press_event)
        self._myCanvas.mpl_connect('button_release_event', self.on_mouse_release_event)
        self._myCanvas.mpl_connect('motion_notify_event', self.on_mouse_motion)

        # class variable
        self._mousePressed = self.MousePress.RELEASED

        self._bkgdIndicatorID = -1

        # current mouse position
        self._currX = 0.
        self._currY = 0.

        return

    def add_background_indictor(self):
        """
        :return:
        """
        min_y, max_y = self.getYLimit()
        self._bkgdIndicatorID = self.add_horizontal_indicator(y=min_y+0.5*(max_y-min_y), color='red')

        return

    def on_mouse_motion(self, event):
        """

        :param event:
        :return:
        """
        self._currX = event.xdata
        self._currY = event.ydata

        return

    def on_mouse_press_event(self, event):
        """

        :return:
        """
        if event.button == 1:
            self._mousePressed = self.MousePress.LEFT
        elif event.button == 3:
            self._mousePressed = self.MousePress.RIGHT

        return

    def on_mouse_release_event(self, event):
        """

        :param event:
        :return:
        """
        self._mousePressed = self.MousePress.RELEASED

        print event.y, event.ydata

        return

    def set_smart_y_limit(self, vec_y):
        """
        Set limit on Y axis automatically (in a 'smart' way), i.e.,
        - to the smaller of zero and 10 percent delta Y under minimum Y
        - to 10 percent delta Y above maximum Y
        :return:
        """
        # check
        assert len(vec_y) > 0

        # find y's minimum and maximum
        min_y = min(vec_y)
        max_y = max(vec_y)

        d_y = max_y - min_y

        # set Y to the smaller of zero and 10 percent delta Y under minimum Y
        if min_y > 0:
            y_lower_limit = 0
        else:
            y_lower_limit = min_y - 0.1 * d_y

        # set Y to 10 percent delta Y above maximum Y
        y_upper_limit = max_y + 0.1 * d_y

        self.setXYLimit(ymin=y_lower_limit, ymax=y_upper_limit)

        return
