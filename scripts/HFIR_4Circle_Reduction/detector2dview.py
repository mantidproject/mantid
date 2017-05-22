#pylint: disable=W0403,R0902,R0903,R0904,W0212
import os
import numpy as np
import mpl2dgraphicsview


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
        self._myPolygon = None  # matplotlib.patches.Polygon

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

        # parent window
        self._myParentWindow = None

        return

    def add_roi(self, roi_start, roi_end):
        """ Add region of interest
        :param roi_start:
        :param roi_end:
        :return:
        """
        # check
        assert isinstance(roi_start, tuple) and len(roi_start) == 2
        assert isinstance(roi_end, tuple) and len(roi_end) == 2

        # set
        self._roiStart = roi_start
        self._roiEnd = roi_end

        # plot
        self.plot_roi()

        return

    def enter_roi_mode(self, state):
        """
        Enter or leave the region of interest (ROI) selection mode
        :return:
        """
        assert isinstance(state, bool)

        # set
        self._roiSelectMode = state

        if state:
            # new in add-ROI mode
            self._roiStart = None
            self._roiEnd = None
            self._myPolygon = None

        # # reset _myPolygen
        # if state is False:
        #     if self._myPolygon is not None:
        #         self.remove_roi()

        return

    def integrate_roi_linear(self, exp_number, scan_number, pt_number, output_dir):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        def save_to_file(base_file_name, axis, array1d, start_index):
            """
            save the result (1D data) to an ASCII file
            :param base_file_name:
            :param axis:
            :param array1d:
            :param start_index:
            :return:
            """
            file_name = '{0}_axis_{1}.dat'.format(base_file_name, axis)

            wbuf = ''
            vec_x = np.array(range(len(array1d))) + start_index
            for i in range(len(array1d)):
                wbuf += '{0} \t{1}\n'.format(vec_x[i], array1d[i])

            ofile = open(file_name, 'w')
            ofile.write(wbuf)
            ofile.close()

            return

        matrix = self.array2d
        assert isinstance(matrix, np.ndarray), 'A matrix must be an ndarray but not {0}.'.format(type(matrix))

        # get region of interest
        if self._roiStart is None:
            self._roiStart = (0, 0)
        if self._roiEnd is None:
            self._roiEnd = matrix.shape

        ll_row = min(self._roiStart[0], self._roiEnd[0])
        ll_col = min(self._roiStart[1], self._roiEnd[1])

        ur_row = max(self._roiStart[0], self._roiEnd[0])
        ur_col = max(self._roiStart[1], self._roiEnd[1])

        # print 'Row: {0} : {1}  Col: {2} : {3}'.format(ll_row, ur_row, ll_col, ur_col)

        roi_matrix = matrix[ll_col:ur_col, ll_row:ur_row]

        sum_0 = roi_matrix.sum(0)
        #  print sum_0
        sum_1 = roi_matrix.sum(1)
        #  print sum_1
        print '[SUM 0] Dimension: {0}'.format(sum_0.shape)
        print '[SUM 1] Dimension: {0}'.format(sum_1.shape)

        # write to file
        base_name = os.path.join(output_dir, 'Exp{0}_Scan{1}_Pt{2}'.format(exp_number, scan_number, pt_number))
        save_to_file(base_name, 0, sum_0, ll_row)
        save_to_file(base_name, 1, sum_1, ll_col)

        message = 'Integrated values are saved to {0}...'.format(base_name)

        return message

    def get_roi(self):
        """
        :return: A list for polygon0
        """
        assert self._roiStart is not None
        assert self._roiEnd is not None

        # rio start is upper left, roi end is lower right
        lower_left = (self._roiStart[0], self._roiEnd[1])
        upper_right = (self._roiEnd[0], self._roiStart[1])

        return lower_left, upper_right

    def plot_roi(self):
        """ Plot region of interest (as rectangular) to the canvas from the region set from
        :return:
        """
        # check
        assert self._roiStart is not None
        assert self._roiEnd is not None

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

        # register
        self._myPolygon = self._myCanvas.plot_polygon(vertex_array, fill=False, color='w')

        return

    def remove_roi(self):
        """
        Remove the rectangular for region of interest
        :return:
        """
        if self._myPolygon is not None:
            # polygon is of type matplotlib.patches.Polygon
            self._myPolygon.remove()
            self._myPolygon = None

            # FUTURE-TO-DO: this should be replaced by some update() method of canvas
            self._myCanvas._flush()

            self._roiStart = None
            self._roiEnd = None

        return

    def on_mouse_motion(self, event):
        """
        Event handing as mouse is moving
        :param event:
        :return:
        """
        # skip if the mouse cursor is still outside of the canvas
        if event.xdata is None or event.ydata is None:
            return

        # check: _currX and _currY must be specified
        assert self._currX is not None and self._currY is not None

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
        # return if the cursor position is out of canvas
        if event.xdata is None or event.ydata is None:
            return

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
        # return without any operation if mouse cursor is out side of canvas
        if event.xdata is None or event.ydata is None:
            return

        # update mouse' position
        self._currX = event.xdata
        self._currY = event.ydata

        # update button
        prev_mouse_pressed = self._mousePressed
        self._mousePressed = Detector2DView.MousePress.RELEASED

        # do something
        if self._roiSelectMode and prev_mouse_pressed == Detector2DView.MousePress.LEFT:
            # end the ROI selection mode
            self.update_roi_poly(self._currX, self._currY)
        # END-IF

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

    def set_parent_window(self, parent_window):
        """
        Set the parent window for synchronizing the operation
        :param parent_window:
        :return:
        """
        assert parent_window is not None, 'Parent window cannot be None'

        self._myParentWindow = parent_window

        return

    def update_roi_poly(self, cursor_x, cursor_y):
        """ Update region of interest.  It is to
        (1) remove the original polygon
        (2) draw a new polygon
        :return:
        """
        # check
        assert isinstance(cursor_x, float), 'Cursor x coordination {0} must be a float.'.format(cursor_x)
        assert isinstance(cursor_y, float), 'Cursor y coordination {0} must be a float.'.format(cursor_y)

        # remove the original polygon
        if self._myPolygon is not None:
            self._myPolygon.remove()
            self._myPolygon = None
            # self.canvas._flush()

        # set RIO end
        self._roiEnd = [cursor_x, cursor_y]

        # plot the new polygon
        self.plot_roi()

        # update
        if self._myPolygon is not None:
            self._myParentWindow.do_apply_roi()

        return
