# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import argparse
import atexit
import os
import sys
from sys import setswitchinterval
from functools import partial
import multiprocessing

from mantid.api import FrameworkManagerImpl
from mantid.kernel import ConfigService, UsageService, version_str as mantid_version_str
from mantidqt.utils.qt import plugins
import mantidqt.utils.qt as qtutils

# Find Qt plugins for development builds on some platforms
plugins.setup_library_paths()

from qtpy.QtGui import QIcon, QSurfaceFormat
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import QCoreApplication, Qt

# Importing resources loads the data in. This must be imported before the
# QApplication is created or paths to Qt's resources will not be set up correctly
from workbench.app.resources import qCleanupResources
from workbench.config import APPNAME, ORG_DOMAIN, ORGANIZATION
from workbench.widgets.about.presenter import AboutPresenter

# Constants
SYSCHECK_INTERVAL = 50
ORIGINAL_SYS_EXIT = sys.exit
ORIGINAL_STDOUT = sys.stdout
ORIGINAL_STDERR = sys.stderr
STACKTRACE_FILE = "workbench_stacktrace.txt"


def qapplication():
    """Either return a reference to an existing application instance
    or create a new one
    :return: A reference to the QApplication object
    """
    app = QApplication.instance()
    if app is None:
        # share OpenGL contexts across the application
        QCoreApplication.setAttribute(Qt.AA_ShareOpenGLContexts)

        # set global compatability profile for OpenGL
        # We use deprecated OpenGL calls so anything with a profile version >= 3
        # causes widgets like the instrument view to fail to render
        gl_surface_format = QSurfaceFormat.defaultFormat()
        gl_surface_format.setProfile(QSurfaceFormat.CompatibilityProfile)
        gl_surface_format.setSwapBehavior(QSurfaceFormat.DoubleBuffer)
        QSurfaceFormat.setDefaultFormat(gl_surface_format)

        argv = sys.argv[:]
        argv[0] = APPNAME  # replace application name

        app = QApplication(argv)
        app.setOrganizationName(ORGANIZATION)
        app.setOrganizationDomain(ORG_DOMAIN)
        app.setApplicationName(APPNAME)
        app.setApplicationVersion(mantid_version_str())
        # Spin up the usage service and set the name for the usage reporting
        # The report is sent when the FrameworkManager kicks up
        UsageService.setApplicationName(APPNAME)

        app.setAttribute(Qt.AA_UseHighDpiPixmaps)
        if hasattr(Qt, "AA_DisableWindowContextHelpButton"):
            app.setAttribute(Qt.AA_DisableWindowContextHelpButton)

    return app


def initialize():
    """Perform an initialization of the application instance.

        - Patches sys.exit so that it does nothing.
        - Uses WindowsSelectorEventLoop required by Tornado

    :return: A reference to the existing application instance
    """
    if sys.platform == "win32":
        # Tornado requires WindowsSelectorEventLoop
        # https://www.tornadoweb.org/en/stable/#installation
        import asyncio

        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    if sys.platform == "darwin":
        qtutils.force_layer_backing_BigSur()

    app = qapplication()

    # Monkey patching sys.exit so users can't kill
    # the application this way
    def fake_sys_exit(arg=[]):
        pass

    sys.exit = fake_sys_exit

    return app


def initialise_qapp_and_launch_workbench(command_line_options):
    # Set the global figure manager in matplotlib. Very important this happens first.
    from workbench.plotting.config import init_mpl_gcf

    init_mpl_gcf()

    # cleanup static resources at exit
    atexit.register(qCleanupResources)

    # fix/validate arguments
    if command_line_options.script is not None:
        # convert into absolute path
        command_line_options.script = os.path.abspath(os.path.expanduser(command_line_options.script))
        if not os.path.exists(command_line_options.script):
            print('script "{}" does not exist'.format(command_line_options.script))
            command_line_options.script = None

    app = initialize()
    # the default sys check interval leads to long lags
    # when request scripts to be aborted
    setswitchinterval(SYSCHECK_INTERVAL)

    create_and_launch_workbench(app, command_line_options)


def start_error_reporter():
    """
    Used to start the error reporter if the program has segfaulted.
    """
    from mantidqt.dialogs.errorreports import main as errorreports_main

    errorreports_main.main(["--application", APPNAME, "--orgname", ORGANIZATION, "--orgdomain", ORG_DOMAIN])


def create_and_launch_workbench(app, command_line_options):
    """Given an application instance create the MainWindow,
    show it and start the main event loop
    """
    exit_value = 0
    try:
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
        from workbench.plugins.exception_handler import exception_logger

        sys.excepthook = partial(exception_logger, main_window)

        # Load matplotlib as early as possible and set our defaults
        # Setup our custom backend and monkey patch in custom current figure manager
        main_window.set_splash("Preloading matplotlib")
        from workbench.plotting.config import initialize_matplotlib

        initialize_matplotlib()

        # Setup widget layouts etc. mantid.simple cannot be used before this
        # or the log messages don't get through to the widget
        main_window.setup()
        # start mantid
        main_window.set_splash("Initializing mantid framework")
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
        main_window.setWindowIcon(QIcon(":/images/MantidIcon.ico"))
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
        exit_value = app.exec_()
    except BaseException:
        # We count this as a crash
        import traceback

        # This is type of thing we want to capture and have reports
        # about. Prints to stderr as we can't really count on anything
        # else
        traceback.print_exc(file=ORIGINAL_STDERR)
        try:
            print_file_path = os.path.join(ConfigService.getAppDataDirectory(), STACKTRACE_FILE)
            with open(print_file_path, "w") as print_file:
                traceback.print_exc(file=print_file)
        except OSError:
            pass
        exit_value = -1
    finally:
        ORIGINAL_SYS_EXIT(exit_value)


def start(options: argparse.ArgumentParser):
    """Start workbench based on the given options

    :param options: An object describing the command-line arguments passed in
    """
    if options.single_process:
        initialise_qapp_and_launch_workbench(options)
    else:
        # Mantid's FrameworkManagerImpl::Instance Python export uses a process-wide static flag to ensure code
        # only runs once (see Instance function in Framework/PythonInterface/mantid/api/src/Exports/FrameworkManager.cpp).
        # The default start method on Unix ('fork') inherits resources such as these flags from the parent.
        # We require the start method to be 'spawn' so that we do not inherit these resources from the parent process,
        # this is already the default on Windows/macOS.
        # This will mean the relevant 'atexit' code will execute in the child process, and therefore the
        # FrameworkManager and UsageService will be shutdown as expected.
        context = multiprocessing.get_context("spawn")
        workbench_process = context.Process(target=initialise_qapp_and_launch_workbench, args=[options])
        workbench_process.start()
        workbench_process.join()

        # handle exit information
        exit_code = workbench_process.exitcode if workbench_process.exitcode is not None else 1
        if exit_code != 0:
            # start error reporter if requested
            if not options.no_error_reporter:
                start_error_reporter()

            # a signal was emited so raise the signal from the application
            if exit_code < 0:
                import signal

                try:
                    sig_code = signal.Signals(abs(exit_code))
                    name = sig_code.name
                    print("Received signal:", name)
                except ValueError:
                    pass
                signal.raise_signal(sig_code)  # this will exit the mainloop

            # application returned non-zero that wasn't interpreted as a signal, return it as an exit code
            sys.exit(exit_code)
