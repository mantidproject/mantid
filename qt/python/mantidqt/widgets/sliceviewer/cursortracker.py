# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt
# std imports

# 3rd party imports
from matplotlib.axes import Axes


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
        self._mouse_move_cid = canvas.mpl_connect('motion_notify_event', self.on_mouse_move)
        self._mouse_outside_cid = canvas.mpl_connect('axes_leave_event', self.on_mouse_leave)

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
