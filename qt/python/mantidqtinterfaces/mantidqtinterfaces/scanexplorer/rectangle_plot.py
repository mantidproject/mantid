from typing import Any
from enum import Enum
from bisect import bisect_left

from matplotlib.widgets import RectangleSelector
from matplotlib.patches import Rectangle
import numpy as np

from mantidqt.widgets.sliceviewer.presenters.lineplots import LinePlots, KeyHandler, cursor_info
from mantid.simpleapi import CreateWorkspace
from mantid.kernel import logger

from .rectangle_controller import RectanglesManager


class UserInteraction(Enum):
    RECTANGLE_CREATED = 1
    RECTANGLE_MOVED = 2
    RECTANGLE_RESHAPED = 3
    RECTANGLE_SELECTED = 4


class MultipleRectangleSelectionLinePlot(KeyHandler):

    STATUS_MESSAGE = "Press key to send roi/cuts to workspaces: r=roi, c=both cuts, x=X, y=Y. Esc clears region"
    SELECTION_KEYS = ('c', 'x', 'y', 'f', "delete")
    EPSILON = 1e-3

    def __init__(self, plotter: LinePlots, exporter: Any):
        super().__init__(plotter, exporter)

        ax = plotter.image_axes
        self._selector = RectangleSelector(ax, self._on_rectangle_selected, drawtype='box', interactive=True,
                                           ignore_event_outside=False)

        self._manager: RectanglesManager = exporter.rectangles_manager

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

        click_event_pos = (click_event.xdata, click_event.ydata)
        release_event_pos = (release_event.xdata, release_event.ydata)

        if interaction == UserInteraction.RECTANGLE_CREATED:
            point, width, height = self._snap_to_edges(click_event_pos, release_event_pos)
            self._draw_rectangle(point, width, height)

        elif interaction == UserInteraction.RECTANGLE_SELECTED:
            self._select_rectangle(click_event_pos)

        elif interaction in (UserInteraction.RECTANGLE_MOVED, UserInteraction.RECTANGLE_RESHAPED):
            point, width, height = self._snap_to_edges(click_event_pos, release_event_pos)
            self._move_selected_rectangle(point, width, height)

        self._update_plot_values()

    def _determine_behaviour(self, click_event, release_event) -> UserInteraction:
        """
        Determine if the user drew another rectangle, moved the currently selected one, changed it, or deselected.
        A bit heuristical since RectangleSelector does not bother telling us what kind of event triggered
         _on_rectangle_selected, only the end result.
        @param click_event: the event triggered by the user clicking
        @param release_event: the event triggered by the user releasing the mouse button.
        """

        # if the start and end points are the same, it's a click
        if click_event.xdata == release_event.xdata and click_event.ydata == release_event.ydata:
            return UserInteraction.RECTANGLE_SELECTED

        # if there is no rectangle on screen, we are creating one
        if not self.current_rectangle:
            return UserInteraction.RECTANGLE_CREATED

        x0, y0 = self.current_rectangle.get_xy()
        x1 = x0 + self.current_rectangle.get_width()
        y1 = y0 + self.current_rectangle.get_height()

        # if one corner didn't change from the currently selected rectangle, we assume it has been reshaped
        if self.is_almost_equal(x0, click_event.xdata) or self.is_almost_equal(x0, release_event.xdata) or \
                self.is_almost_equal(x1, click_event.xdata) or self.is_almost_equal(x1, release_event.xdata):
            if self.is_almost_equal(y0, click_event.ydata) or self.is_almost_equal(y0, release_event.ydata) or \
                    self.is_almost_equal(y1, click_event.ydata) or self.is_almost_equal(y1, release_event.ydata):
                return UserInteraction.RECTANGLE_RESHAPED

        # if the shape didn't change from the currently selected rectangle, we assume it has been moved
        if self.is_almost_equal(self.current_rectangle.get_width(), abs(click_event.xdata - release_event.xdata)) and \
           self.is_almost_equal(self.current_rectangle.get_height(), abs(click_event.ydata - release_event.ydata)):
            return UserInteraction.RECTANGLE_MOVED

        return UserInteraction.RECTANGLE_CREATED

    def _snap_to_edges(self, point_1: tuple, point_2: tuple) -> (tuple, float, float):
        """
        Snap the boundaries of the drawn rectangle to the nearest edges
        @param point_1: a corner of the rectangle
        @param point_2: the opposing corner of the rectangle
        @return a corner of the new rectangle and its width and height
        """
        x0, y0 = point_1
        x1, y1 = point_2

        x_axis_values, y_axis_values = self.exporter.get_axes()

        def closest_value(axis, value, mini, maxi):
            """Find the closest edge to the given value along given axis"""
            idx = bisect_left(axis, value)

            if idx == 0:
                return mini
            if idx == len(axis):
                return maxi

            # edges are at the middle point between axis values
            edge = (axis[idx] + axis[idx - 1]) / 2

            # conditions to manage the weirdly cut limits of the plot. See slice viewer to understand better
            if idx == len(axis) - 1 and value > (edge + maxi) / 2:
                return maxi
            if idx == 1 and value < (edge + mini) / 2:
                return mini

            return edge

        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        x0 = closest_value(x_axis_values, x0, xmin, xmax)
        y0 = closest_value(y_axis_values, y0, ymin, ymax)
        x1 = closest_value(x_axis_values, x1, xmin, xmax)
        y1 = closest_value(y_axis_values, y1, ymin, ymax)

        self._selector.extents = (x0, x1, y0, y1)

        return (x0, y0), x1 - x0, y1 - y0

    def _select_rectangle(self, point: tuple):
        """
        Select the rectangle at position point, if there is one. The rectangle selected is the first one found,
        i.e. the oldest one, normally.
        @param point : the position of the click, as (x, y) coordinates
        """
        xpos, ypos = point
        for rect in self.get_rectangles():
            x0, y0 = rect.get_xy()
            x1 = x0 + rect.get_width()
            y1 = y0 + rect.get_height()

            if x0 <= xpos <= x1 and y0 <= ypos <= y1:
                self._selector.extents = (x0, x1, y0, y1)
                self._manager.set_as_current_rectangle(rect)
                return

    def _draw_rectangle(self, point: tuple, width: float, height: float):
        """
        Draw a rectangle at provided coordinates on the image and store the patch for future use
        @param point: the leftmost corner of the rectangle
        @param width: the width of the rectangle. Can be negative.
        @param height: the height of the rectangle. Can be negative.
        """
        rectangle_patch = Rectangle(point, width, height, edgecolor="black", facecolor='none', alpha=.7)
        self.plotter.image_axes.add_patch(rectangle_patch)
        self._manager.add_rectangle(rectangle_patch)

    def _move_selected_rectangle(self, new_point: tuple, new_width: float, new_height: float):
        """
        Move or reshape the currently selected rectangle.
        @param new_point: the new starting corner of the rectangle
        @param new_width: the new width
        @param new_height: the new height
        """
        rectangle_patch = Rectangle(new_point, new_width, new_height, edgecolor="black", facecolor='none', alpha=0.7)
        self.plotter.image_axes.add_patch(rectangle_patch)

        self.current_rectangle.remove()
        self._manager.move_current(rectangle_patch)

    def _update_plot_values(self):
        """
        Update the line plots with the new values.
        """

        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self._compute_plot_axes()

        # transmit the new plot values and update
        self.plotter.plot_x_line(x_line_x_axis, x_line_y_values)
        self.plotter.plot_y_line(y_line_x_axis, y_line_y_values)

        self.plotter.update_line_plot_limits()
        self.plotter.redraw()

    def _compute_plot_axes(self) -> (tuple, tuple):
        """
        Compute values for the line plots and redraw them. It sums the values from every patch currently drawn.
        @return the x and y plots, with the x axis and the associated y values, as a tuple of lists.
        """
        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        arr = self.plotter.image.get_array()

        mask_array = np.zeros(arr.shape)

        # prepare x axes
        x_line_x_axis = np.linspace(xmin, xmax, arr.shape[1])
        y_line_x_axis = np.linspace(ymin, ymax, arr.shape[0])

        x_step = (xmax - xmin) / arr.shape[1]
        y_step = (ymax - ymin) / arr.shape[0]

        # add every rectangle to the mask
        for rect in self.get_rectangles():
            # get rectangle position in the image
            x0, y0 = rect.get_xy()
            x1 = x0 + rect.get_width()
            y1 = y0 + rect.get_height()

            # find the indices corresponding to the position in the array
            x0_ind = max(int(np.ceil((x0 - xmin) / x_step)), 0)
            y0_ind = max(int(np.ceil((y0 - ymin) / y_step)), 0)

            x1_ind = min(int(np.floor((x1 - xmin) / x_step)), len(arr[0]))
            y1_ind = min(int(np.floor((y1 - ymin) / y_step)), len(arr))

            # TODO find a more efficient / pythonic way for that
            for x in range(x0_ind, x1_ind):
                for y in range(y0_ind, y1_ind):
                    mask_array[y][x] = 1

        masked_array = np.ma.masked_where(condition=mask_array == 0, a=arr, copy=True)

        x_line_y_values = np.sum(masked_array, axis=0)
        y_line_y_values = np.sum(masked_array, axis=1)

        return (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values)

    def _place_interpolated_rectangles(self):
        """
        Place new rectangles based on those already placed by the user. Only linearly interpolate new positions
        from the center of the drawn rectangles. Only supports 2 rectangles.
        """
        rectangles = self.get_rectangles()
        if len(rectangles) != 2:
            logger.warning("Cannot place more peak regions : current number of regions invalid "
                           "(2 expected, {} found)".format(len(rectangles)))
            return

        rect_0, rect_1 = rectangles
        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        new_height = (rect_0.get_height() + rect_1.get_height()) / 2
        new_width = (rect_0.get_width() + rect_1.get_width()) / 2

        center_0 = np.array((rect_0.get_x() + rect_0.get_width() / 2, rect_0.get_y() + rect_0.get_height() / 2))
        center_1 = np.array((rect_1.get_x() + rect_1.get_width() / 2, rect_1.get_y() + rect_1.get_height() / 2))

        def rectangle_fit_on_image(center):
            """
            Check if the rectangle with center at center fits in the image boundaries.
            @param center: the center of the rectangle
            """
            # TODO why no / 2 on the second member ?
            return xmin <= center[0] + new_width / 2 <= xmax and xmin <= center[0] - new_width <= xmax and \
                ymin <= center[1] + new_height / 2 <= ymax and ymin <= center[1] - new_height <= ymax

        def move(seed: np.array, offset: np.array):
            """
            Starting at seed, place a rectangle every offset
            @param seed: the center of the first rectangle to place
            @param offset: the offset between each rectangle, in both x and y.
            """
            while rectangle_fit_on_image(seed):
                self._draw_rectangle((seed[0] - new_width / 2, seed[1] - new_height / 2), new_width, new_height)
                seed += offset

        first_center = 2 * center_1 - center_0
        move(first_center, center_1 - center_0)

        first_center = 2 * center_0 - center_1
        move(first_center, center_0 - center_1)

        self._update_plot_values()

    def _delete_current(self):
        """
        Delete currently selected rectangle.
        """
        self._manager.delete_current()
        self._update_plot_values()
        self._selector.set_visible(False)

    def clear(self):
        """
        Clear all the rectangles currently shown
        """
        self._manager.clear()
        self._update_plot_values()

    def handle_key(self, key: str):
        """
        Handle user key inputs, if they are supported keys.
        @param key: the character pressed by the user
        """
        if key not in self.SELECTION_KEYS:
            return
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self._compute_plot_axes()

        if key in ('c', 'x'):
            CreateWorkspace(DataX=x_line_x_axis, DataY=x_line_y_values, OutputWorkspace="x_cut")
        if key in ('c', 'y'):
            CreateWorkspace(DataX=y_line_x_axis, DataY=y_line_y_values, OutputWorkspace="y_cut")
        if key == 'f':
            self._place_interpolated_rectangles()
        if key == 'delete':
            self._delete_current()

    @property
    def current_rectangle(self):
        """
        Get the current rectangle patch
        """
        return self._manager.get_current_rectangle()

    def get_rectangles(self):
        return self._manager.get_rectangles()

    @classmethod
    def is_almost_equal(cls, a, b):
        """
        Check if 2 numbers are within epsilon of relative distance
        """
        return a == b == 0 or abs(a - b) / max(abs(a), abs(b)) < cls.EPSILON


def is_the_same(point_a, point_b, epsilon):
    return ((point_a[0] == 0 and point_b[0] == 0) or abs(point_a[0] - point_b[0]) / max(abs(point_a[0]), abs(point_b[0])) < epsilon) \
            and ((point_a[1] == 0 and point_b[1] == 0) or abs(point_a[1] - point_b[1]) / max(abs(point_a[1]), abs(point_b[1])) < epsilon)


def get_opposing_corners(point, width, height):
    return point[0], point[1], point[0] + width, point[1] + height
