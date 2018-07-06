#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""Provides our custom figure manager to wrap the canvas, window and our custom toolbar"""

# 3rdparty imports
import matplotlib
from matplotlib.backend_bases import FigureManagerBase
from matplotlib.backends.backend_qt5agg import (FigureCanvasQTAgg, backend_version, draw_if_interactive, show)  # noqa
from matplotlib._pylab_helpers import Gcf
from qtpy.QtCore import Qt, QEvent, QObject, Signal
from qtpy.QtWidgets import QApplication, QLabel, QMainWindow
from six import text_type

# local imports
from .propertiesdialog import LabelEditor, XAxisEditor, YAxisEditor
from .toolbar import WorkbenchNavigationToolbar
from .qappthreadcall import QAppThreadCall


class MainWindow(QMainWindow):
    activated = Signal()
    closing = Signal()

    def event(self, event):
        if event.type() == QEvent.WindowActivate:
            self.activated.emit()
        return QMainWindow.event(self, event)

    def closeEvent(self, event):
        self.closing.emit()
        QMainWindow.closeEvent(self, event)


class FigureManagerWorkbench(FigureManagerBase, QObject):
    """
    Attributes
    ----------
    canvas : `FigureCanvas`
        The FigureCanvas instance
    num : int or str
        The Figure number
    toolbar : qt.QToolBar
        The qt.QToolBar
    window : qt.QMainWindow
        The qt.QMainWindow

    """

    def __init__(self, canvas, num):
        QObject.__init__(self)
        FigureManagerBase.__init__(self, canvas, num)
        # Patch show/destroy to be thread aware
        self._destroy_orig = self.destroy
        self.destroy = QAppThreadCall(self._destroy_orig)
        self._show_orig = self.show
        self.show = QAppThreadCall(self._show_orig)
        self._window_activated_orig = self._window_activated
        self._window_activated = QAppThreadCall(self._window_activated_orig)
        self._widgetclosed_orig = self._widgetclosed
        self._widgetclosed = QAppThreadCall(self._widgetclosed_orig)

        self.canvas = canvas
        self.window = MainWindow()
        self.window.activated.connect(self._window_activated)
        self.window.closing.connect(canvas.close_event)
        self.window.closing.connect(self._widgetclosed)

        self.window.setWindowTitle("Figure %d" % num)

        # Give the keyboard focus to the figure instead of the
        # manager; StrongFocus accepts both tab and click to focus and
        # will enable the canvas to process event w/o clicking.
        # ClickFocus only takes the focus is the window has been
        # clicked
        # on. http://qt-project.org/doc/qt-4.8/qt.html#FocusPolicy-enum or
        # http://doc.qt.digia.com/qt/qt.html#FocusPolicy-enum
        self.canvas.setFocusPolicy(Qt.StrongFocus)
        self.canvas.setFocus()

        self.window._destroying = False

        # add text label to status bar
        self.statusbar_label = QLabel()
        self.window.statusBar().addWidget(self.statusbar_label)

        self.toolbar = self._get_toolbar(self.canvas, self.window)
        if self.toolbar is not None:
            self.window.addToolBar(self.toolbar)
            self.toolbar.message.connect(self.statusbar_label.setText)
            self.toolbar.sig_grid_toggle_triggered.connect(self.grid_toggle)
            tbs_height = self.toolbar.sizeHint().height()
        else:
            tbs_height = 0

        # resize the main window so it will display the canvas with the
        # requested size:
        cs = canvas.sizeHint()
        sbs = self.window.statusBar().sizeHint()
        self._status_and_tool_height = tbs_height + sbs.height()
        height = cs.height() + self._status_and_tool_height
        self.window.resize(cs.width(), height)

        self.window.setCentralWidget(self.canvas)

        if matplotlib.is_interactive():
            self.window.show()
            self.canvas.draw_idle()

        def notify_axes_change(fig):
            # This will be called whenever the current axes is changed
            if self.toolbar is not None:
                self.toolbar.update()
        self.canvas.figure.add_axobserver(notify_axes_change)

        # Register canvas observers
        self._cids = []
        self._cids.append(self.canvas.mpl_connect('button_press_event', self.on_button_press))

        self.window.raise_()

    def full_screen_toggle(self):
        if self.window.isFullScreen():
            self.window.showNormal()
        else:
            self.window.showFullScreen()

    def _window_activated(self):
        Gcf.set_active(self)

    def _widgetclosed(self):
        if self.window._destroying:
            return
        self.window._destroying = True
        map(self.canvas.mpl_disconnect, self._cids)
        try:
            Gcf.destroy(self.num)
        except AttributeError:
            pass
            # It seems that when the python session is killed,
            # Gcf can get destroyed before the Gcf.destroy
            # line is run, leading to a useless AttributeError.

    def _get_toolbar(self, canvas, parent):
            return WorkbenchNavigationToolbar(canvas, parent, False)

    def resize(self, width, height):
        'set the canvas size in pixels'
        self.window.resize(width, height + self._status_and_tool_height)

    def show(self):  # noqa
        self.window.show()
        self.window.activateWindow()
        self.window.raise_()
        if self.window.windowState() & Qt.WindowMinimized:
            # windowState() stores a combination of window state enums
            # and multiple window states can be valid. On Windows
            # a window can be both minimized and maximized at the
            # same time, so we make a check here. For more info see:
            # http://doc.qt.io/qt-5/qt.html#WindowState-enum
            if self.window.windowState() & Qt.WindowMaximized:
                self.window.setWindowState(Qt.WindowMaximized)
            else:
                self.window.setWindowState(Qt.WindowNoState)

        # Hack to ensure the canvas is up to date
        self.canvas.draw_idle()

    def destroy(self, *args):
        # check for qApp first, as PySide deletes it in its atexit handler
        if QApplication.instance() is None:
            return
        if self.window._destroying:
            return
        self.window._destroying = True
        self.window.destroyed.connect(self._widgetclosed)
        if self.toolbar:
            self.toolbar.destroy()
        self.window.close()

    def grid_toggle(self):
        """
        Toggle grid lines on/off
        """
        canvas = self.canvas
        axes = canvas.figure.get_axes()
        for ax in axes:
            ax.grid()
        canvas.draw_idle()

    def hold(self):
        """
        Mark this figure as held
        """
        self.toolbar.hold()

    def get_window_title(self):
        return text_type(self.window.windowTitle())

    def set_window_title(self, title):
        old_title = self.get_window_title()
        self.window.setWindowTitle(title)
        # We need to add a call to the figure manager here to call
        # notify methods when a figure is renamed, to update our
        # plot list.
        Gcf.figure_title_changed(old_title, title)

    # ------------------------ Interaction events --------------------
    def on_button_press(self, event):
        if not event.dblclick:
            # shortcut
            return
        # We assume this is used for editing axis information e.g. labels
        # which are outside of the axes so event.inaxes is no use.
        canvas = self.canvas
        figure = canvas.figure
        axes = figure.get_axes()

        def move_and_show(editor):
            editor.move(event.x, figure.bbox.height - event.y + self.window.y())
            editor.exec_()

        for ax in axes:
            if ax.title.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.title))
            elif ax.xaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.xaxis.label))
            elif ax.yaxis.label.contains(event)[0]:
                move_and_show(LabelEditor(canvas, ax.yaxis.label))
            elif ax.xaxis.contains(event)[0]:
                move_and_show(XAxisEditor(canvas, ax))
            elif ax.yaxis.contains(event)[0]:
                move_and_show(YAxisEditor(canvas, ax))


