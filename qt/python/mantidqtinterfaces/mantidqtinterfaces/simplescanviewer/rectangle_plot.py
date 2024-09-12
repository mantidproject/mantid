# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Any, Tuple
from enum import Enum
from bisect import bisect_left

from matplotlib.patches import Rectangle
from matplotlib.backend_bases import MouseEvent
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QWidget
import numpy as np

from mantidqt.widgets.sliceviewer.presenters.lineplots import LinePlots, KeyHandler, cursor_info
from mantid.simpleapi import CreateWorkspace, CreateEmptyTableWorkspace, Fit
from mantid.kernel import logger

from .rectangle_controller import RectanglesManager
from .rectangle_selection import RectangleSelection


class UserInteraction(Enum):
    RECTANGLE_CREATED = 1
    RECTANGLE_MOVED = 2
    RECTANGLE_RESHAPED = 3
    RECTANGLE_SELECTED = 4


class MultipleRectangleSelectionLinePlot(KeyHandler):
    STATUS_MESSAGE = "Press key to export: c=both cuts, x=X, y=Y, p=peaks. 'Del' deletes a ROI. " "'f' interpolates rectangles."
    SELECTION_KEYS = ("c", "x", "y", "f", "delete", "p")
    EPSILON = 1e-3

    def __init__(self, plotter: LinePlots, exporter: Any):
        super().__init__(plotter, exporter)

        ax = plotter.image_axes
        self._selector = RectangleSelection(ax, self._on_rectangle_selected, interactive=True)
        self.signals = MultipleRectangleSelectionLinePlotSignals()
        self._manager: RectanglesManager = exporter.rectangles_manager

        self.signals.sig_current_updated.connect(self._manager.on_current_updated)
        self._manager.sig_controller_updated.connect(self._on_controller_updated)

    def _on_rectangle_selected(self, click_event: MouseEvent, release_event: MouseEvent):
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

    def _determine_behaviour(self, click_event: MouseEvent, release_event: MouseEvent) -> UserInteraction:
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
        if (
            self.is_almost_equal(x0, click_event.xdata)
            or self.is_almost_equal(x0, release_event.xdata)
            or self.is_almost_equal(x1, click_event.xdata)
            or self.is_almost_equal(x1, release_event.xdata)
        ):
            if (
                self.is_almost_equal(y0, click_event.ydata)
                or self.is_almost_equal(y0, release_event.ydata)
                or self.is_almost_equal(y1, click_event.ydata)
                or self.is_almost_equal(y1, release_event.ydata)
            ):
                return UserInteraction.RECTANGLE_RESHAPED

        # if the shape didn't change from the currently selected rectangle, we assume it has been moved
        if self.is_almost_equal(self.current_rectangle.get_width(), abs(click_event.xdata - release_event.xdata)) and self.is_almost_equal(
            self.current_rectangle.get_height(), abs(click_event.ydata - release_event.ydata)
        ):
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

        def closest_edge(axis: np.ndarray, value: float, mini: float, maxi: float) -> float:
            """
            Find the closest edge to the given value along given axis
            @param axis: the array to use to find the edge
            @param value: we are searching the closest edge of this value
            @param mini: the minimum value an edge can have. It can change depending on the state of the slice viewer
            and the axis array does not help finding it so we have to provide it.
            @param maxi: the maximum value an edge can have.
            @return the value of the closest edge
            """
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

        def find_nearest_with_gap(axis: np.ndarray, first_value: float, second_value: float, mini: float, maxi: float) -> (float, float):
            """
            Find the nearest edges for each values so that they are not the same.
            Most of the times it is just the closest one, but if both are closest to the same one,
            it means finding the second closest for the value that is the most distant from the closest
            @param axis: the array to use to find the nearest edges
            @param first_value: one of the limits to snap to the nearest edge
            @param second_value: the other limit
            @param mini: the minimum value an edge can have. It can change depending on the state of the slice viewer
            and the axis array does not help finding it so we have to provide it.
            @param maxi: the maximum value an edge can have.
            @return the two new values for the limits, that correspond to edges
            """

            closest_1 = closest_edge(axis, first_value, mini, maxi)
            closest_2 = closest_edge(axis, second_value, mini, maxi)

            # nice case, they snap to different edges
            if closest_1 != closest_2:
                return closest_1, closest_2

            # they both snap to the bottom of the image: we take the edge just above
            if closest_1 == mini:
                edge = (axis[0] + axis[1]) / 2
                return (mini, edge) if first_value < second_value else (edge, mini)

            # they both snap to the top of the image: we take the edge just below
            if closest_1 == maxi:
                edge = (axis[-1] + axis[-2]) / 2
                return (edge, maxi) if first_value < second_value else (maxi, edge)

            # they both snap to the same edge: the one farthest from that edge snaps to the other side's edge
            idx = bisect_left(axis, first_value)
            edge = (axis[idx] + axis[idx - 1]) / 2

            previous_edge = (axis[idx - 1] + axis[idx - 2]) / 2 if idx > 1 else mini
            next_edge = (axis[idx] + axis[idx + 1]) / 2 if idx < len(axis) - 1 else maxi

            if abs(first_value - edge) < abs(second_value - edge):
                return (edge, previous_edge) if second_value < edge else (edge, next_edge)
            else:
                return (previous_edge, edge) if first_value < edge else (next_edge, edge)

        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        x0, x1 = find_nearest_with_gap(x_axis_values, x0, x1, xmin, xmax)
        y0, y1 = find_nearest_with_gap(y_axis_values, y0, y1, ymin, ymax)

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
        rectangle_patch = Rectangle(point, width, height, edgecolor="black", facecolor="none", alpha=0.7)
        self.plotter.image_axes.add_patch(rectangle_patch)
        self._manager.add_rectangle(rectangle_patch)
        peak = self._find_peak(self.current_rectangle)
        self._show_peak(self.current_rectangle, peak)
        self._update_plot_values()

    def _move_selected_rectangle(self, new_point: tuple, new_width: float, new_height: float):
        """
        Move or reshape the currently selected rectangle.
        @param new_point: the new starting corner of the rectangle
        @param new_width: the new width
        @param new_height: the new height
        """
        self.current_rectangle.set_bounds(*new_point, new_width, new_height)
        peak = self._find_peak(self.current_rectangle)
        self._show_peak(self.current_rectangle, peak)
        self.signals.sig_current_updated.emit()
        self._update_plot_values()

    def _on_controller_updated(self, rectangle_patch: Rectangle):
        """
        Slot called when the controller associated to this patch is updated to these new values
        @param rectangle_patch: the updated patch
        """
        new_x0, new_y0 = rectangle_patch.get_xy()
        new_x1 = new_x0 + rectangle_patch.get_width()
        new_y1 = new_y0 + rectangle_patch.get_height()
        self._update_plot_values()

        peak = self._find_peak(rectangle_patch)
        self._show_peak(rectangle_patch, peak)

        if self.current_rectangle == rectangle_patch:
            self._selector.extents = (new_x0, new_x1, new_y0, new_y1)

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

            mask_array[y0_ind:y1_ind, x0_ind:x1_ind] = 1

        masked_array = np.ma.masked_where(condition=mask_array == 0, a=arr, copy=True)

        x_line_y_values = np.sum(masked_array, axis=0)
        y_line_y_values = np.sum(masked_array, axis=1)

        return (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values)

    def _extract_projections(self, extract_x=True, extract_y=True):
        """
        Extract the projections of the roi, and then each rectangle projection in a different workspace.
        Note that the sum of all rectangles' individual curves can be different from the sum computed
        by _compute_plot_axis, because of possible intersections that are not counted twice by the later function.
        @param extract_x: whether to extract x-axis projections
        @param extract_y: whether to extract y-axis projections
        """
        if not extract_x and not extract_y:
            return

        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        arr = self.plotter.image.get_array()

        x_step = (xmax - xmin) / arr.shape[1]
        y_step = (ymax - ymin) / arr.shape[0]

        for index, rect in enumerate(self.get_rectangles()):
            # get rectangle position in the image
            x0, y0 = rect.get_xy()
            x1 = x0 + rect.get_width()
            y1 = y0 + rect.get_height()

            # find the indices corresponding to the position in the array
            x0_ind = max(int(np.ceil((x0 - xmin) / x_step)), 0)
            y0_ind = max(int(np.ceil((y0 - ymin) / y_step)), 0)

            x1_ind = min(int(np.floor((x1 - xmin) / x_step)), len(arr[0]))
            y1_ind = min(int(np.floor((y1 - ymin) / y_step)), len(arr))

            slice_cut = arr[y0_ind:y1_ind, x0_ind:x1_ind]

            if extract_x:
                x_line_x_axis = np.linspace(x0, x1, x1_ind - x0_ind)
                x_line_y_values = np.sum(slice_cut, axis=0)
                CreateWorkspace(DataX=x_line_x_axis, DataY=x_line_y_values, OutputWorkspace="x_cut_{}".format(index))

            if extract_y:
                y_line_x_axis = np.linspace(y0, y1, y1_ind - y0_ind)
                y_line_y_values = np.sum(slice_cut, axis=1)
                CreateWorkspace(DataX=y_line_x_axis, DataY=y_line_y_values, OutputWorkspace="y_cut_{}".format(index))

        # extract the complete projection (which is the line plot)
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self._compute_plot_axes()

        if extract_x:
            CreateWorkspace(DataX=x_line_x_axis, DataY=x_line_y_values, OutputWorkspace="x_cut")
        if extract_y:
            CreateWorkspace(DataX=y_line_x_axis, DataY=y_line_y_values, OutputWorkspace="y_cut")

    def _place_interpolated_rectangles(self):
        """
        Place new rectangles based on those already placed by the user. Only linearly interpolate new positions
        from the center of the drawn rectangles. Only supports 2 rectangles.
        """
        rectangles = self.get_rectangles()
        if len(rectangles) == 1:
            self._place_interpolate_linear()
        elif len(rectangles) == 2:
            self._place_interpolate_linear()
        else:
            logger.warning(
                "Cannot place more regions of interest: current number of regions invalid " "(1 or 2 expected, {} found)".format(
                    len(rectangles)
                )
            )
            return

        self._update_plot_values()

    def _place_interpolate_linear(self):
        """
        Interpolate linearly the positions and shapes of the 2 provided rectangles to place estimated ROI of peaks.
        If only one has been drawn, the origin of the plot is used as the other one.
        """
        rectangles = self.get_rectangles()

        # we don't handle cases apart from these two
        if len(rectangles) != 1 and len(rectangles) != 2:
            return

        def two_same_rectangles(rect: list) -> bool:
            """
            Check if two rectangle patches are the (almost) the same
            @param rect: a pair of rectangle patches
            @return whether those 2 patches are the same within an epsilon
            """
            if len(rect) != 2:
                return False
            rect0, rect1 = rect

            return (
                self.is_almost_equal(rect0.get_x() + rect0.get_width() / 2, rect1.get_x() + rect1.get_width() / 2)
                and self.is_almost_equal(rect0.get_y() + rect0.get_height() / 2, rect1.get_y() + rect1.get_height() / 2)
                and self.is_almost_equal(abs(rect0.get_width()), abs(rect1.get_width()))
                and self.is_almost_equal(abs(rect0.get_height()), abs(rect1.get_height()))
            )

        if len(rectangles) == 1 or two_same_rectangles(rectangles):
            if len(rectangles) != 1:
                logger.notice("The 2 regions of interest are superposed. Proceeding as if there was only one.")

            # there is only one relevant rectangle in this case
            rect_0 = rectangles[0]

            new_height = rect_0.get_height()
            new_width = rect_0.get_width()

            peak = self._find_peak(rectangles[0])
            center_0 = np.array([peak[0], peak[1]])

            # we act as if there was a second rectangle with the same size center around (0, 0)
            center_1 = np.array((0, 0))

        else:
            # there are two rectangles,a nd we use them both to decide where to place the new ones
            rect_0, rect_1 = rectangles
            new_height = (rect_0.get_height() + rect_1.get_height()) / 2
            new_width = (rect_0.get_width() + rect_1.get_width()) / 2
            center_0 = np.array((rect_0.get_x() + rect_0.get_width() / 2, rect_0.get_y() + rect_0.get_height() / 2))
            center_1 = np.array((rect_1.get_x() + rect_1.get_width() / 2, rect_1.get_y() + rect_1.get_height() / 2))

        def move(seed: np.array, offset: np.array, limit=10):
            """
            Starting at seed, place a rectangle every offset
            @param seed: the center of the first rectangle to place
            @param offset: the offset between each rectangle, in both x and y.
            @param limit: the maximum of number of rectangles to place, just as a security against really small offsets
            """
            current_number = 1
            while self.rectangle_fit_on_image(seed, new_width, new_height) and current_number <= limit:
                self._draw_rectangle((seed[0] - new_width / 2, seed[1] - new_height / 2), new_width, new_height)
                seed += offset
                current_number += 1

        first_center = 2 * center_1 - center_0
        move(first_center, center_1 - center_0)

        first_center = 2 * center_0 - center_1
        move(first_center, center_0 - center_1)

    def _place_interpolate_in_q(self):
        """
        THIS METHOD IS CURRENTLY NOT USED
        According to the theory, Qn = nQ1 is the expected relationship. But the scientists insist for linear spacing
        between ROIs, and we don't have data to decide yet, so we are going with their formula for now.
        This stays here nonetheless just in case the data was to show this method is more useful.
        If it is not needed by the end of next cycle (221 or 231), it should be safe to remove this function.

        Interpolate the user-provided rectangle position to determine ROI around the other peaks.
        This method assumes that:
        1) the rectangle provided correspond to the first (non-zero theta) peak
        2) peaks positions verify theta = omega
        3) Qn = nQ1 for all peaks
        """
        rect = self.get_rectangles()[0]
        peak = self._find_peak(rect)

        sin_theta = np.sin(np.deg2rad(peak[0]))
        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        i = 2
        v = np.rad2deg(np.arcsin(i * sin_theta))
        width = rect.get_width()
        height = rect.get_height()

        # there is no ROI at 2theta = 0 because that's the beam and no data can be analysed there.

        # place the ROI symmetric to the user given one.
        if self.rectangle_fit_on_image((-peak[0], peak[1]), width, height):
            self._draw_rectangle((-peak[0] - width / 2, -peak[1] - height / 2), width, height)

        # as long as we can add more ROis on either side of the first ROI, we do
        while xmin < v < xmax or xmin < -v < xmax:
            # since the x axis is 2*theta, omega = theta means y = x/2
            if self.rectangle_fit_on_image((v, v / 2), width, height):
                self._draw_rectangle((v - width / 2, v / 2 - height / 2), width, height)
            if self.rectangle_fit_on_image((-v, -v / 2), width, height):
                self._draw_rectangle((-v - width / 2, -v / 2 - height / 2), width, height)

            i += 1
            v = np.rad2deg(np.arcsin(i * sin_theta))

    def rectangle_fit_on_image(self, center: tuple, width: float, height: float) -> bool:
        """
        Check if the rectangle with center at center fits in the image boundaries.
        @param center: the center of the rectangle
        @param width: the width of the rectangle
        @param height: the height of the rectangle
        @return whether the provided rectangle fits
        """
        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        return (
            xmin <= center[0] + width / 2 <= xmax
            and xmin <= center[0] - height / 2 <= xmax
            and ymin <= center[1] + height / 2 <= ymax
            and ymin <= center[1] - height / 2 <= ymax
        )

    def _extract_peaks(self):
        """
        Extract the peaks to a table workspace
        """
        table_ws = CreateEmptyTableWorkspace(OutputWorkspace="peaks")
        table_ws.addColumn("int", "Peak")
        table_ws.addColumn("float", "2Theta")
        table_ws.addColumn("float", "2ThetaWidth")
        table_ws.addColumn("float", "Omega")
        table_ws.addColumn("float", "OmegaWidth")

        additional_data = self._manager.additional_peaks_info(self.get_rectangles())

        # adding new necessary columns
        for key in additional_data.keys():
            if additional_data[key]:
                table_ws.addColumn("float", key)

        for index, rect in enumerate(self.get_rectangles()):
            peak = self._find_peak(rect)
            self._show_peak(rect, peak)
            peak_dict = {"Peak": index, "2Theta": peak[0], "2ThetaWidth": peak[2], "Omega": peak[1], "OmegaWidth": peak[3]}

            for key in additional_data.keys():
                peak_dict[key] = additional_data[key][index]

            table_ws.addRow(peak_dict)

    @staticmethod
    def _fit_projection(axis: np.ndarray, data: np.ndarray) -> Tuple[float, float]:
        """
        Fits the provided data with a single gaussian and flat background and returns fitted peak centre value and width.

        Args:
        axis: Array containing position information related to intensities stored in data
        data: Array containing intensities to be fitted

        Returns: Fitted peak centre and width
        """
        ws = CreateWorkspace(DataX=axis, DataY=data, DataE=np.sqrt(data), StoreInADS=False)

        # prepare inputs for fitting function and constraints
        average_count = np.sum(data) / len(data)
        pos_max = axis[np.argmax(data)]
        max_value = np.max(data) - average_count
        axis_range = axis[-1] - axis[0]
        gauss_width = 0.2 * axis_range

        # define fitting function: flat background + one gaussian and constraints to keep the found peak in the ROI
        fit_function = "name=FlatBackground, A0={0}; name=Gaussian, PeakCentre={1}, Height={2}, Sigma={3}"
        fit_constraints = "f1.Height > {0}, f1.Sigma < {1}, {2} < f1.PeakCentre < {3}"

        # try to fit the output
        try:
            fit_output = Fit(
                Function=fit_function.format(average_count, pos_max, max_value, gauss_width),
                Constraints=fit_constraints.format(str(0.5 * max_value), str(0.5 * axis_range), axis[0], axis[-1]),
                InputWorkspace=ws,
                CreateOutput=True,
                IgnoreInvalidData=True,
                StoreInADS=False,
            )
        except (RuntimeError, ValueError):
            # if fit fails, default the peak centre to the rectangle centre and width to rectangle's width
            peak_pos = axis[0] + 0.5 * axis_range
            peak_width = 0.5 * axis_range
        else:
            peak_pos = fit_output.OutputParameters.row(2)["Value"]
            peak_width = fit_output.OutputParameters.row(3)["Value"]

        # if the fit returned the peak position outside of ROI, place the peak at the ROI's centre
        if not (axis[0] < peak_pos < axis[-1]):
            peak_pos = axis[0] + 0.5 * axis_range
            peak_width = 0.5 * axis_range

        return peak_pos, peak_width

    def _find_peak(self, rect: Rectangle) -> Tuple[float, float, float, float]:
        """
        Find the peak by fitting x and y projections of the rectangle
        @param rect: the relevant rectangle
        @return the peak x, y position and its x, y widths
        """
        xmin, xmax, ymin, ymax = self.plotter.image.get_extent()

        arr = self.plotter.image.get_array()

        x_step = (xmax - xmin) / arr.shape[1]
        y_step = (ymax - ymin) / arr.shape[0]
        # calculating the step this way can introduce a slight deviation on the peak position, because for convenience
        # we are then working on the matplotlib array of the data instead of the data itself

        x0, y0 = rect.get_xy()
        x1 = x0 + rect.get_width()
        y1 = y0 + rect.get_height()

        # we set x0, x1 with xmin <= x0 <= x1 <= xmax
        x0, x1 = max(min(x0, x1, xmax), xmin), min(max(x0, x1, xmin), xmax)
        y0, y1 = max(min(y0, y1, ymax), ymin), min(max(y0, y1, ymin), ymax)

        # find the indices corresponding to the position in the array
        x0_ind = int(np.floor((x0 - xmin) / x_step))
        y0_ind = int(np.floor((y0 - ymin) / y_step))

        x1_ind = int(np.ceil((x1 - xmin) / x_step))
        y1_ind = int(np.ceil((y1 - ymin) / y_step))

        slice_cut = arr[y0_ind:y1_ind, x0_ind:x1_ind]

        total_sum = slice_cut.sum()
        # if there is no counts or the sum is masked, we return the middle point
        if total_sum == 0 or np.ma.is_masked(total_sum):
            return (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2

        x_cut = np.sum(slice_cut, axis=0)
        x_axis = np.linspace(x0, x1, len(slice_cut[0]))
        y_cut = np.sum(slice_cut, axis=1)
        y_axis = np.linspace(y0, y1, len(slice_cut))

        x_peak_pos, x_peak_width = self._fit_projection(x_axis, x_cut)
        y_peak_pos, y_peak_width = self._fit_projection(y_axis, y_cut)

        return x_peak_pos, y_peak_pos, x_peak_width, y_peak_width

    def _show_peak(self, rect: Rectangle, peak: Tuple[float, float, float, float]):
        """
        Display the peak on the figure, replacing previous one if needed
        @param rect: the rectangle whose peak is shown
        @param peak: the position of the peak to show
        """
        indexes = self._manager.find_controllers(*get_opposing_corners(rect.get_xy(), rect.get_width(), rect.get_height()))
        peak_pos = (peak[0], peak[1])
        for index in indexes:
            plot = self.plotter.image_axes.plot(*peak_pos, marker="+", color="r")[0]
            controller = self._manager.rectangles[index][0]
            controller.set_peak_plot(plot)

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
        self._selector.set_visible(False)
        self._update_plot_values()

    def handle_key(self, key: str):
        """
        Handle user key inputs, if they are supported keys.
        @param key: the character pressed by the user
        """
        if key not in self.SELECTION_KEYS:
            return

        if key in ("c", "x", "y"):
            self._extract_projections(extract_x=key in ("c", "x"), extract_y=key in ("c", "y"))
        if key == "f":
            self._place_interpolated_rectangles()
        if key == "p":
            self._extract_peaks()
        if key == "delete":
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
    def is_almost_equal(cls, a: float, b: float) -> bool:
        """
        Check if 2 numbers are within epsilon of relative distance
        """
        return a == b == 0 or abs(a - b) / max(abs(a), abs(b)) < cls.EPSILON


class MultipleRectangleSelectionLinePlotSignals(QWidget):
    sig_current_updated = Signal()


def get_opposing_corners(point, width, height):
    return point[0], point[1], point[0] + width, point[1] + height
