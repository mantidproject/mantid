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
"""
Defines the QMainWindow of the application and the main() entry point.
"""
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

import atexit
import importlib
import sys

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
SYSCHECK_INTERVAL = 50
ORIGINAL_SYS_EXIT = sys.exit
ORIGINAL_STDOUT = sys.stdout
ORIGINAL_STDERR = sys.stderr

# -----------------------------------------------------------------------------
# Requirements
# -----------------------------------------------------------------------------
from workbench import requirements  # noqa

requirements.check_qt()

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
from qtpy.QtCore import (QByteArray, QCoreApplication, QEventLoop,
                         QPoint, QSize, Qt, QTimer)  # noqa
from qtpy.QtGui import (QColor, QPixmap)  # noqa
from qtpy.QtWidgets import (QApplication, QDockWidget, QFileDialog, QMainWindow,
                            QSplashScreen)  # noqa
from mantidqt.utils.qt import plugins, widget_updates_disabled  # noqa

# Pre-application setup
plugins.setup_library_paths()
if hasattr(Qt, 'AA_EnableHighDpiScaling'):
    QCoreApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)


# -----------------------------------------------------------------------------
# Create the application instance early, set the application name for window
# titles and hold on to a reference to it. Required to be performed early so
# that the splash screen can be displayed
# -----------------------------------------------------------------------------

def qapplication():
    """Either return a reference to an existing application instance
    or create a new one
    :return: A reference to the QApplication object
    """
    app = QApplication.instance()
    if app is None:
        app = QApplication(['Mantid Workbench'])
    return app


# Create the application object early
MAIN_APP = qapplication()

# -----------------------------------------------------------------------------
# Splash screen
# -----------------------------------------------------------------------------
# Importing resources loads the data in
from workbench.app.resources import qCleanupResources  # noqa
atexit.register(qCleanupResources)

SPLASH = QSplashScreen(QPixmap(':/images/MantidSplashScreen.png'),
                       Qt.WindowStaysOnTopHint)
SPLASH.show()
SPLASH.showMessage("Starting...", Qt.AlignBottom | Qt.AlignLeft |
                   Qt.AlignAbsolute, QColor(Qt.black))
# The event loop has not started - force event processing
QApplication.processEvents(QEventLoop.AllEvents)

# -----------------------------------------------------------------------------
# Utilities/Widgets
# -----------------------------------------------------------------------------
from mantidqt.py3compat import qbytearray_to_str  # noqa
from mantidqt.utils.qt import add_actions, create_action  # noqa
from workbench.config.main import CONF  # noqa
from workbench.external.mantid import prepare_mantid_env  # noqa

# -----------------------------------------------------------------------------
# MainWindow
# -----------------------------------------------------------------------------


