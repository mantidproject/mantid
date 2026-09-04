# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import argparse
import atexit
from dataclasses import dataclass
import os
import sys
from pathlib import Path
from sys import setswitchinterval
from functools import partial

from mantid.api import FrameworkManagerImpl
from mantid.kernel import ConfigService, Logger, UsageService, version_str as mantid_version_str
from mantidqt.utils.qt import plugins
import mantidqt.utils.qt as qtutils
import mantid.kernel.environment as mtd_env

# Find Qt plugins for development builds on some platforms
plugins.setup_library_paths()

from qtpy.QtGui import QIcon, QSurfaceFormat  # noqa: E402
from qtpy.QtWidgets import QApplication  # noqa: E402
from qtpy.QtCore import QCoreApplication, Qt  # noqa: E402

# Register the workbench Qt resources. This must happen before the QApplication is
# created or paths to Qt's resources will not be set up correctly.
from workbench.app.resource_loader import register_resources, cleanup_resources  # noqa: E402

register_resources()
from workbench.identity import APPNAME, ORG_DOMAIN, ORGANIZATION  # noqa: E402

# Constants
SYSCHECK_INTERVAL = 50
ORIGINAL_SYS_EXIT = sys.exit
_QSETTINGS_STAGING_WARNING_EMITTED = False


@dataclass(frozen=True)
class WorkbenchLaunchResult:
    exit_code: int
    clean_shutdown: bool


def _evaluate_qsettings_staging():
    """Evaluate staging lazily so importing this module has no settings side effects."""
    from mantidqt.utils.qt.qsettings_staging import evaluate_qsettings_staging

    return evaluate_qsettings_staging()


def _prepare_and_activate_qsettings_staging(eligibility):
    """Prepare and activate a staged QSettings session for this process."""
    from mantidqt.utils.qt.qsettings_staging_session import QSettingsStagingSessionManager, StagingPreparationError

    try:
        session = QSettingsStagingSessionManager(eligibility).prepare()
    except StagingPreparationError as error:
        return None, error

    session.activate()
    return session, None


def _warn_about_qsettings_staging(message):
    global _QSETTINGS_STAGING_WARNING_EMITTED
    if _QSETTINGS_STAGING_WARNING_EMITTED:
        return

    Logger("Mantid Workbench").warning(message)
    _QSETTINGS_STAGING_WARNING_EMITTED = True


def _prepare_qsettings_staging():
    """Activate eligible QSettings staging, otherwise retain the canonical path."""
    eligibility = _evaluate_qsettings_staging()
    if not eligibility.active:
        reason = eligibility.reason.value
        if reason == "cache_is_nfs":
            _warn_about_qsettings_staging(
                "QSettings staging was requested, but the XDG cache directory is also on NFS. "
                "Workbench will use the canonical configuration directory. Use a user-owned local "
                "XDG_CACHE_HOME or unset MANTID_QSETTINGS_STAGING."
            )
        elif reason not in {"disabled", "unsupported_platform", "config_not_nfs"}:
            _warn_about_qsettings_staging(
                f"QSettings staging was requested but is unavailable ({reason}). Workbench will use the canonical configuration directory."
            )
        return None

    session, error = _prepare_and_activate_qsettings_staging(eligibility)
    if error is not None:
        _warn_about_qsettings_staging(
            f"QSettings staging preparation failed ({error}). Workbench will use the canonical configuration directory."
        )
    return session


def _verify_qsettings_filename(session, filename):
    expected = (session.staging_root / "mantidproject" / "mantidworkbench.ini").resolve(strict=False)
    actual = Path(filename).resolve(strict=False)
    if actual != expected:
        raise RuntimeError(f"QSettings staging activation selected {actual}, expected {expected}")


def _verify_qsettings_staging(session):
    # Import only after QSettings.setPath has redirected UserScope IniFormat.
    from workbench.config import CONF

    _verify_qsettings_filename(session, CONF.filename)


def _abort_qsettings_staging(session, reason):
    """Release the staging coordinator and retain the session for recovery."""
    try:
        session.abort()
    except Exception as error:
        reason = f"{reason}; coordinator release also failed: {error}"
    _warn_about_qsettings_staging(
        f"QSettings staging was not copied back ({reason}). Recoverable files remain under {session.staging_root}."
    )


