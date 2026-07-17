# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt
# std imports
from math import ceil, floor
from typing import Callable, Tuple

# 3rd party imports
from matplotlib.axes import Axes
from matplotlib.image import AxesImage
import numpy as np
from qtpy.QtCore import QPoint
from qtpy.QtGui import QCursor


class CursorTracker:
    """
    Attach to the motion_notify_event of the canvas a given
    axes is draw on. Holds a list of subscribers that are notified
    of the events. Subscribers are expected to have a cursor_at method.
    """

    def __init__(self, *, image_axes: Axes, autoconnect=True):
        """
        :param image_axes: A reference to the Axes object the image resides in
        :param autoconnect: True if connections should be made on construction
        """
        self._im_axes = image_axes
        self._mouse_move_cid, self._mouse_outside_cid = None, None
        if autoconnect:
            self.connect()

    def connect(self):
        canvas = self._im_axes.figure.canvas
        self._mouse_move_cid = canvas.mpl_connect("motion_notify_event", self.on_mouse_move)
        self._mouse_outside_cid = canvas.mpl_connect("axes_leave_event", self.on_mouse_leave)

    def disconnect(self):
        canvas = self._im_axes.figure.canvas

        def impl(cid):
            if cid is not None:
                canvas.mpl_disconnect(cid)

        self._mouse_move_cid = impl(self._mouse_move_cid)
        self._mouse_outside_cid = impl(self._mouse_outside_cid)

    def on_mouse_leave(self, event):
        """
        Subscribed to the axes_leave_event.
        :param event: A MouseEvent describing the event
        """
        self.on_cursor_outside_axes()

    def on_mouse_move(self, event):
        """
        Subscribed to the motion_notify_event event.
        :param event: A MouseEvent describing the event
        """
        xdata, ydata = event.xdata, event.ydata
        if event.inaxes == self._im_axes:
            self.on_cursor_at(xdata, ydata)


class MoveMouseCursor:
    """
    Define a transformation that computes the new position on an image grid
    that will take that position to the next pixel in a given direction.
    The __call__ method expects a derived type to implement the actual
    computation of the new data coordinates
    """

    def __init__(self, image: AxesImage, new_pixel: Callable[[float, float], Tuple[float, float]], to_int: Callable[[float], int]):
        """
        :param image: A reference to the image the cursor hovers over
        :param new_pixel: Callable to calculate the new position in data coordinates
                          given an initial position.
        :param to_int: Callable converting a float to an integer. Used when converting
                       from data coordinates to Qt screen-pixel coordinates with the
                       origin at top-left of the widget
                       In the case of small shifts it can matter if the coordinate
                       is rounded up or down when moving in different directions
                       as the new data coordinate may be calculate in the same pixel
                       so to move away it must be rounded up.
        """
        self.new_pixel = new_pixel
        self.to_int = to_int
        axes = image.axes
        self.data_to_display = axes.transData
        # compute pixel widths, assuming a regular grid
        self.extent = image.get_extent()
        xmin, xmax, ymin, ymax = self.extent
        if hasattr(image, "orig_shape"):
            nx, ny = image.orig_shape
        else:
            arr = image.get_array()
            nx, ny = arr.shape[1], arr.shape[0]
        # y=rows, x=columns
        self.delta_x = (xmax - xmin) / nx
        self.delta_y = (ymax - ymin) / ny
        self.canvas = image.axes.figure.canvas

    def move_from(self, cur_pos_data: Tuple[float, float]) -> None:
        """
        Calculate a new position based on the given starting position
        and self.new_pixel transform and move the mouse cursor there.
        :param cur_pos_data: Current cursor position in data coordinates
        """
        new_pos_data = self.clip(self.new_pixel(*cur_pos_data))
        xdisp, ydisp = self.data_to_display.transform_point(new_pos_data)
        canvas = self.canvas
        dpi_ratio = canvas._dpi_ratio
        to_int = self.to_int
        xp = to_int(xdisp / dpi_ratio)
        yp = to_int((canvas.figure.bbox.height - ydisp) / dpi_ratio)
        # in the special case of just vertical/horizontal moves
        # use the original cursor position for the orthogonal coordinate
        # to avoid rounding errors and mouse jitter
        new_global_pos = canvas.mapToGlobal(QPoint(xp, yp))
        cur_global_pos = QCursor.pos()
        if new_pos_data[0] == cur_pos_data[0]:
            new_global_pos.setX(cur_global_pos.x())
        if new_pos_data[1] == cur_pos_data[1]:
            new_global_pos.setY(cur_global_pos.y())

        QCursor.setPos(new_global_pos)

    def clip(self, pos):
        """
        If the given position is outside of the image extents pull it back to
        just inside the boundary.
        :param pos: (xdata, ydata) Position in data coordinates
        """
        xmin, xmax, ymin, ymax = self.extent
        return np.clip(pos[0], xmin, xmax), np.clip(pos[1], ymin, ymax)


class MoveMouseCursorUp(MoveMouseCursor):
    """
    Transform to compute the new position in data coordinates
    by moving up 1 pixel
    """

    def __init__(self, image: AxesImage):
        super().__init__(image, new_pixel=lambda x, y: (x, y + self.delta_y), to_int=floor)


class MoveMouseCursorDown(MoveMouseCursor):
    """Transform to compute the new position in data coordinates
    by moving down 1 pixel
    """

    def __init__(self, image: AxesImage):
        super().__init__(image, new_pixel=lambda x, y: (x, y - self.delta_y), to_int=ceil)


class MoveMouseCursorLeft(MoveMouseCursor):
    """Transform to compute the new position in data coordinates
    by moving left 1 pixel
    """

    def __init__(self, image: AxesImage):
        super().__init__(image, new_pixel=lambda x, y: (x - self.delta_x, y), to_int=floor)


class MoveMouseCursorRight(MoveMouseCursor):
    """Transform to compute the new position in data coordinates
    by moving right 1 pixel
    """

    def __init__(self, image: AxesImage):
        super().__init__(image, new_pixel=lambda x, y: (x + self.delta_x, y), to_int=ceil)