class MainWindow(QMainWindow):

    DOCKOPTIONS = QMainWindow.AllowTabbedDocks | QMainWindow.AllowNestedDocks

    def __init__(self):
        QMainWindow.__init__(self)

        qapp = QApplication.instance()
        qapp.setAttribute(Qt.AA_UseHighDpiPixmaps)

        self.setWindowTitle("Mantid Workbench")

        # -- instance attributes --
        self.window_size = None
        self.window_position = None
        self.maximized_flag = None
        # widgets
        self.messagedisplay = None
        self.ipythonconsole = None
        self.workspacewidget = None
        self.editor = None
        self.algorithm_selector = None
        self.widgets = []

        # Menus
        self.file_menu = None
        self.file_menu_actions = None
        self.editor_menu = None

        # Allow splash screen text to be overridden in set_splash
        self.splash = SPLASH

        # Layout
        self.setDockOptions(self.DOCKOPTIONS)

    def setup(self):
        # menus must be done first so they can be filled by the
        # plugins in register_plugin
        self.create_menus()
        self.create_actions()
        self.populate_menus()

        # widgets
        self.set_splash("Loading message display")
        from workbench.plugins.logmessagedisplay import LogMessageDisplay
        self.messagedisplay = LogMessageDisplay(self)
        # this takes over stdout/stderr
        self.messagedisplay.register_plugin()
        self.widgets.append(self.messagedisplay)

        self.set_splash("Loading IPython console")
        from workbench.plugins.jupyterconsole import JupyterConsole
        self.ipythonconsole = JupyterConsole(self)
        self.ipythonconsole.register_plugin()
        self.widgets.append(self.ipythonconsole)

        self.set_splash("Loading code editing widget")
        from workbench.plugins.editor import MultiFileEditor
        self.editor = MultiFileEditor(self)
        self.editor.register_plugin()
        self.widgets.append(self.editor)

        self.set_splash("Loading Workspace Widget")
        from workbench.plugins.workspacewidget import WorkspaceWidget
        self.workspacewidget = WorkspaceWidget(self)
        self.workspacewidget.register_plugin()
        self.widgets.append(self.workspacewidget)

        self.set_splash("Loading Algorithm Selector")
        from workbench.plugins.algorithmselectorwidget import AlgorithmSelector
        self.algorithm_selector = AlgorithmSelector(self)
        self.algorithm_selector.register_plugin()
        self.widgets.append(self.algorithm_selector)

        self.setup_layout()
        self.read_user_settings()

    def set_splash(self, msg=None):
        if not self.splash:
            return
        if msg:
            self.splash.showMessage(msg, Qt.AlignBottom | Qt.AlignLeft | Qt.AlignAbsolute,
                                    QColor(Qt.black))
        QApplication.processEvents(QEventLoop.AllEvents)

    def create_menus(self):
        self.file_menu = self.menuBar().addMenu("&File")
        self.editor_menu = self.menuBar().addMenu("&Editor")

    def create_actions(self):
        # --- general application menu options --
        # file menu
        action_open = create_action(self, "Open File",
                                    on_triggered=self.open_file,
                                    shortcut="Ctrl+O",
                                    shortcut_context=Qt.ApplicationShortcut)
        action_save = create_action(self, "Save File",
                                    on_triggered=self.save_file,
                                    shortcut="Ctrl+S",
                                    shortcut_context=Qt.ApplicationShortcut)

        action_quit = create_action(self, "&Quit", on_triggered=self.close,
                                    shortcut="Ctrl+Q",
                                    shortcut_context=Qt.ApplicationShortcut)
        self.file_menu_actions = [action_open, action_save, None, action_quit]

    def populate_menus(self):
        # Link to menus
        add_actions(self.file_menu, self.file_menu_actions)

    def add_dockwidget(self, plugin):
        """Create a dockwidget around a plugin and add the dock to window"""
        dockwidget, location = plugin.create_dockwidget()
        self.addDockWidget(location, dockwidget)

    # ----------------------- Layout ---------------------------------

    def setup_layout(self):
        window_settings = self.load_window_settings('window/')
        hexstate = window_settings[0]
        if hexstate is None:
            self.setup_for_first_run(window_settings)
        else:
            self.set_window_settings(*window_settings)

    def setup_for_first_run(self, window_settings):
        """Assume this is a first run of the application and set layouts
        accordingly"""
        self.setWindowState(Qt.WindowMaximized)
        self.setup_default_layouts(window_settings)

    def load_window_settings(self, prefix, section='main'):
        """Load window layout settings from userconfig-based configuration
        with *prefix*, under *section*
        default: if True, do not restore inner layout"""
        get_func = CONF.get
        window_size = get_func(section, prefix + 'size')
        try:
            hexstate = get_func(section, prefix + 'state')
        except KeyError:
            hexstate = None
        pos = get_func(section, prefix + 'position')

        # It's necessary to verify if the window/position value is valid
        # with the current screen.
        width = pos[0]
        height = pos[1]
        screen_shape = QApplication.desktop().geometry()
        current_width = screen_shape.width()
        current_height = screen_shape.height()
        if current_width < width or current_height < height:
            pos = CONF.get_default(section, prefix + 'position')

        is_maximized = get_func(section, prefix + 'is_maximized')
        is_fullscreen = get_func(section, prefix + 'is_fullscreen')
        return hexstate, window_size, pos, is_maximized, \
            is_fullscreen

    def get_window_settings(self):
        """Return current window settings
        Symetric to the 'set_window_settings' setter"""
        window_size = (self.window_size.width(), self.window_size.height())
        is_fullscreen = self.isFullScreen()
        if is_fullscreen:
            is_maximized = self.maximized_flag
        else:
            is_maximized = self.isMaximized()
        pos = (self.window_position.x(), self.window_position.y())
        hexstate = qbytearray_to_str(self.saveState())
        return (hexstate, window_size, pos, is_maximized,
                is_fullscreen)

    def set_window_settings(self, hexstate, window_size, pos,
                            is_maximized, is_fullscreen):
        """Set window settings
        Symetric to the 'get_window_settings' accessor"""
        with widget_updates_disabled(self):
            self.window_size = QSize(window_size[0], window_size[1])  # width,height
            self.window_position = QPoint(pos[0], pos[1])  # x,y
            self.setWindowState(Qt.WindowNoState)
            self.resize(self.window_size)
            self.move(self.window_position)

            # Window layout
            if hexstate:
                self.restoreState(QByteArray().fromHex(
                    str(hexstate).encode('utf-8')))
                # QDockWidget objects are not painted if restored as floating
                # windows, so we must dock them before showing the mainwindow.
                for widget in self.children():
                    if isinstance(widget, QDockWidget) and widget.isFloating():
                        self.floating_dockwidgets.append(widget)
                        widget.setFloating(False)

            # Is fullscreen?
            if is_fullscreen:
                self.setWindowState(Qt.WindowFullScreen)

            # Is maximized?
            if is_fullscreen:
                self.maximized_flag = is_maximized
            elif is_maximized:
                self.setWindowState(Qt.WindowMaximized)

    def setup_default_layouts(self, window_settings):
        """Set or reset the layouts of the child widgets"""
        self.set_window_settings(*window_settings)

        # layout definition
        logmessages = self.messagedisplay
        ipython = self.ipythonconsole
        workspacewidget = self.workspacewidget
        editor = self.editor
        algorithm_selector = self.algorithm_selector
        default_layout = {
            'widgets': [
                # column 0
                [[workspacewidget], [algorithm_selector]],
                # column 1
                [[editor, ipython]],
                # column 2
                [[logmessages]]
            ],
            'width-fraction': [0.25,            # column 0 width
                               0.50,            # column 1 width
                               0.25],           # column 2 width
            'height-fraction': [[0.5, 0.5],     # column 0 row heights
                                [1.0],          # column 1 row heights
                                [1.0]]          # column 2 row heights
        }

        with widget_updates_disabled(self):
            widgets_layout = default_layout['widgets']
            # flatten list
            widgets = [item for column in widgets_layout for row in column for item in row]
            # show everything
            map(lambda w: w.toggle_view(True), widgets)
            # split everything on the horizontal
            for i in range(len(widgets) - 1):
                first, second = widgets[i], widgets[i+1]
                self.splitDockWidget(first.dockwidget, second.dockwidget,
                                     Qt.Horizontal)
            # now arrange the rows
            for column in widgets_layout:
                for i in range(len(column) - 1):
                    first_row, second_row = column[i], column[i+1]
                    self.splitDockWidget(first_row[0].dockwidget,
                                         second_row[0].dockwidget,
                                         Qt.Vertical)
            # and finally tabify those in the same position
            for column in widgets_layout:
                for row in column:
                    for i in range(len(row) - 1):
                        first, second = row[i], row[i+1]
                        self.tabifyDockWidget(first.dockwidget, second.dockwidget)

                    # Raise front widget per row
                    row[0].dockwidget.show()
                    row[0].dockwidget.raise_()

            # set the width and height
            self._layout_widget_info = []
            width, height = self.window_size.width(), self.window_size.height()

            # fix column width
            for c in range(len(widgets_layout)):
                widget = widgets_layout[c][0][0].dockwidget
                min_width, max_width = widget.minimumWidth(), widget.maximumWidth()
                info = {'widget': widget,
                        'dock min width': min_width,
                        'dock max width': max_width}
                self._layout_widget_info.append(info)
                new_width = int(default_layout['width-fraction'][c] * width)
                widget.setMinimumWidth(new_width)
                widget.setMaximumWidth(new_width)
                widget.updateGeometry()

            # fix column height
            for c, column in enumerate(widgets_layout):
                for r in range(len(column) - 1):
                    widget = column[r][0]
                    dockwidget = widget.dockwidget
                    dock_min_h = dockwidget.minimumHeight()
                    dock_max_h = dockwidget.maximumHeight()
                    info = {'widget': widget,
                            'dock min height': dock_min_h,
                            'dock max height': dock_max_h}
                    self._layout_widget_info.append(info)
                    # The 0.95 factor is to adjust height based on usefull
                    # estimated area in the window
                    new_height = int(default_layout['height-fraction'][c][r] * height)
                    dockwidget.setMinimumHeight(new_height)
                    dockwidget.setMaximumHeight(new_height)
                    dockwidget.updateGeometry()

            self._custom_layout_timer = QTimer(self)
            self._custom_layout_timer.timeout.connect(self.layout_fix_timer)
            self._custom_layout_timer.setSingleShot(True)
            self._custom_layout_timer.start(500)

    def layout_fix_timer(self):
        """Fixes the height of docks after a new layout is set."""
        info = self._layout_widget_info
        for i in info:
            dockwidget = i['widget']
            if 'dock min width' in i:
                dockwidget.setMinimumWidth(i['dock min width'])
                dockwidget.setMaximumWidth(i['dock max width'])
            if 'dock min height' in i:
                dockwidget.setMinimumHeight(i['dock min height'])
                dockwidget.setMaximumHeight(i['dock max height'])

        self.setUpdatesEnabled(True)

    def save_current_window_settings(self, prefix, section='main'):
        """Save current window settings with *prefix* in
        the userconfig-based configuration, under *section*"""
        win_size = self.window_size

        CONF.set(section, prefix + 'size', (win_size.width(), win_size.height()))
        CONF.set(section, prefix + 'is_maximized', self.isMaximized())
        CONF.set(section, prefix + 'is_fullscreen', self.isFullScreen())
        pos = self.window_position
        CONF.set(section, prefix + 'position', (pos.x(), pos.y()))
        self.maximize_dockwidget(restore=True)  # Restore non-maximized layout
        qba = self.saveState()
        CONF.set(section, prefix + 'state', qbytearray_to_str(qba))

    def read_user_settings(self):
        for widget in self.widgets:
            widget.read_user_settings(CONF.qsettings)

    # ----------------------- Slots ---------------------------------
    def open_file(self):
        # todo: when more file types are added this should
        # live in its own type
        filepath, _ = QFileDialog.getOpenFileName(self, "Open File...", "", "Python (*.py)")
        if not filepath:
            return
        self.editor.open_file_in_new_tab(filepath)

    def save_file(self):
        # todo: how should this interact with project saving and workspaces when they are implemented?
        self.editor.save_current_file()


