# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import argparse
import importlib
import os
import sys
from sys import setswitchinterval
from functools import partial

from mantid.api import FrameworkManagerImpl
from mantid.kernel import ConfigService, UsageService, version_str as mantid_version_str
from mantid.utils import is_required_version
from workbench.config import APPNAME, ORG_DOMAIN, ORGANIZATION
from workbench.plugins.exception_handler import exception_logger
from workbench.widgets.about.presenter import AboutPresenter

from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import QCoreApplication, Qt, qVersion

SYSCHECK_INTERVAL = 50
ORIGINAL_SYS_EXIT = sys.exit
ORIGINAL_STDOUT = sys.stdout
ORIGINAL_STDERR = sys.stderr
STACKTRACE_FILE = 'workbench_stacktrace.txt'


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

        if is_required_version(required_version='5.10.0', version=qVersion()):
            app.setAttribute(Qt.AA_DisableWindowContextHelpButton)

    return app


def initialize():
    """Perform an initialization of the application instance.

        - Patches sys.exit so that it does nothing.
        - Uses WindowsSelectorEventLoop required by Tornado

    :return: A reference to the existing application instance
    """
    if sys.version_info.major >= 3 and sys.platform == 'win32':
        # Tornado requires WindowsSelectorEventLoop
        # https://www.tornadoweb.org/en/stable/#installation
        import asyncio
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

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
    # MainWindow needs to be imported locally to ensure the matplotlib
    # backend is not imported too early.
    from workbench.app.mainwindow import MainWindow

    # The ordering here is very delicate. Test thoroughly when
    # changing anything!
    main_window = MainWindow()

    # Set the mainwindow as the parent for additional QMainWindow instances
    from workbench.config import set_additional_windows_parent
    set_additional_windows_parent(main_window)

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
    # Project Recovery on startup
    main_window.project_recovery.repair_checkpoints()
    if main_window.project_recovery.check_for_recover_checkpoint():
        main_window.project_recovery.attempt_recovery()
    else:
        main_window.project_recovery.start_recovery_thread()

    if not (command_line_options.execute or command_line_options.quit):
        if AboutPresenter.should_show_on_startup():
            AboutPresenter(main_window).show()

    # lift-off!
    return app.exec_()


def main():
    """Main entry point for the application"""
    # Set the global figure manager in matplotlib. Very important this happens first.
    from workbench.plotting.config import init_mpl_gcf
    init_mpl_gcf()

    # setup command line arguments
    parser = argparse.ArgumentParser(description='Mantid Workbench')
    parser.add_argument('script', nargs='?')
    parser.add_argument('-x',
                        '--execute',
                        action='store_true',
                        help='execute the script file given as argument')
    parser.add_argument('-q',
                        '--quit',
                        action='store_true',
                        help='execute the script file with \'-x\' given as argument and then exit')

    try:
        # set up bash completion as a soft dependency
        import argcomplete
        argcomplete.autocomplete(parser)
    except ImportError:
        pass  # silently skip this

    # parse the command line options
    options = parser.parse_args()

    # fix/validate arguments
    if options.script is not None:
        # convert into absolute path
        options.script = os.path.abspath(os.path.expanduser(options.script))
        if not os.path.exists(options.script):
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
        try:
            print_file_path = os.path.join(ConfigService.getAppDataDirectory(), STACKTRACE_FILE)
            with open(print_file_path, 'w') as print_file:
                traceback.print_exc(file=print_file)
        except OSError:
            pass
        exit_value = -1
    finally:
        ORIGINAL_SYS_EXIT(exit_value)


if __name__ == '__main__':
    main()
