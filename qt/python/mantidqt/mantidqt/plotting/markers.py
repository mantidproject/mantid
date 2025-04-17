# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt, QObject, Signal
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication

from matplotlib.path import Path
from matplotlib.patches import PathPatch

MARKER_SENSITIVITY = 5


class HorizontalMarker(QObject):
    """
    An interactive marker displayed as a horizontal line.
    """

    y_moved = Signal(float)

    def __init__(self, canvas, color, y, x0=None, x1=None, line_width=1.0, picker_width=5, line_style="-", move_cursor=None, axis=None):
        """
        Init the marker.
        :param canvas: A MPL canvas.
        :param color: An MPL colour value
        :param y: The y coordinate (data) of the marker.
        :param x0: The x coordinate (data) of the left end of the marker. Default is None which means dynamically
            set it to the current maximum x value displayed.
        :param x1: The x coordinate (data) of the right end of the marker. Default is None which means dynamically
            set it to the current minimum x value displayed.
        :param line_width: The line width (pixels).
        :param picker_width: The picker sensitivity (pixels).
        :param line_style: An MPL line style value.
        """
        super(HorizontalMarker, self).__init__()
        self.canvas = canvas
        if axis is None:
            self.axis = canvas.figure.get_axes()[0]
        else:
            self.axis = axis
        self.y = y
        self.x0 = x0
        self.x1 = x1
        x0, x1 = self._get_x0_x1()
        path = Path([(x0, y), (x1, y)], [Path.MOVETO, Path.LINETO])
        self.patch = PathPatch(
            path, facecolor="None", edgecolor=color, picker=picker_width, linewidth=line_width, linestyle=line_style, animated=True
        )
        self.axis.add_artist_correctly(self.patch)
        self.axis.interactive_markers.append(self.patch)
        self.is_moving = False
        self.move_cursor = move_cursor

    def _get_x0_x1(self):
        """
        Calculate the current x coordinates of the line ends.
        :return: Tuple x0, x1.
        """
        if self.x0 is None or self.x1 is None:
            x0, x1 = self.axis.get_xlim()
        if self.x0 is not None:
            x0 = self.x0
        if self.x1 is not None:
            x1 = self.x1
        return x0, x1

    def remove(self):
        """
        Remove this marker from the canvas.
        """
        try:
            self.patch.remove()
        except ValueError:
            pass

    def redraw(self):
        """
        Redraw this marker.
        """
        x0, x1 = self._get_x0_x1()
        vertices = self.patch.get_path().vertices
        vertices[0] = x0, self.y
        vertices[1] = x1, self.y
        self.axis.draw_artist(self.patch)
        self.redraw_legend()

    def redraw_legend(self):
        """
        After we redraw the marker, the marker will appear above the legend
        hence we'll have to redraw the legend to place it back ontop
        """
        if self.axis.get_legend():
            self.axis.draw_artist(self.axis.get_legend())

    def set_visible(self, visible):
        self.patch.set_visible(visible)

    def set_color(self, color):
        """
        Set the colour of the marker
        :param color: The color to set the marker to.
        """
        self.patch.set_edgecolor(color)

    def get_position(self):
        """
        Get the y coordinate in axes coords.
        :return: y in axes coords
        """
        return self.y

    def set_position(self, y):
        """
        Set the y position of the marker.
        :param y: An y axis coordinate.
        """
        self.y = y
        self.y_moved.emit(y)

    def get_y_in_pixels(self):
        """
        Returns the y coordinate in screen pixels.
        :return: y in pixels
        """
        _, y_pixels = self.patch.get_transform().transform((0, self.y))
        return y_pixels

    def is_above(self, x, y):
        """
        Check if a mouse positioned at (x, y) is over this marker.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True or False.
        """
        _, y_pixels = self.patch.get_transform().transform((x, y))

        if self.x0 is not None and x < self.x0:
            return False
        if self.x1 is not None and x > self.x1:
            return False
        return abs(self.get_y_in_pixels() - y_pixels) < MARKER_SENSITIVITY

    def mouse_move_start(self, x, y):
        """
        Start moving this marker if (x, y) is above it. Ignore otherwise.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :param pixels: True if the coordinates are already in pixels.
        """
        self.is_moving = self.is_above(x, y)

    def mouse_move_stop(self):
        """
        Stop moving.
        """
        self.is_moving = False

    def mouse_move(self, x, y):
        """
        Move this marker to a new position if movement had been started earlier by a call to mouse_move_start(x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True if moved or False if stayed at the old position.
        """
        if self.is_moving and y is not None and x is not None:
            self.set_position(y)
            return True
        return False

    def is_marker_moving(self):
        """
        Returns true if the marker is being moved
        :return: True if the marker is being moved.
        """
        return self.is_moving

    def override_cursor(self, x, y):
        """
        Get the override cursor for mouse position (x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: QCursor or None.
        """
        if self.x0 is not None and x < self.x0:
            return None
        if self.x1 is not None and x > self.x1:
            return None
        if self.is_moving or self.is_above(x, y):
            return self.move_cursor if self.move_cursor is not None else QCursor(Qt.SizeVerCursor)
        return None

    def set_move_cursor(self, cursor, x_pos, y_pos):
        """Set the style of the cursor to use when the marker is moving"""
        if cursor is not None:
            cursor = QCursor(cursor)
        self.move_cursor = cursor
        self.override_cursor(x_pos, y_pos)


