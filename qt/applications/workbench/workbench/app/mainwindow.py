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
ORIGINAL_SYS_EXIT = sys.exit
STDERR = sys.stderr

# -----------------------------------------------------------------------------
# Requirements
# -----------------------------------------------------------------------------
from workbench import requirements  # noqa

requirements.check_qt()

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
from qtpy.QtCore import (QByteArray, QCoreApplication, QEventLoop,
                         QPoint, QSize, Qt)  # noqa
from qtpy.QtGui import (QColor, QPixmap)  # noqa
from qtpy.QtWidgets import (QApplication, QDockWidget, QMainWindow,
                            QSplashScreen)  # noqa
from mantidqt.utils.qt import load_ui, plugins  # noqa

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

    DOCKOPTIONS = QMainWindow.AllowTabbedDocks|QMainWindow.AllowNestedDocks

    def __init__(self):
        QMainWindow.__init__(self)

        qapp = QApplication.instance()
        qapp.setAttribute(Qt.AA_UseHighDpiPixmaps)

        self.setWindowTitle("Mantid Workbench")

        # -- instance attributes --
        # widgets
        self.messagedisplay = None

        # Menus
        self.file_menu = None
        self.file_menu_actions = None

        # Allow splash screen text to be overridden in set_splash
        self.splash = SPLASH

        # Layout
        self.setDockOptions(self.DOCKOPTIONS)

    def setup(self):
        self.create_menus()
        self.create_actions()
        self.populate_menus()

        # widgets
        self.set_splash("Loading message display")
        from workbench.plugins.logmessagedisplay import LogMessageDisplay
        self.messagedisplay = LogMessageDisplay(self)

        self.setup_layout()


    def create_menus(self):
        self.file_menu = self.menuBar().addMenu("&File")

    def create_actions(self):
        action_quit = create_action(self, "&Quit", on_triggered=self.close,
                                    shortcut="Ctrl+Q",
                                    shortcut_context=Qt.ApplicationShortcut)
        self.file_menu_actions = [action_quit]

    def populate_menus(self):
        # Link to menus
        add_actions(self.file_menu, self.file_menu_actions)

    def setup_layout(self):
        settings = self.load_window_settings('window/')
        hexstate = settings[0]
        if hexstate is None:
            self.setup_for_first_run()

        self.set_window_settings(*settings)

    def setup_for_first_run(self):
        """Assume this is a first run of the application and set layouts
        accordingly"""
        self.setWindowState(Qt.WindowMaximized)
        self.setup_default_layouts()

    def set_splash(self, msg=None):
        if not self.splash:
            return
        if msg:
            self.splash.showMessage(msg, Qt.AlignBottom | Qt.AlignLeft | Qt.AlignAbsolute,
                                    QColor(Qt.black))
        QApplication.processEvents(QEventLoop.AllEvents)

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
        self.setUpdatesEnabled(False)
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

    # preloaded_packages = ('mantid', 'matplotlib')
    # for name in preloaded_packages:
    #     main_window.set_splash('Preloading ' + name)
    #     importlib.import_module('mantid')

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

    # TODO: parse command arguments

    # general initialization
    app = initialize()
    main_window = None
    try:
        main_window = start_workbench(app)
    except BaseException:
        # We count this as a crash
        import traceback
        # This is type of thing we want to capture and have reports
        # about. Prints to stderr as we can't really count on anything
        # else
        traceback.print_exc(file=STDERR)

    if main_window is None:
        # An exception occurred don't exit here
        return

    ORIGINAL_SYS_EXIT()


if __name__ == '__main__':
    main()
