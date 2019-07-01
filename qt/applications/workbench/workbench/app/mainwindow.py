# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Defines the QMainWindow of the application and the main() entry point.
"""
from __future__ import (absolute_import, division, print_function, unicode_literals)

import argparse
import atexit
import importlib
import os
import sys
from functools import partial

from mantid.api import FrameworkManagerImpl
from mantid.kernel import (ConfigService, UsageService, logger, version_str as mantid_version_str)
from mantid.py3compat import setswitchinterval
from workbench.plugins.exception_handler import exception_logger
from workbench.widgets.settings.presenter import SettingsPresenter

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------

SYSCHECK_INTERVAL = 50
ORIGINAL_SYS_EXIT = sys.exit
ORIGINAL_STDOUT = sys.stdout
ORIGINAL_STDERR = sys.stderr

from workbench import requirements  # noqa

requirements.check_qt()

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
from qtpy.QtCore import (QEventLoop, Qt, QCoreApplication, QPoint, QSize)  # noqa
from qtpy.QtGui import (QColor, QGuiApplication, QIcon, QPixmap)  # noqa
from qtpy.QtWidgets import (QApplication, QDesktopWidget, QFileDialog,
                            QMainWindow, QSplashScreen)  # noqa
from mantidqt.algorithminputhistory import AlgorithmInputHistory  # noqa
from mantidqt.interfacemanager import InterfaceManager  # noqa
from mantidqt.widgets.manageuserdirectories import ManageUserDirectories  # noqa
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution  # noqa
from mantidqt.utils.qt import (add_actions, create_action, plugins,
                               widget_updates_disabled)  # noqa
from mantidqt.project.project import Project  # noqa
from mantidqt.interfacemanager import InterfaceManager  # noqa
from mantidqt.usersubwindowfactory import UserSubWindowFactory  # noqa

# Pre-application setup
plugins.setup_library_paths()

# Importing resources loads the data in. This must be imported before the
# QApplication is created or paths to Qt's resources will not be set up correctly
from workbench.app.resources import qCleanupResources  # noqa
from workbench.config import APPNAME, CONF, ORG_DOMAIN, ORGANIZATION  # noqa
from workbench.plotting.globalfiguremanager import GlobalFigureManager  # noqa
from workbench.app.windowfinder import find_all_windows_that_are_savable  # noqa
from workbench.projectrecovery.projectrecovery import ProjectRecovery  # noqa


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
        QCoreApplication.setAttribute(Qt.AA_ShareOpenGLContexts)
        argv = sys.argv[:]
        argv[0] = APPNAME  # replace application name
        # Workaround a segfault with the IPython console when using Python 3.5 + PyQt 5
        # Without this using this fix the above combination causes a segfault when the IPython
        # console is started
        # The workaround mentioned in https://groups.google.com/forum/#!topic/leo-editor/ghiIN7irzY0
        # is to ensure readline is imported before the QApplication object is created
        if sys.version_info[0] == 3 and sys.version_info[1] == 5:
            importlib.import_module("readline")
        app = QApplication(argv)
        app.setOrganizationName(ORGANIZATION)
        app.setOrganizationDomain(ORG_DOMAIN)
        app.setApplicationName(APPNAME)
        app.setApplicationVersion(mantid_version_str())
        # Spin up the usage service and set the name for the usage reporting
        # The report is sent when the FrameworkManager kicks up
        UsageService.setApplicationName(APPNAME)

    return app


# Create the application object early
MAIN_APP = qapplication()

# -----------------------------------------------------------------------------
# Splash screen
# -----------------------------------------------------------------------------

atexit.register(qCleanupResources)


def _get_splash_image_name():
    # gets the width of the screen where the main window was initialised
    width = QGuiApplication.primaryScreen().size().width()

    if width > 2048:
        return ':/images/MantidSplashScreen_4k.jpg'
    else:
        return ':/images/MantidSplashScreen.png'


SPLASH = QSplashScreen(QPixmap(_get_splash_image_name()),
                       Qt.WindowStaysOnTopHint)
SPLASH.show()
SPLASH.showMessage("Starting...", Qt.AlignBottom | Qt.AlignLeft
                   | Qt.AlignAbsolute, QColor(Qt.black))
# The event loop has not started - force event processing
QApplication.processEvents(QEventLoop.AllEvents)


# -----------------------------------------------------------------------------
# MainWindow
# -----------------------------------------------------------------------------


class MainWindow(QMainWindow):
    DOCKOPTIONS = QMainWindow.AllowTabbedDocks | QMainWindow.AllowNestedDocks

    def __init__(self):
        QMainWindow.__init__(self)

        # -- instance attributes --
        self.setWindowTitle("Mantid Workbench")
        self.setObjectName("Mantid Workbench")

        # widgets
        self.messagedisplay = None
        self.ipythonconsole = None
        self.workspacewidget = None
        self.editor = None
        self.algorithm_selector = None
        self.plot_selector = None
        self.interface_manager = None
        self.widgets = []

        # Widget layout map: required for use in Qt.connection
        self._layout_widget_info = None

        # Menus
        self.file_menu = None
        self.file_menu_actions = None
        self.view_menu = None
        self.view_menu_actions = None
        self.interfaces_menu = None
        self.help_menu = None
        self.help_menu_actions = None

        # Allow splash screen text to be overridden in set_splash
        self.splash = SPLASH

        # Layout
        self.setDockOptions(self.DOCKOPTIONS)

        # Project
        self.project = None
        self.project_recovery = None

        # Interfaces
        self.interface_manager = None
        self.interface_executor = None

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

        self.set_splash("Loading Plot Selector")
        from workbench.plugins.plotselectorwidget import PlotSelector
        self.plot_selector = PlotSelector(self)
        self.plot_selector.register_plugin()
        self.widgets.append(self.plot_selector)

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

        # Set up the project, recovery and interface manager objects
        self.project = Project(GlobalFigureManager, find_all_windows_that_are_savable)
        self.project_recovery = ProjectRecovery(globalfiguremanager=GlobalFigureManager,
                                                multifileinterpreter=self.editor.editors,
                                                main_window=self)

        self.interface_executor = PythonCodeExecution()
        self.interface_executor.sig_exec_error.connect(lambda errobj: logger.warning(str(errobj)))
        self.interface_manager = InterfaceManager()

        # uses default configuration as necessary
        self.readSettings(CONF)
        self.config_updated()

        self.setup_layout()
        self.create_actions()

    def post_mantid_init(self):
        """Run any setup that requires mantid
        to have been initialized
        """
        self.populate_menus()
        self.algorithm_selector.refresh()

        # turn on algorithm factory notifications
        from mantid.api import AlgorithmFactory
        algorithm_factory = AlgorithmFactory.Instance()
        algorithm_factory.enableNotifications()

    def set_splash(self, msg=None):
        if not self.splash:
            return
        if msg:
            self.splash.showMessage(msg, Qt.AlignBottom | Qt.AlignLeft | Qt.AlignAbsolute,
                                    QColor(Qt.black))
        QApplication.processEvents(QEventLoop.AllEvents)

    def create_menus(self):
        self.file_menu = self.menuBar().addMenu("&File")
        self.view_menu = self.menuBar().addMenu("&View")
        self.interfaces_menu = self.menuBar().addMenu('&Interfaces')
        self.help_menu = self.menuBar().addMenu('&Help')

    def create_actions(self):
        # --- general application menu options --
        # file menu
        action_open = create_action(
            self, "Open Script", on_triggered=self.open_file,
            shortcut="Ctrl+O", shortcut_context=Qt.ApplicationShortcut)
        action_load_project = create_action(
            self, "Open Project", on_triggered=self.load_project)
        action_save_script = create_action(
            self, "Save Script", on_triggered=self.save_script,
            shortcut="Ctrl+S", shortcut_context=Qt.ApplicationShortcut)
        action_save_script_as = create_action(
            self, "Save Script as...", on_triggered=self.save_script_as)
        action_save_project = create_action(
            self, "Save Project", on_triggered=self.save_project)
        action_save_project_as = create_action(
            self, "Save Project as...", on_triggered=self.save_project_as)
        action_manage_directories = create_action(
            self, "Manage User Directories",
            on_triggered=self.open_manage_directories)
        action_settings = create_action(
            self, "Settings", on_triggered=self.open_settings_window)
        action_quit = create_action(
            self, "&Quit", on_triggered=self.close, shortcut="Ctrl+Q",
            shortcut_context=Qt.ApplicationShortcut)
        self.file_menu_actions = [action_open, action_load_project, None,
                                  action_save_script, action_save_script_as,
                                  action_save_project, action_save_project_as,
                                  None, action_settings, None,
                                  action_manage_directories, None, action_quit]
        # view menu
        action_restore_default = create_action(
            self, "Restore Default Layout",
            on_triggered=self.prep_window_for_reset,
            shortcut="Shift+F10", shortcut_context=Qt.ApplicationShortcut)

        self.view_menu_actions = [action_restore_default, None] + self.create_widget_actions()

        # help menu
        action_mantid_help = create_action(
            self, "Mantid Help", on_triggered=self.open_mantid_help,
            shortcut='F1', shortcut_context=Qt.ApplicationShortcut)
        action_algorithm_descriptions = create_action(
            self, 'Algorithm Descriptions',
            on_triggered=self.open_algorithm_descriptions_help)
        action_mantid_concepts = create_action(
            self, "Mantid Concepts", on_triggered=self.open_mantid_concepts_help)
        action_mantid_homepage = create_action(
            self, "Mantid Homepage", on_triggered=self.open_mantid_homepage)
        action_mantid_forum = create_action(
            self, "Mantid Forum", on_triggered=self.open_mantid_forum)

        self.help_menu_actions = [
            action_mantid_help, action_mantid_concepts,
            action_algorithm_descriptions, None,
            action_mantid_homepage, action_mantid_forum]

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
        add_actions(self.help_menu, self.help_menu_actions)
        self.populate_interfaces_menu()

    def launch_custom_python_gui(self, filename):
        self.interface_executor.execute(open(filename).read(), filename)

    def launch_custom_cpp_gui(self, interface_name):
        interface = self.interface_manager.createSubWindow(interface_name)
        interface.setAttribute(Qt.WA_DeleteOnClose, True)
        interface.show()

    def populate_interfaces_menu(self):
        """Populate then Interfaces menu with all Python and C++ interfaces"""
        interface_dir = ConfigService['mantidqt.python_interfaces_directory']
        interfaces = self._discover_python_interfaces(interface_dir)
        self._discover_cpp_interfaces(interfaces)

        keys = list(interfaces.keys())
        keys.sort()
        for key in keys:
            submenu = self.interfaces_menu.addMenu(key)
            names = interfaces[key]
            names.sort()
            for name in names:
                if '.py' in name:
                    action = submenu.addAction(name.replace('.py', '').replace('_', ' '))
                    script = os.path.join(interface_dir, name)
                    action.triggered.connect(lambda checked_py, script=script: self.launch_custom_python_gui(script))
                else:
                    action = submenu.addAction(name)
                    action.triggered.connect(lambda checked_cpp, name=name:
                                             self.launch_custom_cpp_gui(name))

    def _discover_python_interfaces(self, interface_dir):
        """Return a dictionary mapping a category to a set of named Python interfaces"""
        items = ConfigService['mantidqt.python_interfaces'].split()
        # list of custom interfaces that are not qt4/qt5 compatible
        GUI_BLACKLIST = ['ISIS_Reflectometry_Old.py',
                         'Frequency_Domain_Analysis_Old.py',
                         'Elemental_Analysis.py']

        # detect the python interfaces
        interfaces = {}
        for item in items:
            key, scriptname = item.split('/')
            if not os.path.exists(os.path.join(interface_dir, scriptname)):
                logger.warning('Failed to find script "{}" in "{}"'.format(scriptname, interface_dir))
                continue
            if scriptname in GUI_BLACKLIST:
                logger.information('Not adding gui "{}"'.format(scriptname))
                continue
            interfaces.setdefault(key, []).append(scriptname)

        return interfaces

    def _discover_cpp_interfaces(self, interfaces):
        """Return a dictionary mapping a category to a set of named C++ interfaces"""
        cpp_interface_factory = UserSubWindowFactory.Instance()
        interface_names = cpp_interface_factory.keys()
        for name in interface_names:
            categories = cpp_interface_factory.categories(name)
            if len(categories) == 0:
                categories = ["General"]
            for category in categories:
                if category in interfaces.keys():
                    interfaces[category].append(name)
                else:
                    interfaces[category] = [name]

        return interfaces

    def add_dockwidget(self, plugin):
        """Create a dockwidget around a plugin and add the dock to window"""
        dockwidget, location = plugin.create_dockwidget()
        self.addDockWidget(location, dockwidget)

    # ----------------------- Layout ---------------------------------

    def setup_layout(self):
        """Assume this is a first run of the application and set layouts
        accordingly"""
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
        plot_selector = self.plot_selector
        default_layout = {
            'widgets': [
                # column 0
                [[workspacewidget], [algorithm_selector, plot_selector]],
                # column 1
                [[editor, ipython]],
                # column 2
                [[logmessages]]
            ],
            'width-fraction': [0.25,  # column 0 width
                               0.50,  # column 1 width
                               0.25],  # column 2 width
            'height-fraction': [[0.5, 0.5],  # column 0 row heights
                                [1.0],  # column 1 row heights
                                [1.0]]  # column 2 row heights
        }

        with widget_updates_disabled(self):
            widgets_layout = default_layout['widgets']
            # flatten list
            widgets = [item for column in widgets_layout for row in column for item in row]
            # show everything
            for w in widgets:
                w.toggle_view(True)
            # split everything on the horizontal
            for i in range(len(widgets) - 1):
                first, second = widgets[i], widgets[i + 1]
                self.splitDockWidget(first.dockwidget, second.dockwidget,
                                     Qt.Horizontal)
            # now arrange the rows
            for column in widgets_layout:
                for i in range(len(column) - 1):
                    first_row, second_row = column[i], column[i + 1]
                    self.splitDockWidget(first_row[0].dockwidget,
                                         second_row[0].dockwidget,
                                         Qt.Vertical)
            # and finally tabify those in the same position
            for column in widgets_layout:
                for row in column:
                    for i in range(len(row) - 1):
                        first, second = row[i], row[i + 1]
                        self.tabifyDockWidget(first.dockwidget, second.dockwidget)

                    # Raise front widget per row
                    row[0].dockwidget.show()
                    row[0].dockwidget.raise_()

    # ----------------------- Events ---------------------------------
    def closeEvent(self, event):
        # Check whether or not to save project
        if not self.project.saved:
            # Offer save
            if self.project.offer_save(self):
                # Cancel has been clicked
                event.ignore()
                return

        # Close editors
        if self.editor.app_closing():
            # write out any changes to the mantid config file
            ConfigService.saveConfig(ConfigService.getUserFilename())
            # write current window information to global settings object
            self.writeSettings(CONF)
            # Close all open plots
            # We don't want this at module scope here
            import matplotlib.pyplot as plt  # noqa
            plt.close('all')

            app = QApplication.instance()
            if app is not None:
                app.closeAllWindows()

            # Kill the project recovery thread and don't restart should a save be in progress and clear out current
            # recovery checkpoint as it is closing properly
            self.project_recovery.stop_recovery_thread()
            self.project_recovery.closing_workbench = True
            self.project_recovery.remove_current_pid_folder()

            self.interface_manager.closeHelpWindow()

            event.accept()
        else:
            # Cancel was pressed when closing an editor
            event.ignore()

    # ----------------------- Slots ---------------------------------
    def open_file(self):
        # todo: when more file types are added this should
        # live in its own type
        filepath, _ = QFileDialog.getOpenFileName(self, "Open File...", "", "Python (*.py)")
        if not filepath:
            return
        self.editor.open_file_in_new_tab(filepath)

    def save_script(self):
        self.editor.save_current_file()

    def save_script_as(self):
        self.editor.save_current_file_as()

    def save_project(self):
        self.project.save()

    def save_project_as(self):
        self.project.save_as()

    def load_project(self):
        self.project.load()

    def open_manage_directories(self):
        ManageUserDirectories(self).exec_()

    def open_settings_window(self):
        settings = SettingsPresenter(self)
        settings.show()

    def config_updated(self):
        """
        Updates the widgets that depend on settings from the Workbench Config.
        """
        self.editor.load_settings_from_config(CONF)
        self.project.load_settings_from_config(CONF)

    def open_algorithm_descriptions_help(self):
        self.interface_manager.showAlgorithmHelp('')

    def open_mantid_concepts_help(self):
        self.interface_manager.showConceptHelp('')

    def open_mantid_help(self):
        self.interface_manager.showHelpPage('')

    def open_mantid_homepage(self):
        self.interface_manager.showWebPage('https://www.mantidproject.org')

    def open_mantid_forum(self):
        self.interface_manager.showWebPage('https://forum.mantidproject.org/')

    def readSettings(self, settings):
        qapp = QApplication.instance()
        qapp.setAttribute(Qt.AA_UseHighDpiPixmaps)
        if hasattr(Qt, 'AA_EnableHighDpiScaling'):
            qapp.setAttribute(Qt.AA_EnableHighDpiScaling, settings.get('high_dpi_scaling'))

        # get the saved window geometry
        window_size = settings.get('MainWindow/size')
        if not isinstance(window_size, QSize):
            window_size = QSize(*window_size)
        window_pos = settings.get('MainWindow/position')
        if not isinstance(window_pos, QPoint):
            window_pos = QPoint(*window_pos)

        # make sure main window is smaller than the desktop
        desktop = QDesktopWidget()

        # this gives the maximum screen number if the position is off screen
        screen = desktop.screenNumber(window_pos)

        # recalculate the window size
        desktop_geom = desktop.screenGeometry(screen)
        w = min(desktop_geom.size().width(), window_size.width())
        h = min(desktop_geom.size().height(), window_size.height())
        window_size = QSize(w, h)

        # and position it on the supplied desktop screen
        x = max(window_pos.x(), desktop_geom.left())
        y = max(window_pos.y(), desktop_geom.top())
        window_pos = QPoint(x, y)

        # set the geometry
        self.resize(window_size)
        self.move(window_pos)

        # restore window state
        if settings.has('MainWindow/state'):
            self.restoreState(settings.get('MainWindow/state'))
        else:
            self.setWindowState(Qt.WindowMaximized)

        # read in settings for children
        AlgorithmInputHistory().readSettings(settings)
        for widget in self.widgets:
            if hasattr(widget, 'readSettings'):
                widget.readSettings(settings)

    def writeSettings(self, settings):
        settings.set('MainWindow/size', self.size())  # QSize
        settings.set('MainWindow/position', self.pos())  # QPoint
        settings.set('MainWindow/state', self.saveState())  # QByteArray

        # write out settings for children
        AlgorithmInputHistory().writeSettings(settings)
        for widget in self.widgets:
            if hasattr(widget, 'writeSettings'):
                widget.writeSettings(settings)


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


def start_workbench(app, command_line_options):
    """Given an application instance create the MainWindow,
    show it and start the main event loop
    """

    # The ordering here is very delicate. Test thoroughly when
    # changing anything!
    main_window = MainWindow()
    # decorates the excepthook callback with the reference to the main window
    # this is used in case the user wants to terminate the workbench from the error window shown
    sys.excepthook = partial(exception_logger, main_window)

    # Load matplotlib as early as possible and set our defaults
    # Setup our custom backend and monkey patch in custom current figure manager
    main_window.set_splash('Preloading matplotlib')
    from workbench.plotting.config import initialize_matplotlib  # noqa
    initialize_matplotlib()

    # Setup widget layouts etc. mantid.simple cannot be used before this
    # or the log messages don't get through to the widget
    main_window.setup()
    # start mantid
    main_window.set_splash('Initializing mantid framework')
    FrameworkManagerImpl.Instance()
    main_window.post_mantid_init()

    if main_window.splash:
        main_window.splash.hide()

    if command_line_options.script is not None:
        main_window.editor.open_file_in_new_tab(command_line_options.script)
        editor_task = None
        if command_line_options.execute:
            # if the quit flag is not specified, this task reference will be
            # GC'ed, and the task will be finished alongside the GUI startup
            editor_task = main_window.editor.execute_current_async()

        if command_line_options.quit:
            # wait for the code interpreter thread to finish executing the script
            editor_task.join()
            main_window.close()

            # for task exit code descriptions see the classes AsyncTask and TaskExitCode
            return int(editor_task.exit_code) if editor_task else 0

    main_window.show()
    main_window.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
    # Project Recovey on startup
    main_window.project_recovery.repair_checkpoints()
    if main_window.project_recovery.check_for_recover_checkpoint():
        main_window.project_recovery.attempt_recovery()
    else:
        main_window.project_recovery.start_recovery_thread()

    # lift-off!
    return app.exec_()


def main():
    """Main entry point for the application"""

    # setup command line arguments
    parser = argparse.ArgumentParser(description='Mantid Workbench')
    parser.add_argument('script', nargs='?')
    parser.add_argument('-x', '--execute', action='store_true',
                        help='execute the script file given as argument')
    parser.add_argument('-q', '--quit', action='store_true',
                        help='execute the script file with \'-x\' given as argument and then exit')
    # TODO -a or --about: show about dialog and exit
    # TODO -d or --default-settings: start MantidPlot with the default settings
    # DONE -h or --help: show command line options <- free with command line parser
    # TODO -v or --version: print MantidPlot version and release date
    # TODO -r or --revision: print MantidPlot version and release date
    # TODO -s or --silent: start mantidplot without any setup dialogs
    # DONE -x or --execute: execute the script file given as argument
    # DONE -xq or --executeandquit: execute the script file given as argument and then exit MantidPlot
    # this is not a valid short command line option

    try:
        # set up bash completion as a soft dependency
        import argcomplete
        argcomplete.autocomplete(parser)
    except ImportError:
        pass  # silently skip this

    # parse the command line options
    options = parser.parse_args()
    # TODO handle options that don't require starting the workbench e.g. --help --version

    # fix/validate arguments
    if options.script is not None:
        # convert into absolute path
        options.script = os.path.abspath(os.path.expanduser(options.script))
        if not os.path.exists(options.script):
            # TODO should be logged
            print('script "{}" does not exist'.format(options.script))
            options.script = None

    app = initialize()
    # the default sys check interval leads to long lags
    # when request scripts to be aborted
    setswitchinterval(SYSCHECK_INTERVAL)
    exit_value = 0
    try:
        exit_value = start_workbench(app, options)
    except BaseException:
        # We count this as a crash
        import traceback
        # This is type of thing we want to capture and have reports
        # about. Prints to stderr as we can't really count on anything
        # else
        traceback.print_exc(file=ORIGINAL_STDERR)
        exit_value = -1
    finally:
        ORIGINAL_SYS_EXIT(exit_value)


if __name__ == '__main__':
    main()