class VerticalMarker(QObject):
    """
    An interactive marker displayed as a vertical line.
    """

    x_moved = Signal(float)

    def __init__(self, canvas, color, x, y0=None, y1=None, line_width=1.0, picker_width=5, line_style="-", move_cursor=None, axis=None):
        """
        Init the marker.
        :param canvas: A MPL canvas.
        :param color: An MPL colour value
        :param x: The x coordinate (data) of the marker.
        :param y0: The y coordinate (data) of the bottom end of the marker. Default is None which means dynamically
            set it to the current lowest y value displayed.
        :param y1: The y coordinate (data) of the top end of the marker. Default is None which means dynamically
            set it to the current highest y value displayed.
        :param line_width: The line width (pixels).
        :param picker_width: The picker sensitivity (pixels).
        :param line_style: An MPL line style value.
        """
        super(VerticalMarker, self).__init__()
        self.canvas = canvas
        if axis is None:
            self.axis = canvas.figure.get_axes()[0]
        else:
            self.axis = axis
        self.x = x
        self.y0 = y0
        self.y1 = y1
        y0, y1 = self._get_y0_y1()
        path = Path([(x, y0), (x, y1)], [Path.MOVETO, Path.LINETO])
        self.patch = PathPatch(
            path, facecolor="None", edgecolor=color, picker=picker_width, linewidth=line_width, linestyle=line_style, animated=True
        )
        self.axis.add_artist_correctly(self.patch)
        self.axis.interactive_markers.append(self.patch)
        self.is_moving = False
        self.move_cursor = move_cursor

    def _get_y0_y1(self):
        """
        Calculate the current y coordinates of the line ends.
        :return: Tuple y0, y1.
        """
        if self.y0 is None or self.y1 is None:
            y0, y1 = self.axis.get_ylim()
        if self.y0 is not None:
            y0 = self.y0
        if self.y1 is not None:
            y1 = self.y1
        return y0, y1

    def remove(self):
        """
        Remove this marker from the canvas.
        """
        self.patch.remove()

    def redraw(self):
        """
        Redraw this marker.
        """
        y0, y1 = self._get_y0_y1()
        vertices = self.patch.get_path().vertices
        vertices[0] = self.x, y0
        vertices[1] = self.x, y1
        self.axis.draw_artist(self.patch)
        self.redraw_legend()

    def redraw_legend(self):
        """
        After we redraw the marker, the marker will appear above the legend
        hence we'll have to redraw the legend to place it back ontop
        """
        if self.axis.get_legend():
            self.axis.draw_artist(self.axis.get_legend())

    def set_visible(self, visible):
        self.patch.set_visible(visible)

    def set_color(self, color):
        """
        Set the colour of the marker
        :param color: The color to set the marker to.
        """
        self.patch.set_edgecolor(color)

    def get_position(self):
        """
        Get the x coordinate in axes coords.
        :return: x in axes coords
        """
        return self.x

    def set_position(self, x):
        """
        Set the x position of the marker.
        :param x: An x axis coordinate.
        """
        self.x = x
        self.x_moved.emit(x)

    def get_x_in_pixels(self):
        """
        Get the x coordinate in screen pixels.
        :return: x in pixels
        """
        x_pixels, _ = self.patch.get_transform().transform((self.x, 0))
        return x_pixels

    def is_above(self, x, y):
        """
        Check if a mouse positioned at (x, y) is over this marker.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True or False.
        """
        x_pixels, _ = self.patch.get_transform().transform((x, y))

        if self.y0 is not None and y < self.y0:
            return False
        if self.y1 is not None and y > self.y1:
            return False
        return abs(self.get_x_in_pixels() - x_pixels) < MARKER_SENSITIVITY

    def mouse_move_start(self, x, y):
        """
        Start moving this marker if (x, y) is above it. Ignore otherwise.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        """
        self.is_moving = self.is_above(x, y)

    def mouse_move_stop(self):
        """
        Stop moving.
        """
        self.is_moving = False

    def mouse_move(self, x, y=None):
        """
        Move this marker to a new position if movement had been started earlier by a call to mouse_move_start(x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True if moved or False if stayed at the old position.
        """
        if self.is_moving and x is not None:
            self.set_position(x)
            return True
        return False

    def is_marker_moving(self):
        """
        Returns true if the marker is being moved
        :return: True if the marker is being moved.
        """
        return self.is_moving

    def get_cursor_at_y(self, y):
        """
        Get an override cursor for an y coordinate given that the x == self.x
        :param y: A y coordinate.
        :return: QCursor or None.
        """
        return self.move_cursor if self.move_cursor is not None else QCursor(Qt.SizeHorCursor)

    def override_cursor(self, x, y):
        """
        Get the override cursor for mouse position (x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: QCursor or None.
        """
        if self.y0 is not None and y < self.y0:
            return None
        if self.y1 is not None and y > self.y1:
            return None
        if self.is_moving or self.is_above(x, y):
            return self.get_cursor_at_y(y)
        return None

    def set_move_cursor(self, cursor, x_pos, y_pos):
        """Set the style of the cursor to use when the marker is moving"""
        if cursor is not None:
            cursor = QCursor(cursor)
        self.move_cursor = cursor
        self.override_cursor(x_pos, y_pos)


