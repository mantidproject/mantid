# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
Defines the QMainWindow of the application and the main() entry point.
"""
import os

from mantid.api import FrameworkManager
from mantid.kernel import ConfigService, logger
from workbench.app import MAIN_WINDOW_OBJECT_NAME, MAIN_WINDOW_TITLE
from workbench.utils.windowfinder import find_window
from workbench.widgets.about.presenter import AboutPresenter
from workbench.widgets.settings.presenter import SettingsPresenter

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
from qtpy.QtCore import (QEventLoop, Qt, QPoint, QSize)  # noqa
from qtpy.QtGui import (QColor, QFontDatabase, QGuiApplication, QIcon, QPixmap)  # noqa
from qtpy.QtWidgets import (QApplication, QDesktopWidget, QFileDialog, QMainWindow,
                            QSplashScreen, QMessageBox)  # noqa
from mantidqt.algorithminputhistory import AlgorithmInputHistory  # noqa
from mantidqt.interfacemanager import InterfaceManager  # noqa
from mantidqt.widgets import manageuserdirectories  # noqa
from mantidqt.widgets.scriptrepository import ScriptRepositoryView  # noqa
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution  # noqa
from mantidqt.utils.qt import (add_actions, create_action, widget_updates_disabled)  # noqa
from mantidqt.project.project import Project  # noqa
from mantidqt.usersubwindowfactory import UserSubWindowFactory  # noqa

from workbench.config import CONF  # noqa
from workbench.plotting.globalfiguremanager import GlobalFigureManager  # noqa
from workbench.utils.windowfinder import find_all_windows_that_are_savable  # noqa
from workbench.utils.workspacehistorygeneration import get_all_workspace_history_from_ads  # noqa
from workbench.projectrecovery.projectrecovery import ProjectRecovery  # noqa
from workbench.utils.recentlyclosedscriptsmenu import RecentlyClosedScriptsMenu # noqa
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback  # noqa
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall  # noqa

# -----------------------------------------------------------------------------
# Splash screen
# -----------------------------------------------------------------------------


def _get_splash_image():
    # gets the width of the screen where the main window was initialised
    width = QGuiApplication.primaryScreen().size().width()
    height = QGuiApplication.primaryScreen().size().height()

    # the proportion of the whole window size for the splash screen
    splash_screen_scaling = 0.25
    return QPixmap(':/images/MantidSplashScreen_4k.jpg').scaled(int(width * splash_screen_scaling),
                                                                int(height * splash_screen_scaling),
                                                                Qt.KeepAspectRatio,
                                                                Qt.SmoothTransformation)


SPLASH = QSplashScreen(_get_splash_image(), Qt.WindowStaysOnTopHint)
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
    # list of custom interfaces that are not qt4/qt5 compatible
    PYTHON_GUI_BLACKLIST = ['Frequency_Domain_Analysis_Old.py']

    def __init__(self):
        QMainWindow.__init__(self)

        # -- instance attributes --
        self.setWindowTitle(MAIN_WINDOW_TITLE)
        self.setObjectName(MAIN_WINDOW_OBJECT_NAME)

        # widgets
        self.messagedisplay = None
        self.ipythonconsole = None
        self.workspacewidget = None
        self.editor = None
        self.algorithm_selector = None
        self.plot_selector = None
        self.interface_manager = None
        self.script_repository = None
        self.widgets = []

        # Widget layout map: required for use in Qt.connection
        self._layout_widget_info = None

        # Menus
        self.file_menu = None
        self.file_menu_actions = None
        self.view_menu = None
        self.view_menu_actions = None
        self.view_menu_layouts = None
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
        self.interface_list = None

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
        # read settings early so that logging level is in place before framework mgr created
        self.messagedisplay.readSettings(CONF)
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
        self.messagedisplay.display.setActiveScript(self.editor.editors.current_tab_filename)
        self.editor.register_plugin()
        self.widgets.append(self.editor)
        self.editor.editors.sig_code_exec_start.connect(self.messagedisplay.script_executing)
        self.editor.editors.sig_file_name_changed.connect(self.messagedisplay.file_name_modified)
        self.editor.editors.sig_current_tab_changed.connect(self.messagedisplay.current_tab_changed)

        self.set_splash("Loading IPython console")
        from workbench.plugins.jupyterconsole import JupyterConsole
        self.ipythonconsole = JupyterConsole(self)
        self.ipythonconsole.register_plugin()
        self.widgets.append(self.ipythonconsole)

        from workbench.plugins.workspacewidget import WorkspaceWidget
        self.workspacewidget = WorkspaceWidget(self)
        self.workspacewidget.register_plugin()
        prompt = CONF.get('project/prompt_on_deleting_workspace')
        self.workspacewidget.workspacewidget.enableDeletePrompt(bool(prompt))
        self.widgets.append(self.workspacewidget)

        # set the link between the algorithm and workspace widget
        self.algorithm_selector.algorithm_selector.set_get_selected_workspace_fn(
            self.workspacewidget.workspacewidget.getSelectedWorkspaceNames)

        # Set up the project, recovery and interface manager objects
        self.project = Project(GlobalFigureManager, find_all_windows_that_are_savable)
        self.project_recovery = ProjectRecovery(globalfiguremanager=GlobalFigureManager,
                                                multifileinterpreter=self.editor.editors,
                                                main_window=self)

        self.interface_executor = PythonCodeExecution()
        self.interface_executor.sig_exec_error.connect(lambda errobj: logger.warning(str(errobj)))
        self.interface_manager = InterfaceManager()

        # uses default configuration as necessary
        self.setup_default_layouts()
        self.create_actions()
        self.readSettings(CONF)
        self.config_updated()

    def post_mantid_init(self):
        """Run any setup that requires mantid
        to have been initialized
        """
        self.redirect_python_warnings()
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
        action_open = create_action(self,
                                    "Open Script",
                                    on_triggered=self.open_file,
                                    shortcut="Ctrl+O")
        action_load_project = create_action(self, "Open Project", on_triggered=self.load_project)
        action_save_script = create_action(self,
                                           "Save Script",
                                           on_triggered=self.save_script,
                                           shortcut="Ctrl+S")
        action_save_script_as = create_action(self,
                                              "Save Script as...",
                                              on_triggered=self.save_script_as)
        action_generate_ws_script = create_action(self,
                                                  "Generate Recovery Script",
                                                  on_triggered=self.generate_script_from_workspaces)
        action_save_project = create_action(self, "Save Project", on_triggered=self.save_project)
        action_save_project_as = create_action(self,
                                               "Save Project as...",
                                               on_triggered=self.save_project_as)
        action_manage_directories = create_action(self,
                                                  "Manage User Directories",
                                                  on_triggered=self.open_manage_directories)
        action_script_repository = create_action(self,
                                                 "Script Repository",
                                                 on_triggered=self.open_script_repository)
        action_settings = create_action(self, "Settings", on_triggered=self.open_settings_window)
        action_quit = create_action(self,
                                    "&Quit",
                                    on_triggered=self.close,
                                    shortcut="Ctrl+Q")
        action_clear_all_memory = create_action(self,
                                                "Clear All Memory",
                                                on_triggered=self.clear_all_memory_action,
                                                shortcut="Ctrl+Shift+L")

        menu_recently_closed_scripts = RecentlyClosedScriptsMenu(self)
        self.editor.editors.sig_tab_closed.connect(menu_recently_closed_scripts.add_script_to_settings)

        self.file_menu_actions = [
            action_open, action_load_project, None, action_save_script, action_save_script_as,
            menu_recently_closed_scripts, action_generate_ws_script, None, action_save_project,
            action_save_project_as, None, action_settings, None, action_manage_directories, None,
            action_script_repository, None, action_clear_all_memory, None, action_quit
        ]

        # view menu
        action_restore_default = create_action(self,
                                               "Restore Default Layout",
                                               on_triggered=self.setup_default_layouts,
                                               shortcut="Shift+F10",
                                               shortcut_context=Qt.ApplicationShortcut)

        self.view_menu_layouts = self.view_menu.addMenu("&User Layouts")
        self.populate_layout_menu()

        self.view_menu_actions = [action_restore_default, None] + self.create_widget_actions()

        # help menu
        action_mantid_help = create_action(self,
                                           "Mantid Help",
                                           on_triggered=self.open_mantid_help,
                                           shortcut='F1',
                                           shortcut_context=Qt.ApplicationShortcut)
        action_algorithm_descriptions = create_action(
            self, 'Algorithm Descriptions', on_triggered=self.open_algorithm_descriptions_help)
        action_mantid_concepts = create_action(self,
                                               "Mantid Concepts",
                                               on_triggered=self.open_mantid_concepts_help)
        action_mantid_homepage = create_action(self,
                                               "Mantid Homepage",
                                               on_triggered=self.open_mantid_homepage)
        action_mantid_forum = create_action(self,
                                            "Mantid Forum",
                                            on_triggered=self.open_mantid_forum)
        action_about = create_action(self, "About Mantid Workbench", on_triggered=self.open_about)

        self.help_menu_actions = [
            action_mantid_help, action_mantid_concepts, action_algorithm_descriptions, None,
            action_mantid_homepage, action_mantid_forum, None, action_about
        ]

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

    def launch_custom_cpp_gui(self, interface_name, submenu=None):
        """Create a new interface window if one does not already exist,
        else show existing window"""
        object_name = 'custom-cpp-interface-' + interface_name
        window = find_window(object_name, QMainWindow)
        if window is None:
            interface = self.interface_manager.createSubWindow(interface_name)
            interface.setObjectName(object_name)
            interface.setAttribute(Qt.WA_DeleteOnClose, True)
            # make indirect interfaces children of workbench
            if submenu == "Indirect":
                interface.setParent(self, interface.windowFlags())
            interface.show()
        else:
            if window.windowState() == Qt.WindowMinimized:
                window.setWindowState(Qt.WindowActive)
            else:
                window.raise_()

    def populate_interfaces_menu(self):
        """Populate then Interfaces menu with all Python and C++ interfaces"""
        self.interfaces_menu.clear()
        interface_dir = ConfigService['mantidqt.python_interfaces_directory']
        self.interface_list = self._discover_python_interfaces(interface_dir)
        self._discover_cpp_interfaces(self.interface_list)

        hidden_interfaces = ConfigService['interfaces.categories.hidden'].split(';')

        keys = list(self.interface_list.keys())
        keys.sort()
        for key in keys:
            if key not in hidden_interfaces:
                submenu = self.interfaces_menu.addMenu(key)
                names = self.interface_list[key]
                names.sort()
                for name in names:
                    if '.py' in name:
                        action = submenu.addAction(name.replace('.py', '').replace('_', ' '))
                        script = os.path.join(interface_dir, name)
                        action.triggered.connect(
                            lambda checked_py, script=script: self.launch_custom_python_gui(script))
                    else:
                        action = submenu.addAction(name)
                        action.triggered.connect(lambda checked_cpp, name=name, key=key: self.
                                                 launch_custom_cpp_gui(name, key))

    def redirect_python_warnings(self):
        """By default the warnings module writes warnings to sys.stderr. stderr is assumed to be
        an error channel so we don't confuse warnings with errors this redirects warnings
        from the warnings module to mantid.logger.warning
        """
        import warnings

        def to_mantid_warning(*args, **kwargs):
            logger.warning(warnings.formatwarning(*args, **kwargs))

        warnings.showwarning = to_mantid_warning

    def _discover_python_interfaces(self, interface_dir):
        """Return a dictionary mapping a category to a set of named Python interfaces"""
        items = ConfigService['mantidqt.python_interfaces'].split()

        # detect the python interfaces
        interfaces = {}
        for item in items:
            key, scriptname = item.split('/')
            if not os.path.exists(os.path.join(interface_dir, scriptname)):
                logger.warning('Failed to find script "{}" in "{}"'.format(
                    scriptname, interface_dir))
                continue
            if scriptname in self.PYTHON_GUI_BLACKLIST:
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

    def populate_layout_menu(self):
        self.view_menu_layouts.clear()
        try:
            layout_dict = CONF.get("MainWindow/user_layouts")
        except KeyError:
            layout_dict = {}
        layout_keys = sorted(layout_dict.keys())
        layout_options = []
        for item in layout_keys:
            layout_options.append(self.create_load_layout_action(item, layout_dict[item]))
        layout_options.append(None)
        action_settings = create_action(self,
                                        "Settings",
                                        on_triggered=self.open_settings_layout_window)
        layout_options.append(action_settings)

        add_actions(self.view_menu_layouts, layout_options)

    def create_load_layout_action(self, layout_name, layout):
        action_load_layout = create_action(self,
                                           layout_name,
                                           on_triggered=lambda: self.restoreState(layout))
        return action_load_layout

    def prep_window_for_reset(self):
        """Function to reset all dock widgets to a state where they can be
        ordered by setup_default_layout"""
        for widget in self.widgets:
            widget.dockwidget.setFloating(False)  # Bring back any floating windows
            self.addDockWidget(Qt.LeftDockWidgetArea, widget.dockwidget)  # Un-tabify all widgets
            widget.toggle_view(False)

    def setup_default_layouts(self):
        """Set the default layouts of the child widgets"""
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
            'width-fraction': [
                0.25,  # column 0 width
                0.50,  # column 1 width
                0.25
            ],  # column 2 width
            'height-fraction': [
                [0.5, 0.5],  # column 0 row heights
                [1.0],  # column 1 row heights
                [1.0]
            ]  # column 2 row heights
        }

        size = self.size()  # Preserve size on reset
        self.arrange_layout(default_layout)
        self.resize(size)

    def arrange_layout(self, layout):
        """Arrange the layout of the child widgets according to the supplied layout"""
        self.prep_window_for_reset()
        widgets_layout = layout['widgets']
        with widget_updates_disabled(self):
            # flatten list
            widgets = [item for column in widgets_layout for row in column for item in row]
            # show everything
            for w in widgets:
                w.toggle_view(True)
            # split everything on the horizontal
            for i in range(len(widgets) - 1):
                first, second = widgets[i], widgets[i + 1]
                self.splitDockWidget(first.dockwidget, second.dockwidget, Qt.Horizontal)
            # now arrange the rows
            for column in widgets_layout:
                for i in range(len(column) - 1):
                    first_row, second_row = column[i], column[i + 1]
                    self.splitDockWidget(first_row[0].dockwidget, second_row[0].dockwidget,
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
        if self.project.is_saving or self.project.is_loading:
            event.ignore()
            self.project.inform_user_not_possible()
            return

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

    def generate_script_from_workspaces(self):
        if not self.workspacewidget.empty_of_workspaces():
            task = BlockingAsyncTaskWithCallback(target=self._generate_script_from_workspaces,
                                                 blocking_cb=QApplication.processEvents)
            task.start()
        else:
            # Tell users they need a workspace to do that
            QMessageBox().warning(None, "No Workspaces!",
                                  "In order to generate a recovery script there needs to be some workspaces.",
                                  QMessageBox.Ok)

    def _generate_script_from_workspaces(self):
        script = "from mantid.simpleapi import *\n\n" + get_all_workspace_history_from_ads()
        QAppThreadCall(self.editor.open_script_in_new_tab)(script)

    def save_project(self):
        self.project.save(CONF)

    def save_project_as(self):
        self.project.open_project_save_dialog(CONF)

    def load_project(self):
        self.project.load()

    def open_manage_directories(self):
        manageuserdirectories.ManageUserDirectories.openManageUserDirectories()

    def open_script_repository(self):
        self.script_repository = ScriptRepositoryView(self)
        self.script_repository.loadScript.connect(self.editor.open_file_in_new_tab)
        self.script_repository.setAttribute(Qt.WA_DeleteOnClose, True)
        self.script_repository.show()

    def open_settings_window(self):
        settings = SettingsPresenter(self)
        settings.show()

    def open_settings_layout_window(self):
        settings = SettingsPresenter(self)
        settings.show()
        settings.general_settings.focus_layout_box()

    def clear_all_memory_action(self):
        """
        Creates Question QMessageBox to check user wants to clear all memory
        when action is pressed from file menu
        """
        msg = QMessageBox(QMessageBox.Question,
                          "Clear All", "All workspaces and windows will be removed.\nAre you sure?")
        msg.addButton(QMessageBox.Ok)
        msg.addButton(QMessageBox.Cancel)
        msg.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        reply = msg.exec()
        if reply == QMessageBox.Ok:
            self.clear_all_memory()

    def clear_all_memory(self):
        """
        Wrapper for call to FrameworkManager to clear all memory
        """
        FrameworkManager.Instance().clear()

    def config_updated(self):
        """
        Updates the widgets that depend on settings from the Workbench Config.
        """
        self.editor.load_settings_from_config(CONF)
        self.project.load_settings_from_config(CONF)
        self.algorithm_selector.refresh()
        self.populate_interfaces_menu()
        self.workspacewidget.refresh_workspaces()

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

    def open_about(self):
        about = AboutPresenter(self)
        about.show()

    def readSettings(self, settings):
        qapp = QApplication.instance()

        # get the saved window geometry
        window_size = settings.get('MainWindow/size')
        if not isinstance(window_size, QSize):
            window_size = QSize(*window_size)
        window_pos = settings.get('MainWindow/position')
        if not isinstance(window_pos, QPoint):
            window_pos = QPoint(*window_pos)
        if settings.has('MainWindow/font'):
            font_string = settings.get('MainWindow/font').split(',')
            font = QFontDatabase().font(font_string[0], font_string[-1], int(font_string[1]))
            qapp.setFont(font)

        # make sure main window is smaller than the desktop
        desktop = QDesktopWidget()

        # this gives the maximum screen number if the position is off screen
        screen = desktop.screenNumber(window_pos)

        # recalculate the window size
        desktop_geom = desktop.availableGeometry(screen)
        w = min(desktop_geom.size().width(), window_size.width())
        h = min(desktop_geom.size().height(), window_size.height())
        window_size = QSize(w, h)

        # and position it on the supplied desktop screen
        x = max(window_pos.x(), desktop_geom.left())
        y = max(window_pos.y(), desktop_geom.top())
        if x + w > desktop_geom.right():
            x = desktop_geom.right() - w
        if y + h > desktop_geom.bottom():
            y = desktop_geom.bottom() - h
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
            if hasattr(widget, 'readSettingsIfNotDone'):
                widget.readSettingsIfNotDone(settings)

    def writeSettings(self, settings):
        settings.set('MainWindow/size', self.size())  # QSize
        settings.set('MainWindow/position', self.pos())  # QPoint
        settings.set('MainWindow/state', self.saveState())  # QByteArray

        # write out settings for children
        AlgorithmInputHistory().writeSettings(settings)
        for widget in self.widgets:
            if hasattr(widget, 'writeSettings'):
                widget.writeSettings(settings)