# -----------------------------------------------------------------------------
# Figure control
# -----------------------------------------------------------------------------

def new_figure_manager(num, *args, **kwargs):
    """
    Create a new figure manager instance
    """
    from matplotlib.figure import Figure  # noqa
    figure_class = kwargs.pop('FigureClass', Figure)
    this_fig = figure_class(*args, **kwargs)
    return new_figure_manager_given_figure(num, this_fig)


def new_figure_manager_given_figure(num, figure):
    """
    Create a new figure manager instance for the given figure.
    """
    canvas = FigureCanvasQTAgg(figure)
    manager = FigureManagerWorkbench(canvas, num)
    return manager


if __name__ == '__main__':
    # testing code
    import numpy as np
    qapp = QApplication([' '])
    qapp.setAttribute(Qt.AA_UseHighDpiPixmaps)
    if hasattr(Qt, 'AA_EnableHighDpiScaling'):
        qapp.setAttribute(Qt.AA_EnableHighDpiScaling, True)

    x = np.linspace(0, 10*np.pi, 1000)
    cx, sx = np.cos(x), np.sin(x)
    fig_mgr_1 = new_figure_manager(1)
    fig1 = fig_mgr_1.canvas.figure
    ax = fig1.add_subplot(111)
    ax.set_title("Test title")
    ax.set_xlabel("$\mu s$")
    ax.set_ylabel("Counts")

    ax.plot(x, cx)
    fig1.show()
    qapp.exec_()