class CentreMarker(VerticalMarker):
    """
    A marker for a peak centre.
    """

    selected_color = "red"
    deselected_color = "grey"

    def __init__(self, canvas, x, y0, y1):
        """
        Init the marker.
        :param canvas: A MPL canvas.
        :param x: The x coordinate (data) of the marker.
        :param y0: The y coordinate (data) of the bottom end of the marker.
        :param y1: The y coordinate (data) of the top end of the marker.
        """
        VerticalMarker.__init__(self, canvas, self.deselected_color, x, y0, y1)
        self.is_at_top = False

    def _is_at_top(self, y):
        """
        Check if the mouse at position (self.x, y) points to the top end of the marker. There is some tolerance.
        :param y: A y coordinate.
        :return: True or False.
        """
        _, y1_pixels = self.patch.get_transform().transform((0, self.y1))
        _, y_pixels = self.patch.get_transform().transform((0, y))
        return abs(y1_pixels - y_pixels) < 10

    def get_cursor_at_y(self, y):
        """
        Get an override cursor for an y coordinate given that the x == self.x
        :param y: A y coordinate.
        :return: QCursor or None.
        """
        is_at_top = self.is_at_top if self.is_moving else self._is_at_top(y)
        return QCursor(Qt.SizeAllCursor) if is_at_top else VerticalMarker.get_cursor_at_y(self, y)

    def mouse_move_start(self, x, y):
        """
        Start moving marker.
        """
        VerticalMarker.mouse_move_start(self, x, y)
        self.is_at_top = self._is_at_top(y)

    def mouse_move_stop(self):
        """
        Stop moving marker.
        """
        VerticalMarker.mouse_move_stop(self)
        self.is_at_top = False

    def mouse_move(self, x, y=None):
        """
        Move marker.
        :return: True if it was actually moved.
        """
        if not self.is_moving:
            return False
        if self.is_at_top:
            self.y1 = y
        self.x = x
        return True

    def height(self):
        """
        Get the height of the marker (== peak height).
        """
        return self.y1 - self.y0

    def set_height(self, height):
        """
        Set height - change the top end y coordinate only.
        :param height: A new height.
        """
        self.y1 = self.y0 + height

    def select(self):
        """
        Mark as selected.
        """
        self.patch.set_edgecolor(self.selected_color)

    def deselect(self):
        """
        Remove selection.
        """
        self.patch.set_edgecolor(self.deselected_color)


