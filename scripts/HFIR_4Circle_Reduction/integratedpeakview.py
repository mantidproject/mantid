#pylint: disable=W0403,R0904,R0903
import numpy
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

        # data managing
        self._rawDataID = None
        self._modelDataID = None

        return

    def add_background_indictor(self):
        """
        :return:
        """
        min_y, max_y = self.getYLimit()
        self._bkgdIndicatorID = self.add_horizontal_indicator(y=min_y+0.5*(max_y-min_y), color='red')

        return

    def get_raw_data(self):
        """
        :exception: RuntimeError if no plot on canvas
        :return: 2-tuple as vec_x and vec_y
        """
        if self._rawDataID is None:
            raise RuntimeError('There is no raw data plot on the canvas')

        data_set = self.canvas().get_data(self._rawDataID)
        vec_x = data_set[0]
        vec_y = data_set[1]

        return vec_x, vec_y

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

        return

    def plot_raw_data(self, vec_x, vec_y):
        """
        plot raw data, which will be recorded by _rawDataID
        :param vec_x:
        :param vec_y:
        :return:
        """
        # plot data
        self._rawDataID = self.add_plot_1d(vec_x, vec_y,  color='blue')
        self.set_smart_y_limit(vec_y)

        return

    def plot_model(self, vec_x, model_vec_y, title=None):
        """
        plot model data which will be recorded by
        :param vec_x:
        :param model_vec_y:
        :param title:
        :return:
        """
        # plot data
        self._modelDataID = self.add_plot_1d(vec_x, model_vec_y)
        if title is not None:
            self.set_title(title)
        self.set_smart_y_limit(model_vec_y)

        self.setXYLimit(xmin=vec_x[0] - 1., xmax=vec_x[-1] + 1.)

        return

    def remove_model(self):
        """
        remove the plot for model
        :return:
        """
        if self._modelDataID is None:
            raise RuntimeError('There is no model plot on canvas')

        # reset title
        self.set_title('')
        self.remove_line(self._modelDataID)

        self._modelDataID = None

        return

    def reset(self):
        """
        reset the canvas and thus the handler to the plots
        :return:
        """
        # clear all lines
        self.clear_all_lines()

        # reset handlers
        self._rawDataID = None
        self._modelDataID = None

        # reset title
        self.set_title('')

        return

    def set_smart_y_limit(self, vec_y):
        """
        Set limit on Y axis automatically (in a 'smart' way), i.e.,
        - to the smaller of zero and 10 percent delta Y under minimum Y
        - to 10 percent delta Y above maximum Y
        :return:
        """
        # check
        assert isinstance(vec_y, numpy.ndarray) and len(vec_y) > 0, 'Vector Y must be a numpy array and not empty.'

        # find y's minimum and maximum
        try:
            min_y = numpy.min(vec_y)
            max_y = numpy.max(vec_y)
        except ValueError as value_err:
            raise RuntimeError(str(value_err))

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
