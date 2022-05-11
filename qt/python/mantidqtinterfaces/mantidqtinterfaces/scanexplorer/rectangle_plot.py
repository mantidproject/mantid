from typing import Any

from matplotlib.widgets import RectangleSelector
from matplotlib.patches import Rectangle
import numpy as np

from mantidqt.widgets.sliceviewer.presenters.lineplots import LinePlots, KeyHandler, cursor_info
from mantid.simpleapi import CreateWorkspace


class MultipleRectangleSelectionLinePlot(KeyHandler):

    STATUS_MESSAGE = "Press key to send roi/cuts to workspaces: r=roi, c=both cuts, x=X, y=Y. Esc clears region"
    SELECTION_KEYS = ('c', 'x', 'y')

    def __init__(self, plotter: LinePlots, exporter: Any):
        super().__init__(plotter, exporter)

        ax = plotter.image_axes
        self._selector = RectangleSelector(ax, self._on_rectangle_selected)

        self._rectangles = []

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

        self._draw_rectangle((click_event.xdata, click_event.ydata),
                             release_event.xdata - click_event.xdata,
                             release_event.ydata - click_event.ydata)

        self._update_plot_values(cinfo_click.extent)

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