class WidthMarker(VerticalMarker):
    """
    A peak width marker (left or right).
    """

    def __init__(self, canvas, x):
        VerticalMarker.__init__(self, canvas, "red", x, line_style="--")


class PeakMarker(QObject):
    """
    A peak marker. Consists of a CentreMarker and two WidthMarkers placed at half width at half peak maximum on either
    side of the centre line.
    """

    peak_moved = Signal(int, float, float)
    fwhm_changed = Signal(int, float)

    def __init__(self, canvas, peak_id, x, height, fwhm, background):
        """
        Init the marker.
        :param canvas: The MPL canvas.
        :param peak_id: A unique peak id.
        :param x: Peak centre.
        :param y_top: Peak's top.
        :param y_bottom: Peaks bottom (background level).
        :param fwhm: A full width at half maximum.
        """
        super(PeakMarker, self).__init__()
        self.peak_id = peak_id
        self.centre_marker = CentreMarker(canvas, x, y0=background, y1=background + height)
        self.left_width = WidthMarker(canvas, x - fwhm / 2)
        self.right_width = WidthMarker(canvas, x + fwhm / 2)
        self.is_selected = False
        # True if the mouse is currently hovering over the centre marker
        self._centre_hover = False

    def redraw(self):
        """
        Redraw the marker.
        """
        self.centre_marker.redraw()
        if self.is_selected:
            self.left_width.redraw()
            self.right_width.redraw()

    def override_cursor(self, x, y):
        """
        Get the override cursor for mouse position (x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: QCursor or None.
        """
        cursor = self.centre_marker.override_cursor(x, y)
        if self.is_selected:
            if cursor is None:
                cursor = self.left_width.override_cursor(x, y)
            if cursor is None:
                cursor = self.right_width.override_cursor(x, y)
        return cursor

    def mouse_move_start(self, x, y):
        """
        Start moving an element of this marker if it's under the cursor.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        """
        self.centre_marker.mouse_move_start(x, y)
        if self.centre_marker.is_moving:
            self.left_width.is_moving = True
            self.right_width.is_moving = True
        elif self.is_selected:
            self.left_width.mouse_move_start(x, y)
            self.right_width.mouse_move_start(x, y)

        if self.centre_marker.is_moving or self.left_width.is_moving or self.right_width.is_moving:
            QApplication.setOverrideCursor(Qt.SizeHorCursor)

    def mouse_move_stop(self):
        """
        Stop moving.
        """
        if self.centre_marker.is_moving:
            self.left_width.is_moving = False
            self.right_width.is_moving = False
        else:
            self.left_width.mouse_move_stop()
            self.right_width.mouse_move_stop()
        self.centre_marker.mouse_move_stop()
        QApplication.restoreOverrideCursor()

    def mouse_move(self, x, y):
        """
        Move an element of this marker if it's started moving.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        """
        moved = self.centre_marker.mouse_move(x, y)
        if moved:
            dx = (self.right_width.x - self.left_width.x) / 2
            self.left_width.mouse_move(x - dx, y)
            self.right_width.mouse_move(x + dx, y)
            self.peak_moved.emit(self.peak_id, x, self.height())
        elif self.is_selected:
            moved = self.left_width.mouse_move(x, y)
            if moved:
                self.right_width.x = 2 * self.centre_marker.x - x
            else:
                moved = self.right_width.mouse_move(x, y)
                if moved:
                    self.left_width.x = 2 * self.centre_marker.x - x
            if self.left_width.x > self.right_width.x:
                tmp = self.right_width.x
                self.right_width.x = self.left_width.x
                self.left_width.x = tmp
            if moved:
                self.fwhm_changed.emit(self.peak_id, self.fwhm())
        return moved

    def mouse_move_hover(self, x: float, y: float) -> None:
        """
        Check if the provided coordinate is above the centre marker.
        """
        is_above_centre = self.centre_marker.is_above(x, y)
        if not self._centre_hover and is_above_centre:
            QApplication.setOverrideCursor(Qt.SizeHorCursor)
        elif self._centre_hover and not is_above_centre:
            QApplication.restoreOverrideCursor()
        self._centre_hover = is_above_centre

    def is_moving(self):
        """
        Check if this marker is moving as a whole.
        """
        return self.centre_marker.is_moving

    def centre(self):
        """
        Get peak centre.
        """
        return self.centre_marker.x

    def height(self):
        """
        Get peak height.
        """
        return self.centre_marker.y1 - self.centre_marker.y0

    def fwhm(self):
        """
        Get peak FWHM.
        """
        return self.right_width.x - self.left_width.x

    def peak_properties(self):
        """
        Get the centre, height and fwhm of the peak.
        """
        return self.centre(), self.height(), self.fwhm()

    def update_peak(self, centre, height, fwhm, background=0.0):
        """
        Update this marker.
        :param centre: A new peak centre.
        :param height: A new peak height.
        :param fwhm: A new peak FWHM.
        :param background: The background to place the peak on top of.
        """
        self.centre_marker.x = centre
        self.centre_marker.y0 = background
        self.centre_marker.set_height(height)
        dx = fwhm / 2
        self.left_width.x = centre - dx
        self.right_width.x = centre + dx

    def select(self):
        """
        Select this peak.
        """
        self.centre_marker.select()
        self.is_selected = True

    def deselect(self):
        """
        Deselect this peak.
        """
        self.centre_marker.deselect()
        self.is_selected = False

    def remove(self):
        """
        Remove this peak marker from the canvas.
        """
        self.centre_marker.remove()
        self.left_width.remove()
        self.right_width.remove()

    def set_visible(self, visible):
        self.centre_marker.set_visible(visible)
        self.left_width.set_visible(visible)
        self.right_width.set_visible(visible)