def initialize():
    """Perform an initialization of the application instance. Most notably
    this patches sys.exit so that it does nothing.

    :return: A reference to the existing application instance
    """
    app = qapplication()

    # Monkey patching sys.exit so users can't kill
    # the application this way
    def fake_sys_exit(arg=[]):
        pass

    sys.exit = fake_sys_exit

    return app


def start_workbench(app):
    """Given an application instance create the MainWindow,
    show it and start the main event loop
    """
    main_window = MainWindow()
    main_window.setup()

    preloaded_packages = ('mantid',)
    for name in preloaded_packages:
        main_window.set_splash('Preloading ' + name)
        importlib.import_module(name)

    main_window.show()
    if main_window.splash:
        main_window.splash.hide()
    # lift-off!
    app.exec_()

    return main_window


def main():
    """Main entry point for the application"""
    # Prepare for mantid import
    prepare_mantid_env()

    # todo: parse command arguments

    # general initialization
    app = initialize()
    # the default sys check interval leads to long lags
    # when request scripts to be aborted
    sys.setcheckinterval(SYSCHECK_INTERVAL)
    main_window = None
    try:
        main_window = start_workbench(app)
    except BaseException:
        # We count this as a crash
        import traceback
        # This is type of thing we want to capture and have reports
        # about. Prints to stderr as we can't really count on anything
        # else
        traceback.print_exc(file=ORIGINAL_STDERR)

    if main_window is None:
        # An exception occurred don't exit here
        return

    ORIGINAL_SYS_EXIT()


if __name__ == '__main__':
    main()
