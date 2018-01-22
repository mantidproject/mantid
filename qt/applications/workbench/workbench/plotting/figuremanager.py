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

# std imports
import importlib
import functools
import sys

# 3rdparty imports
import matplotlib
from matplotlib.backend_bases import FigureManagerBase
from qtpy import QT_VERSION
from qtpy.QtCore import Qt, QMetaObject, QThread, Signal
from qtpy.QtWidgets import qApp, QApplication, QLabel, QMainWindow
from six import reraise, text_type

# local imports
from workbench.plotting.toolbar import WorkbenchNavigationToolbar

# Import the *real* matplotlib backend for the canvas
mpl_qt_backend = importlib.import_module('matplotlib.backends.backend_qt{}'.format(QT_VERSION[0]))
mpl_qtagg_backend = importlib.import_module('matplotlib.backends.backend_qt{}agg'.format(QT_VERSION[0]))
try:
    Gcf = getattr(mpl_qt_backend, 'Gcf')
    FigureCanvas = getattr(mpl_qtagg_backend, 'FigureCanvasQTAgg')
except KeyError:
    raise ImportError("Unknown form of matplotlib Qt backend.")


class MainWindow(QMainWindow):
    closing = Signal()

    def closeEvent(self, event):
        self.closing.emit()
        QMainWindow.closeEvent(self, event)


class FigureManagerWorkbench(FigureManagerBase):
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
        FigureManagerBase.__init__(self, canvas, num)
        self._destroy_exc = None
        self.canvas = canvas
        self.window = MainWindow()
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
            if hasattr(Gcf, 'set_hold'):
                self.toolbar.sig_hold_triggered.connect(functools.partial(Gcf.set_hold, self))
                self.toolbar.sig_active_triggered.connect(functools.partial(Gcf.set_active, self))
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

    def show(self):
        self.window.show()
        self.window.activateWindow()
        self.window.raise_()

    def destroy(self, *args):
        # check for qApp first, as PySide deletes it in its atexit handler
        if QApplication.instance() is None:
            return
        if self.window._destroying:
            return
        if QThread.currentThread() == qApp.thread():
            # direct call
            self._destroy_impl()
        else:
            # dispatch through event loop to we end up on the main thread
            QMetaObject.invokeMethod(self, "_destroy_impl",
                                     Qt.BlockingQueuedConnection)
            if self._destroy_exc is not None:
                exc_info = self._destroy_exc
                self._destroy_exc = None
                reraise(*exc_info)

    def hold(self):
        """
        Mark this figure as held
        """
        self.toolbar.hold()

    def _destroy_impl(self):
        self.window._destroying = True
        self.window.destroyed.connect(self._widgetclosed)
        try:
            if self.toolbar:
                self.toolbar.destroy()
            self.window.close()
        except BaseException:  # noqa
            self._destroy_exc = sys.exc_info()

    def get_window_title(self):
        return text_type(self.window.windowTitle())

    def set_window_title(self, title):
        self.window.setWindowTitle(title)

    # ------------------------ Interaction events --------------------
    def on_button_press(self, event):
        if not event.dblclick:
            # shortcut
            return
        # We assume this is used for editing axis information e.g. labels



class Show(object):
    """

    """
    def __call__(self, block=None):
        """
        Show all figures.

        block is ignored as calling mainloop does nothing anyway
        since the event loop is already running
        """
        managers = Gcf.get_all_fig_managers()
        if not managers:
            return

        for manager in managers:
            manager.show()

        # I'm not sure if we need to do this...

        # # Hack: determine at runtime whether we are
        # # inside ipython in pylab mode.
        # from matplotlib import pyplot
        # try:
        #     ipython_pylab = not pyplot.show._needmain
        #     # IPython versions >= 0.10 tack the _needmain
        #     # attribute onto pyplot.show, and always set
        #     # it to False, when in %pylab mode.
        #     ipython_pylab = ipython_pylab and get_backend() != 'WebAgg'
        #     # TODO: The above is a hack to get the WebAgg backend
        #     # working with ipython's `%pylab` mode until proper
        #     # integration is implemented.
        # except AttributeError:
        #     ipython_pylab = False
        #
        # # Leave the following as a separate step in case we
        # # want to control this behavior with an rcParam.
        # if ipython_pylab:
        #     return

    def mainloop(self):
        # we have already started the event loop
        pass


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
    canvas = FigureCanvas(figure)
    manager = FigureManagerWorkbench(canvas, num)
    return manager


show = Show()


if __name__ == '__main__':
    # testing code
    import numpy as np
    qapp = QApplication([' '])
    x = np.linspace(0, 2*np.pi, 100)
    cx, sx = np.cos(x), np.sin(x)
    fig_mgr_1 = new_figure_manager(1)
    fig1 = fig_mgr_1.canvas.figure
    ax = fig1.add_subplot(111)
    ax.set_title("Test title")
    ax.plot(x, cx)
    fig1.show()
    qapp.exec_()