class SingleMarker(QObject):
    """
    A marker used to mark out a vertical or horizontal line on a plot.
    """

    def __init__(self, canvas, color, position, lower_bound, upper_bound, marker_type="XSingle", line_style="-", name=None, axis=None):
        """
        Init the marker.
        :param canvas: The MPL canvas.
        :param color: An MPL colour value
        :param position: The axes coordinate of the marker.
        :param lower_bound: The axes coordinate of the lower bound.
        :param upper_bound: The axes coordinate of the upper bound.
        :param marker_type: Whether the SingleMarker is vertical or horizontal.
        :param line_style: An MPL line style value.
        """
        super(SingleMarker, self).__init__()
        self.lower_bound = lower_bound
        self.upper_bound = upper_bound
        self.marker_type = marker_type
        self.canvas = canvas
        self.annotations = {}
        self.name = name
        self.label_visible = True
        self.style = line_style
        self.color = color
        self.draggable = True
        self.axis = axis
        if self.marker_type == "XSingle":
            self.marker = VerticalMarker(canvas, color, position, line_style=line_style, axis=self.axis)
        elif self.marker_type == "YSingle":
            self.marker = HorizontalMarker(canvas, color, position, line_style=line_style, axis=self.axis)
        else:
            raise RuntimeError("Incorrect SingleMarker type provided. Types are XSingle or YSingle.")

        # Set default label position
        if self.marker_type == "YSingle":
            self.label_x_offset = 0.98
            self.label_y_offset = 0.005
        else:
            self.label_x_offset = 0.0
            self.label_y_offset = 0.95

    def set_label_visible(self, is_visible):
        """Allows for labels to be hidden/shown"""
        self.remove_all_annotations()
        self.label_visible = is_visible
        self.add_all_annotations()

    def set_label_position(self, x_offset, y_offset):
        """
        Updates the position of a label (coordinates are relative, i.e. 0 <= pos <= 1)
        """
        self.remove_all_annotations()
        old_x = self.label_x_offset
        old_y = self.label_y_offset
        self.label_x_offset = x_offset
        self.label_y_offset = y_offset
        try:
            self.add_all_annotations()
        except RuntimeError as err:
            self.label_x_offset = old_x
            self.label_y_offset = old_y
            self.add_all_annotations()
            raise RuntimeError(str(err))

    def redraw(self):
        """
        Redraw the single marker.
        """
        self.marker.redraw()

    def remove(self):
        """
        Remove this marker from the canvas.
        """
        self.marker.remove()

    def set_style(self, style):
        """
        Change the marker style.
        :param style: a valid matplotlib style (e.g. 'solid', 'dotted'...)
        """
        position = self.marker.get_position()
        self.style = style
        if self.marker_type == "XSingle":
            self.marker.remove()
            self.marker = VerticalMarker(self.canvas, self.color, position, line_style=style, axis=self.axis)
        elif self.marker_type == "YSingle":
            self.marker.remove()
            self.marker = HorizontalMarker(self.canvas, self.color, position, line_style=style, axis=self.axis)
        else:
            raise RuntimeError("Incorrect SingleMarker type provided. Types are XSingle or YSingle.")

    def set_color(self, color):
        """
        Set the colour of the marker.
        """
        self.marker.set_color(color)
        self.color = color

    def set_position(self, position):
        """
        Sets the positions of the marker's
        :param position: The position of the marker in axes coords.
        :return True if the value was changed.
        """
        if self.upper_bound >= position >= self.lower_bound:
            self.remove_name()
            self.marker.set_position(position)
            self.add_name()
            self.redraw()
            return True
        return False

    def get_position(self):
        """
        Gets the position of the marker
        :return the position of the marker.
        """
        return self.marker.get_position()

    def set_bounds(self, minimum, maximum):
        """
        Sets the bounds within which the marker is allowed to be moved.
        :param minimum: The lower end of the marker position.
        :param maximum: The higher end of the marker position.
        """
        self.set_lower_bound(minimum)
        self.set_upper_bound(maximum)

    def set_lower_bound(self, minimum):
        """
        Sets the minimum bound for the marker.
        :param minimum: The minimum bound for the marker.
        """
        self.lower_bound = minimum
        if self.lower_bound > self.get_position():
            self.set_position(minimum)

    def set_upper_bound(self, maximum):
        """
        Sets the maximum bound for the marker.
        :param maximum: The maximum bound for the marker.
        """
        self.upper_bound = maximum
        if self.upper_bound < self.get_position():
            self.set_position(maximum)

    def get_lower_bound(self):
        """
        Gets the minimum bound for the marker.
        :return: The minimum bound for the marker.
        """
        return self.lower_bound

    def get_upper_bound(self):
        """
        Gets the maximum bound for the marker.
        :return: The maximum bound for the marker.
        """
        return self.upper_bound

    def is_inside_bounds(self, x, y):
        """
        Determines if the axis coords are within the bounds specified.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return True if the axes coordinate is within the bounds, and the new position for the marker.
        """
        position_offset = MARKER_SENSITIVITY if not self.is_marker_moving() else 0
        position = x if self.marker_type == "XSingle" else y
        if position is not None:
            if position > self.upper_bound + position_offset:
                return False, self.upper_bound
            elif position < self.lower_bound - position_offset:
                return False, self.lower_bound
        return True, position

    def add_annotate(self, text):
        """
        Add an annotation near the marker and redraw the canvas.
        :param text: annotation text
        """
        if text is None:
            return

        if not self.label_visible:
            self.annotations[self.name] = ""
            return

        marker_in_scope = True
        ax = self.marker.axis
        x_lower, x_upper = ax.get_xlim()
        y_lower, y_upper = ax.get_ylim()

        if self.marker_type == "YSingle":
            # Horizontal marker
            x_pos = x_upper - (x_upper - x_lower) * (1 - self.label_x_offset)
            y_pos = self.marker.y
            rotation = 0
            if not y_lower <= y_pos <= y_upper:
                marker_in_scope = False
            horizontal = "right"
            vertical = "bottom"
        else:
            # Vertical marker
            x_pos = self.marker.x
            y_pos = y_upper - (y_upper - y_lower) * (1 - self.label_y_offset)
            rotation = -90
            if not x_lower <= x_pos <= x_upper:
                marker_in_scope = False
            horizontal = "left"
            vertical = "top"

        if marker_in_scope:
            self.annotations[text] = ax.annotate(text, xy=(x_pos, y_pos), xycoords="data", ha=horizontal, va=vertical, rotation=rotation)
        else:
            self.annotations[text] = None

        self.canvas.draw()

    def remove_annotate(self, label):
        """
        Remove the label from the canvas
        """
        if label not in self.annotations or self.annotations[label] is None:
            return

        try:
            self.annotations[label].remove()
        except:
            return

    def add_all_annotations(self):
        """Add all previously added annotations"""
        for label in self.annotations:
            self.add_annotate(label)

    def remove_all_annotations(self):
        """Remove all annotations from plot"""
        for label in self.annotations:
            self.remove_annotate(label)

    def add_name(self):
        """Only add the annotation for the name of the marker"""
        self.add_annotate(self.name)

    def remove_name(self):
        """Remove the annotation for the name of the marker"""
        self.remove_annotate(self.name)

    def set_name(self, name):
        """Update the name of the marker by deleting the old one first"""
        self.remove_name()
        if self.name in self.annotations:
            del self.annotations[self.name]
        self.name = name
        self.add_name()

    def mouse_move_start(self, x, y):
        """
        Start moving this marker if (x, y) is above it. Ignore otherwise.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        """
        inside_bounds, _ = self.is_inside_bounds(x, y)
        if self.marker.is_above(x, y) and inside_bounds and self.draggable:
            self.marker.mouse_move_start(x, y)
            QApplication.setOverrideCursor(self.marker.override_cursor(x, y))

        if self.is_marker_moving():
            self.remove_name()

    def mouse_move(self, x, y=None):
        """
        Move this marker to a new position if movement had been started earlier by a call to mouse_move_start(x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True if moved or False if stayed at the old position.
        """
        marker_moving = self.marker.is_marker_moving()
        inside_bounds, new_position = self.is_inside_bounds(x, y)
        if marker_moving and inside_bounds:
            return self.marker.mouse_move(x, y)
        elif marker_moving:
            if self.marker_type == "XSingle":
                return self.marker.mouse_move(new_position, y)
            return self.marker.mouse_move(x, new_position)
        return False

    def mouse_move_stop(self):
        """
        Stop moving.
        """
        self.marker.mouse_move_stop()
        QApplication.restoreOverrideCursor()

    def is_marker_moving(self):
        """
        Returns true if the marker is being moved
        :return: True if the marker is being moved.
        """
        return self.marker.is_marker_moving()

    def is_above(self, x, y):
        """Check if the cursor is above the marker"""
        if self.marker.is_above(x, y):
            return True
        else:
            return False

    def relative(self, value, lower, upper):
        if not lower <= value <= upper:
            return 0.0

        return (value - lower) / (upper - lower)

    def set_move_cursor(self, cursor, x_pos, y_pos):
        """
        Set the style of cursor to be used while moving the marker
        """
        self.marker.set_move_cursor(cursor, x_pos, y_pos)

    def set_visible(self, visible):
        self.marker.patch.set_visible(visible)


