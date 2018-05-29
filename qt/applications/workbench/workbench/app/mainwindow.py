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
MPL_BACKEND = 'module://workbench.plotting.backend_workbench'
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
from qtpy.QtCore import (QEventLoop, Qt, QTimer)  # noqa
from qtpy.QtGui import (QColor, QPixmap)  # noqa
from qtpy.QtWidgets import (QApplication, QDesktopWidget, QFileDialog,
                            QMainWindow, QSplashScreen)  # noqa
from mantidqt.utils.qt import plugins, widget_updates_disabled  # noqa

# Pre-application setup
plugins.setup_library_paths()


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
from mantidqt.widgets.manageuserdirectories import ManageUserDirectories  # noqa
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
        if hasattr(Qt, 'AA_EnableHighDpiScaling'):
            qapp.setAttribute(Qt.AA_EnableHighDpiScaling, True)

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

        # Widget layout map: required for use in Qt.connection
        self._layout_widget_info = None

        # Menus
        self.file_menu = None
        self.file_menu_actions = None
        self.editor_menu = None
        self.view_menu = None
        self.view_menu_actions = None

        # Allow splash screen text to be overridden in set_splash
        self.splash = SPLASH

        # Layout
        self.setDockOptions(self.DOCKOPTIONS)

    def setup(self):
        # menus must be done first so they can be filled by the
        # plugins in register_plugin
        self.create_menus()

        # widgets
        # Log message display must be imported first
        self.set_splash("Loading message display")
        from workbench.plugins.logmessagedisplay import LogMessageDisplay
        self.messagedisplay = LogMessageDisplay(self)
        # this takes over stdout/stderr
        self.messagedisplay.register_plugin()
        self.widgets.append(self.messagedisplay)

        self.set_splash("Loading Algorithm Selector")
        from workbench.plugins.algorithmselectorwidget import AlgorithmSelector
        self.algorithm_selector = AlgorithmSelector(self)
        self.algorithm_selector.register_plugin()
        self.widgets.append(self.algorithm_selector)

        self.set_splash("Loading code editing widget")
        from workbench.plugins.editor import MultiFileEditor
        self.editor = MultiFileEditor(self)
        self.editor.register_plugin()
        self.widgets.append(self.editor)

        self.set_splash("Loading IPython console")
        from workbench.plugins.jupyterconsole import JupyterConsole
        self.ipythonconsole = JupyterConsole(self)
        self.ipythonconsole.register_plugin()
        self.widgets.append(self.ipythonconsole)

        from workbench.plugins.workspacewidget import WorkspaceWidget
        self.workspacewidget = WorkspaceWidget(self)
        self.workspacewidget.register_plugin()
        self.widgets.append(self.workspacewidget)

        self.setup_layout()
        self.create_actions()
        self.populate_menus()

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
        self.view_menu = self.menuBar().addMenu("&View")

    def create_actions(self):
        # --- general application menu options --
        # file menu
        action_open = create_action(self, "Open",
                                    on_triggered=self.open_file,
                                    shortcut="Ctrl+O",
                                    shortcut_context=Qt.ApplicationShortcut,
                                    icon_name="fa.folder-open")
        action_save = create_action(self, "Save",
                                    on_triggered=self.save_file,
                                    shortcut="Ctrl+S",
                                    shortcut_context=Qt.ApplicationShortcut,
                                    icon_name="fa.save")
        action_manage_directories = create_action(self, "Manage User Directories",
                                                  on_triggered=self.open_manage_directories,
                                                  icon_name="fa.folder")

        action_quit = create_action(self, "&Quit", on_triggered=self.close,
                                    shortcut="Ctrl+Q",
                                    shortcut_context=Qt.ApplicationShortcut,
                                    icon_name="fa.power-off")
        self.file_menu_actions = [action_open, action_save, action_manage_directories, None, action_quit]

        # view menu
        action_restore_default = create_action(self, "Restore Default Layout",
                                               on_triggered=self.prep_window_for_reset,
                                               shortcut="Shift+F10",
                                               shortcut_context=Qt.ApplicationShortcut)
        self.view_menu_actions = [action_restore_default, None] + self.create_widget_actions()

    def create_widget_actions(self):
        """
        Creates menu actions to show/hide dockable widgets.
        This uses all widgets that are in self.widgets
        :return: A list of show/hide actions for all widgets
        """
        widget_actions = []
        for widget in self.widgets:
            action = widget.dockwidget.toggleViewAction()
            widget_actions.append(action)
        return widget_actions

    def populate_menus(self):
        # Link to menus
        add_actions(self.file_menu, self.file_menu_actions)
        add_actions(self.view_menu, self.view_menu_actions)

    def add_dockwidget(self, plugin):
        """Create a dockwidget around a plugin and add the dock to window"""
        dockwidget, location = plugin.create_dockwidget()
        self.addDockWidget(location, dockwidget)

    # ----------------------- Layout ---------------------------------

    def setup_layout(self):
        self.setup_for_first_run()

    def setup_for_first_run(self):
        """Assume this is a first run of the application and set layouts
        accordingly"""
        self.setWindowState(Qt.WindowMaximized)
        desktop = QDesktopWidget()
        self.window_size = desktop.screenGeometry().size()
        self.setup_default_layouts()

    def prep_window_for_reset(self):
        """Function to reset all dock widgets to a state where they can be
        ordered by setup_default_layout"""
        for widget in self.widgets:
            widget.dockwidget.setFloating(False)  # Bring back any floating windows
            self.addDockWidget(Qt.LeftDockWidgetArea, widget.dockwidget)  # Un-tabify all widgets
        self.setup_default_layouts()

    def setup_default_layouts(self):
        """Set or reset the layouts of the child widgets"""
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

    # ----------------------- Events ---------------------------------
    def closeEvent(self, event):
        # Close editors
        self.editor.app_closing()

        # Close all open plots
        # We don't want this at module scope here
        import matplotlib.pyplot as plt  #noqa
        plt.close('all')

        event.accept()

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

    def open_manage_directories(self):
        ManageUserDirectories(self).exec_()


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
    # The ordering here is very delicate. Test thoroughly when
    # changing anything!
    main_window = MainWindow()

    # Load matplotlib as early as possible.
    # Setup our custom backend and monkey patch in custom current figure manager
    main_window.set_splash('Preloading matplotlib')
    mpl = importlib.import_module('matplotlib')
    # Replace vanilla Gcf with our custom instance
    _pylab_helpers = importlib.import_module('matplotlib._pylab_helpers')
    currentfigure = importlib.import_module('workbench.plotting.globalfiguremanager')
    setattr(_pylab_helpers, 'Gcf', getattr(currentfigure, 'GlobalFigureManager'))
    # Set up out custom matplotlib backend early. It must be done on
    # the main thread
    mpl.use(MPL_BACKEND)

    # Setup widget layouts etc. mantid cannot be imported before this
    # or the log messages don't get through
    main_window.setup()
    # start mantid
    main_window.set_splash('Preloading mantid')
    importlib.import_module('mantid')

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