def _sync_and_finalize_qsettings_staging(session):
    """Synchronize Workbench settings and copy a clean staged session back."""
    from qtpy.QtCore import QSettings
    from workbench.config import CONF

    try:
        CONF.qsettings.sync()
        status = CONF.qsettings.status()
        if status != QSettings.NoError:
            _abort_qsettings_staging(session, f"QSettings sync failed with status {status}")
            return
    except Exception as error:
        _abort_qsettings_staging(session, f"QSettings sync failed: {error}")
        return

    try:
        finalization = session.finalize()
    except Exception as error:
        _abort_qsettings_staging(session, f"copy-back failed: {error}")
        return

    if not finalization.successful:
        failed_paths = ", ".join(
            f"{result.relative_path} ({result.status.value})"
            for result in finalization.files
            if result.status.value in {"conflict", "failed"}
        )
        detail = finalization.error or failed_paths or "unknown finalization error"
        _warn_about_qsettings_staging(
            f"QSettings staging copy-back was incomplete ({detail}). Recoverable files remain under {session.staging_root}."
        )


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
        # Don't try to use the system GTK palette instead apply the standard theme palette
        if mtd_env.is_linux():
            app.setPalette(app.style().standardPalette())
        app.setOrganizationName(ORGANIZATION)
        app.setOrganizationDomain(ORG_DOMAIN)
        app.setApplicationName(APPNAME)
        app.setApplicationVersion(mantid_version_str())
        # Spin up the usage service and set the name for the usage reporting
        # The report is sent when the FrameworkManager kicks up
        UsageService.setApplicationName(APPNAME)

        if hasattr(Qt, "AA_DisableWindowContextHelpButton"):
            app.setAttribute(Qt.AA_DisableWindowContextHelpButton)

    return app


def initialize():
    """Perform an initialization of the application instance.

        - Patches sys.exit so that it does nothing.
        - Uses WindowsSelectorEventLoop required by Tornado

    :return: A reference to the existing application instance
    """
    if mtd_env.is_windows():
        # Tornado requires WindowsSelectorEventLoop
        # https://www.tornadoweb.org/en/stable/#installation
        import asyncio

        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    if mtd_env.is_mac():
        qtutils.force_layer_backing_BigSur()

    app = qapplication()

    # Monkey patching sys.exit so users can't kill
    # the application this way
    def fake_sys_exit(arg=[]):
        pass

    sys.exit = fake_sys_exit

    return app


def create_and_launch_workbench(app, command_line_options, qsettings_staging_session=None):
    """Given an application instance create the MainWindow,
    show it and start the main event loop

    The optional staging session remains referenced by this stack frame for the
    lifetime of the event loop. Clean-shutdown finalization is wired separately.
    """
    try:
        # MainWindow needs to be imported locally to ensure the matplotlib
        # backend is not imported too early.
        from workbench.app.mainwindow import MainWindow
        from workbench.widgets.about.presenter import AboutPresenter
        from workbench.widgets.update_notification.presenter import UpdateNotificationPresenter

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

        notify_update_popup = ConfigService.getString("CheckMantidVersion.NotifyUpdateOnStartup") in ("1", "On", "true", "True")
        if notify_update_popup:
            # Prevent FrameworkManagerImpl's constructor from also firing its own unobserved CheckMantidVersion run.
            # Below config change is in-memory only and does not write to .properties file.
            ConfigService.setString("CheckMantidVersion.OnStartup", "0")

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
                exit_code = int(editor_task.exit_code) if editor_task else 0
                return WorkbenchLaunchResult(exit_code, main_window.shutdown_accepted)

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

            if notify_update_popup:
                main_window.update_notifier = UpdateNotificationPresenter(main_window)
                main_window.update_notifier.check_for_update()

        # lift-off!
        exit_code = app.exec()
        return WorkbenchLaunchResult(exit_code, main_window.shutdown_accepted)
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
        return WorkbenchLaunchResult(-1, False)


def initialise_qapp_and_launch_workbench(command_line_options):
    # QSettings staging must be active before anything can import workbench.config.
    qsettings_staging_session = _prepare_qsettings_staging()

    try:
        # Set the global figure manager before any other matplotlib initialization.
        from workbench.plotting.config import init_mpl_gcf

        init_mpl_gcf()

        # cleanup static resources at exit
        atexit.register(cleanup_resources)

        # fix/validate arguments
        if command_line_options.script is not None:
            # convert into absolute path
            command_line_options.script = os.path.abspath(os.path.expanduser(command_line_options.script))
            if not os.path.exists(command_line_options.script):
                print('script "{}" does not exist'.format(command_line_options.script))
                command_line_options.script = None

        app = initialize()
        if qsettings_staging_session is not None:
            _verify_qsettings_staging(qsettings_staging_session)

        # the default sys check interval leads to long lags
        # when request scripts to be aborted
        setswitchinterval(SYSCHECK_INTERVAL)

        result = create_and_launch_workbench(app, command_line_options, qsettings_staging_session)
    except BaseException:
        if qsettings_staging_session is not None:
            _abort_qsettings_staging(qsettings_staging_session, "Workbench startup failed")
        raise

    if qsettings_staging_session is not None:
        if result.clean_shutdown:
            _sync_and_finalize_qsettings_staging(qsettings_staging_session)
        else:
            _abort_qsettings_staging(qsettings_staging_session, "Workbench did not complete a clean shutdown")
    ORIGINAL_SYS_EXIT(result.exit_code)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("script", nargs="?")
    parser.add_argument("-x", "--execute", action="store_true", help="execute the script file given as argument")
    parser.add_argument("-q", "--quit", action="store_true", help="execute the script file with '-x' given as argument and then exit")
    options = parser.parse_args()
    initialise_qapp_and_launch_workbench(options)