class RangeMarker(QObject):
    """
    A marker used to mark out a vertical or horizontal range using two markers which correspond to a minimum and
    maximum of that range.
    """

    range_changed = Signal(list)

    def __init__(self, canvas, color, minimum, maximum, range_type="XMinMax", line_style="-"):
        """
        Init the marker.
        :param canvas: The MPL canvas.
        :param color: An MPL colour value
        :param minimum: The axes coordinate of the minimum marker.
        :param maximum: The axes coordinate of the maximum marker.
        :param range_type: Whether the RangeMarker is used to select an x or y range (XMinMax or YMinMax).
        :param line_style: An MPL line style value.
        """
        super(RangeMarker, self).__init__()
        self.range_type = range_type
        single_marker_type = "XSingle" if self.range_type == "XMinMax" else "YSingle"
        self.min_marker = SingleMarker(canvas, color, minimum, minimum, maximum, single_marker_type, line_style=line_style)
        self.max_marker = SingleMarker(canvas, color, maximum, minimum, maximum, single_marker_type, line_style=line_style)

    def set_visible(self, visible):
        self.min_marker.set_visible(visible)
        self.max_marker.set_visible(visible)

    def redraw(self):
        """
        Redraw the range marker.
        """
        self.min_marker.redraw()
        self.max_marker.redraw()

    def remove(self):
        """
        Remove this range marker from the canvas.
        """
        self.min_marker.remove()
        self.max_marker.remove()

    def set_color(self, color):
        """
        Set the colour of the range marker
        """
        self.min_marker.set_color(color)
        self.max_marker.set_color(color)
        self.redraw()

    def set_bounds(self, minimum, maximum):
        """
        Sets the bounds within which the range marker is allowed to move.
        :param minimum: The lower bound for the markers position.
        :param maximum: The higher bound for the markers position.
        """
        self.set_lower_bound(minimum)
        self.set_upper_bound(maximum)

    def get_bounds(self):
        """
        Gets the bounds within which the range marker is allowed to move. Note that the lower and upper bound for the
        min and max markers is the same.
        :return: the bounds within which the range marker is allowed to move.
        """
        return sorted([self.min_marker.get_lower_bound(), self.min_marker.get_upper_bound()])

    def set_lower_bound(self, minimum):
        """
        Sets the minimum bound for the range marker.
        :param minimum: The minimum bound for the range marker.
        """
        self.min_marker.set_lower_bound(minimum)
        self.max_marker.set_lower_bound(minimum)

    def set_upper_bound(self, maximum):
        """
        Sets the maximum bound for the range marker.
        :param maximum: The maximum bound for the range marker.
        """
        self.min_marker.set_upper_bound(maximum)
        self.max_marker.set_upper_bound(maximum)

    def set_range(self, minimum, maximum):
        """
        Sets the positions of the marker's
        :param minimum: The minimum of the range.
        :param maximum: The maximum of the range.
        """
        self.min_marker.set_position(minimum)
        self.max_marker.set_position(maximum)
        self.redraw()

    def get_range(self):
        """
        Gets the positions of the min and max of the range
        :return the minimum and maximum of the range.
        """
        return sorted([self.min_marker.get_position(), self.max_marker.get_position()])

    def set_minimum(self, minimum):
        """
        Sets the minimum for the range.
        :param minimum: The minimum of the range.
        """
        maximum = max([self.min_marker.get_position(), self.max_marker.get_position()])
        self.set_range(minimum, maximum)

    def set_maximum(self, maximum):
        """
        Sets the maximum for the range.
        :param maximum: The maximum of the range.
        """
        minimum = min([self.min_marker.get_position(), self.max_marker.get_position()])
        self.set_range(minimum, maximum)

    def get_minimum(self):
        """
        Gets the minimum of the range.
        :return the minimum of the range.
        """
        return min([self.min_marker.get_position(), self.max_marker.get_position()])

    def get_maximum(self):
        """
        Gets the maximum of the range.
        :return the maximum of the range.
        """
        return max([self.min_marker.get_position(), self.max_marker.get_position()])

    def mouse_move_start(self, x, y):
        """
        Start moving this marker if (x, y) is above it. Ignore otherwise.
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        """
        self.min_marker.mouse_move_start(x, y)
        self.max_marker.mouse_move_start(x, y)

    def mouse_move(self, x, y=None):
        """
        Move this marker to a new position if movement had been started earlier by a call to mouse_move_start(x, y)
        :param x: An x mouse coordinate.
        :param y: An y mouse coordinate.
        :return: True if moved or False if stayed at the old position.
        """
        moved = False
        if self.min_marker.is_marker_moving():
            moved = self.min_marker.mouse_move(x, y)
        elif self.max_marker.is_marker_moving():
            moved = self.max_marker.mouse_move(x, y)

        if moved:
            self.range_changed.emit(self.get_range())
        return moved

    def mouse_move_stop(self):
        """
        Stop moving.
        """
        self.min_marker.mouse_move_stop()
        self.max_marker.mouse_move_stop()

    def is_marker_moving(self):
        """
        Returns true if one of the markers is being moved
        :return: True if one of the markers is being moved.
        """
        return self.min_marker.is_marker_moving() or self.max_marker.is_marker_moving()
