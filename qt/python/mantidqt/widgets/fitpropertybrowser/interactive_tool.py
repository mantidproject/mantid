import numpy as np

from matplotlib.path import Path
from matplotlib.patches import PathPatch
from qtpy.QtCore import QObject, Signal, Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication


class VerticalMarker(QObject):

    moved = Signal(float)

    def __init__(self, canvas, color, x, y0=None, y1=None):
        super(VerticalMarker, self).__init__()
        self.ax = canvas.figure.get_axes()[0]
        self.x = x
        self.y0 = y0
        self.y1 = y1
        y0, y1 = self._get_y0_y1()
        path = Path([(x, y0), (x, y1)], [Path.MOVETO, Path.LINETO])
        self.patch = PathPatch(path, facecolor='None', edgecolor=color, picker=5, linewidth=2.0, animated=True)
        self.ax.add_patch(self.patch)
        self.is_moving = False

    def _get_y0_y1(self):
        if self.y0 is None or self.y1 is None:
            y0, y1 = self.ax.get_ylim()
        if self.y0 is not None:
            y0 = self.y0
        if self.y1 is not None:
            y1 = self.y1
        return y0, y1

    def remove(self):
        self.patch.remove()

    def redraw(self):
        y0, y1 = self._get_y0_y1()
        vertices = self.patch.get_path().vertices
        vertices[0] = self.x, y0
        vertices[1] = self.x, y1
        self.ax.draw_artist(self.patch)

    def get_x_in_pixels(self):
        x_pixels, _ = self.patch.get_transform().transform((self.x, 0))
        return x_pixels

    def is_above(self, x):
        return np.abs(self.get_x_in_pixels() - x) < 3

    def on_click(self, x):
        if self.is_above(x):
            self.is_moving = True

    def stop(self):
        self.is_moving = False

    def mouse_move(self, xd):
        if self.is_moving and xd is not None:
            self.x = xd
            self.moved.emit(xd)

    def should_override_cursor(self, x):
        return self.is_moving or self.is_above(x)


class FitInteractiveTool(QObject):

    fit_start_x_moved = Signal(float)
    fit_end_x_moved = Signal(float)

    def __init__(self, canvas):
        super(FitInteractiveTool, self).__init__()
        self.canvas = canvas
        ax = canvas.figure.get_axes()[0]
        self.ax = ax
        xlim = ax.get_xlim()
        dx = (xlim[1] - xlim[0]) / 20.
        start_x = xlim[0] + dx
        end_x = xlim[1] - dx
        self.fit_start_x = VerticalMarker(canvas, 'green', start_x)
        self.fit_end_x = VerticalMarker(canvas, 'green', end_x)

        self.fit_start_x.moved.connect(self.fit_start_x_moved)
        self.fit_end_x.moved.connect(self.fit_end_x_moved)

        self.peak_markers = []

        self._cids = []
        self._cids.append(canvas.mpl_connect('draw_event', self.draw_callback))
        self._cids.append(canvas.mpl_connect('motion_notify_event', self.motion_notify_callback))
        self._cids.append(canvas.mpl_connect('button_press_event', self.on_click))
        self._cids.append(canvas.mpl_connect('button_release_event', self.on_release))

        self._override_cursor = False

    def disconnect(self):
        for cid in self._cids:
            self.canvas.mpl_disconnect(cid)
        self.fit_start_x.remove()
        self.fit_end_x.remove()

    def draw_callback(self, event):
        if self.fit_start_x.x > self.fit_end_x.x:
            x = self.fit_start_x.x
            self.fit_start_x.x = self.fit_end_x.x
            self.fit_end_x.x = x
        self.fit_start_x.redraw()
        self.fit_end_x.redraw()
        for pm in self.peak_markers:
            pm.redraw()

    def get_override_cursor(self, x, y):
        if self.fit_start_x.should_override_cursor(x) or self.fit_end_x.should_override_cursor(x):
            return QCursor(Qt.SizeHorCursor)
        return None

    @property
    def override_cursor(self):
        return self._override_cursor

    @override_cursor.setter
    def override_cursor(self, cursor):
        self._override_cursor = cursor
        QApplication.restoreOverrideCursor()
        if cursor is not None:
            QApplication.setOverrideCursor(cursor)

    def motion_notify_callback(self, event):
        x = event.x
        if x is None:
            return
        self.override_cursor = self.get_override_cursor(x, 0)
        self.fit_start_x.mouse_move(event.xdata)
        self.fit_end_x.mouse_move(event.xdata)

    def on_click(self, event):
        if event.button == 1:
            self.fit_start_x.on_click(event.x)
            self.fit_end_x.on_click(event.x)

    def on_release(self, event):
        self.fit_start_x.stop()
        self.fit_end_x.stop()

    def move_start_x(self, xd):
        if xd is not None:
            self.fit_start_x.x = xd
            self.canvas.draw()

    def move_end_x(self, xd):
        if xd is not None:
            self.fit_end_x.x = xd
            self.canvas.draw()

    def add_peak(self, x, y_top, y_bottom=None):
        self.peak_markers.append(PeakMarker(self.canvas, x, y_top, y_bottom))
        self.canvas.draw()


class PeakMarker(QObject):

    def __init__(self, canvas, x, y_top, y_bottom):
        super(PeakMarker, self).__init__()
        self.centre_marker = VerticalMarker(canvas, 'red', x, y0=y_bottom, y1=y_top)

    def redraw(self):
        self.centre_marker.redraw()
