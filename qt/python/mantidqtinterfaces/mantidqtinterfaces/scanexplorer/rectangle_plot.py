from typing import Any
from enum import Enum

from matplotlib.widgets import RectangleSelector
from matplotlib.patches import Rectangle
import numpy as np

from mantidqt.widgets.sliceviewer.presenters.lineplots import LinePlots, KeyHandler, cursor_info
from mantid.simpleapi import CreateWorkspace


class UserInteraction(Enum):
    RECTANGLE_CREATED = 1
    RECTANGLE_MOVED = 2
    RECTANGLE_RESHAPED = 3
    RECTANGLE_SELECTED = 4


class MultipleRectangleSelectionLinePlot(KeyHandler):

    STATUS_MESSAGE = "Press key to send roi/cuts to workspaces: r=roi, c=both cuts, x=X, y=Y. Esc clears region"
    SELECTION_KEYS = ('c', 'x', 'y')
    EPSILON = 1e-5

    def __init__(self, plotter: LinePlots, exporter: Any):
        super().__init__(plotter, exporter)

        ax = plotter.image_axes
        self._selector = RectangleSelector(ax, self._on_rectangle_selected, drawtype='box', interactive=True,
                                           ignore_event_outside=False)

        self._rectangles = []
        self._current_rectangle = None

    def _on_rectangle_selected(self, click_event, release_event):
        """
        Called when a rectangle is selected by RectangleSelector. It draws the rectangle and update the plot.
        @param click_event: the event corresponding to the moment the user clicked and started drawing the rectangle
        @param release_event: the event corresponding to the moment the user released the mouse button
        """
        cinfo_click = cursor_info(self.plotter.image, click_event.xdata, click_event.ydata)
        if cinfo_click is None:
            return

        cinfo_release = cursor_info(self.plotter.image, release_event.xdata, release_event.ydata)
        if cinfo_release is None:
            return

        interaction = self._determine_behaviour(click_event, release_event)

        if interaction == UserInteraction.RECTANGLE_CREATED:
            self._draw_rectangle((click_event.xdata, click_event.ydata),
                                 release_event.xdata - click_event.xdata,
                                 release_event.ydata - click_event.ydata)

        elif interaction == UserInteraction.RECTANGLE_SELECTED:
            pass
        elif interaction == UserInteraction.RECTANGLE_MOVED:
            pass
        elif interaction == UserInteraction.RECTANGLE_RESHAPED:
            pass

        self._update_plot_values(cinfo_click.extent)

    def _determine_behaviour(self, click_event, release_event) -> UserInteraction:
        """
        Determine if the user drew another rectangle, moved the currently selected one, changed it, or deselected.
        A bit heuristical since RectangleSelector does not bother telling us what kind of event triggered
         _on_rectangle_selected, only the end result.
        TODO
        """

        # if the start and end points are the same, it's a click
        if click_event.xdata == release_event.xdata and click_event.ydata == release_event.ydata:
            print("click")
            return UserInteraction.RECTANGLE_SELECTED

        if not self._current_rectangle:
            return UserInteraction.RECTANGLE_CREATED

        x0, y0 = self._current_rectangle.get_xy()
        x1 = x0 + self._current_rectangle.get_width()
        y1 = y0 + self._current_rectangle.get_height()

        # TODO float equality is a bad idea, change to epsilon
        if x0 == click_event.xdata or x0 == release_event.xdata or x1 == click_event.xdata or x1 == release_event.xdata:
            if y0 == click_event.ydata or y0 == release_event.ydata or y1 == click_event.ydata or y1 == release_event.ydata:
                return UserInteraction.RECTANGLE_RESHAPED

        if self._current_rectangle.get_width() == abs(click_event.xdata - release_event.xdata) and \
           self._current_rectangle.get_height() == abs(click_event.ydata - release_event.ydata):
            return UserInteraction.RECTANGLE_MOVED

        return UserInteraction.RECTANGLE_CREATED

    def _draw_rectangle(self, point: tuple, width: float, height: float):
        """
        Draw a rectangle at provided coordinates on the image and store the patch for future use
        @param point: the leftmost corner of the rectangle
        @param width: the width of the rectangle. Can be negative.
        @param height: the height of the rectangle. Can be negative.
        """
        rectangle_patch = Rectangle(point, width, height, edgecolor="black", facecolor='none', alpha=.7)
        self.plotter.image_axes.add_patch(rectangle_patch)
        self._rectangles.append(rectangle_patch)
        self._current_rectangle = rectangle_patch

    def _update_plot_values(self, window_range: tuple):
        """
        Update the line plots with the new values.
        @param window_range: the range of the window, in the form (xmin, xmax, ymin, ymax)
        """

        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self._compute_plot_axes(window_range)

        # transmit the new plot values and update
        self.plotter.plot_x_line(x_line_x_axis, x_line_y_values)
        self.plotter.plot_y_line(y_line_x_axis, y_line_y_values)

        self.plotter.update_line_plot_limits()
        self.plotter.redraw()

    def _compute_plot_axes(self, window_range: tuple) -> (tuple, tuple):
        """
        Compute values for the line plots and redraw them. It sums the values from every patch currently drawn.
        @param window_range: the range of the window, in the form (xmin, xmax, ymin, ymax)
        @return the x and y plots, with the x axis and the associated y values, as a tuple of lists.
        """
        xmin, xmax = window_range[0], window_range[1]
        ymin, ymax = window_range[2], window_range[3]
        arr = self.plotter.image.get_array()

        mask_array = np.zeros(arr.shape)

        # prepare x axes
        x_line_x_axis = np.linspace(xmin, xmax, arr.shape[1])
        y_line_x_axis = np.linspace(ymin, ymax, arr.shape[0])

        x_step = (xmax - xmin) / arr.shape[1]
        y_step = (ymax - ymin) / arr.shape[0]

        # add every rectangle to the mask
        for rect in self._rectangles:
            # get rectangle position in the image
            x0, y0 = rect.get_xy()
            x1 = x0 + rect.get_width()
            y1 = y0 + rect.get_height()

            # find the indices corresponding to the position in the array
            x0_ind = int(np.floor((x0 - xmin) / x_step))
            y0_ind = int(np.floor((y0 - ymin) / y_step))

            x1_ind = int(np.ceil((x1 - xmin) / x_step))
            y1_ind = int(np.ceil((y1 - ymin) / y_step))

            # TODO find a more efficient / pythonic way for that
            for x in range(x0_ind, x1_ind):
                for y in range(y0_ind, y1_ind):
                    mask_array[y][x] = 1

        masked_array = np.ma.masked_where(condition=mask_array == 0, a=arr, copy=True)

        x_line_y_values = np.sum(masked_array, axis=0)
        y_line_y_values = np.sum(masked_array, axis=1)

        return (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values)

    def clear(self):
        """
        Clear all the rectangles currently shown
        """
        for rect in self._rectangles:
            rect.remove()
        self._rectangles = []
        self.plotter.update_line_plot_limits()
        self.plotter.redraw()

    def handle_key(self, key: str):
        """
        Handle user key inputs, if they are supported keys. For now, create cuts only.
        @param key: the character pressed by the user
        """
        if key not in self.SELECTION_KEYS:
            return
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self._compute_plot_axes(self.plotter.image.get_extent())
        if key in ('c', 'x'):
            CreateWorkspace(DataX=x_line_x_axis, DataY=x_line_y_values, OutputWorkspace="x_cut")
        if key in ('c', 'y'):
            CreateWorkspace(DataX=y_line_x_axis, DataY=y_line_y_values, OutputWorkspace="y_cut")


def is_the_same(point_a, point_b, epsilon):
    return ((point_a[0] == 0 and point_b[0] == 0) or abs(point_a[0] - point_b[0]) / max(abs(point_a[0]), abs(point_b[0])) < epsilon) \
            and ((point_a[1] == 0 and point_b[1] == 0) or abs(point_a[1] - point_b[1]) / max(abs(point_a[1]), abs(point_b[1])) < epsilon)
