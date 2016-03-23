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
