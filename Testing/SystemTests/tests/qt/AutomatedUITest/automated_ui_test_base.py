# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Base class for system tests that drive a real Qt interface.

These tests replace manual test guides: they build the genuine view/presenter/model, click real
widgets with ``QTest`` and run real algorithms, mocking only what would otherwise block on user
input. This module holds the parts that are not specific to any one interface, so a suite for a
second interface can subclass ``AutomatedUITestBase`` and only supply its own setup.

Four framework details shape everything here, and each of them contradicts the obvious approach:

1. ``AsyncTask`` invokes its success/error callbacks *on the worker thread*, and Mantid's
   ``Observable.notify_subscribers`` marshals back to the GUI thread with a **blocking** queued
   connection. A plain ``worker.join()`` therefore deadlocks - the worker waits for the GUI thread
   to pump events while the GUI thread waits in ``join()``. See ``wait_for_async_task``.
2. Every test class in a module runs in the *same* Python process (``systestrunner.py`` uses
   ``TestRunner.start_in_current_process``), so isolation has to happen per class in
   ``setUp``/``tearDown``, and module scope must stay cheap and free of side effects - in
   particular there must be no module-level ``QApplication``.
3. ``MantidSystemTest`` gives one ``runTest()`` per class, but a manual test guide is dozens of
   small observations. ``runTest`` is therefore a template method and subclasses implement
   ``_run_checks``; see ``check`` for how a failure in one observation avoids hiding the rest.
4. The system test collector picks up *every* ``MantidSystemTest`` subclass visible in a module,
   including imported ones. This class stays uncollected only because it is abstract.
"""

import os
import shutil
import sys
import tempfile
import time
import traceback
from abc import ABCMeta, abstractmethod
from contextlib import contextmanager
from unittest import mock

import systemtesting

# make the shared helpers importable when this file is run by hand rather than through CMake, which
# puts this directory on PYTHONPATH via PYUNITTEST_PYTHONPATH_EXTRA
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
if _THIS_DIR not in sys.path:
    sys.path.insert(0, _THIS_DIR)

# offscreen rendering must be requested before the first QApplication is built, and this module is
# imported before any test constructs one
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")


def qt_is_available():
    """Whether this build can construct widgets at all. Used by ``skipTests`` so a framework-only
    build reports a clean skip instead of an import error during collection."""
    try:
        import qtpy.QtWidgets  # noqa: F401
        import mantidqt  # noqa: F401
    except ImportError:
        return False
    return True


def ensure_qapp():
    """Return the process-wide QApplication, creating it on first use.

    Deliberately lazy. Creating it at module scope - as some older Qt system tests do - builds a
    QApplication inside the collector process too, because the runner imports every test module
    before running anything.

    Built through ``mantidqt``'s helper rather than ``QApplication(sys.argv)`` because that helper
    calls ``setup_library_paths()`` first; without it Qt's plugin path is unset and constructing a
    Mantid C++ widget such as ``FileFinderWidget`` fails with a bare ``RuntimeError``.
    """
    from mantidqt.utils.qt.testing import get_application

    return get_application()


class AutomatedUITestBase(systemtesting.MantidSystemTest, metaclass=ABCMeta):
    """Harness for a Qt interface system test.

    Subclasses implement ``_run_checks`` and, usually, override ``setUp`` to build their interface
    after calling ``super().setUp()``.
    """

    # organisation/application used for the isolated settings store. Deliberately unlike
    # workbench's ("mantidproject"/"mantidworkbench") so a test can never reach the real ini file.
    SETTINGS_ORG = "mantidproject-systemtests"
    SETTINGS_APP = "AutomatedUITest"

    def __init__(self):
        super(AutomatedUITestBase, self).__init__()
        self._failures = []
        self._saved_qsettings_state = None
        self._saved_data_dirs = None
        self.tmp_root = None

    # ------------------------------------------------------------------ test protocol

    @abstractmethod
    def _run_checks(self) -> None:
        """Perform the checks for this test.

        Write this as a readable sequence of ``self._check_*`` calls following the manual test
        guide, so a reader can line the two up.
        """

    def skipTests(self):
        return not qt_is_available()

    def runTest(self):
        self._failures = []
        self._run_checks()
        if self._failures:
            raise AssertionError(
                f"{len(self._failures)} UI check(s) failed in {type(self).__name__}:\n  - " + "\n  - ".join(self._failures)
            )

    @contextmanager
    def check(self, label):
        """Record an assertion failure instead of raising, so the rest of the checks still run.

        A manual test guide is a long list of largely independent observations, and a suite that
        stopped at the first one would need as many CI runs as there are regressions. ``label``
        should name the step being checked, e.g. "Test 1 / Calibration step 12 (three prm files)".

        Use plain ``self.assertX`` instead for preconditions - "the run number resolved", "the
        worker finished" - where continuing would only produce a cascade of meaningless failures.
        """
        try:
            yield
        except AssertionError as exc:
            self._failures.append(f"{label}: {exc}")
        except Exception as exc:
            self._failures.append(f"{label}: unexpected {type(exc).__name__}: {exc}\n{traceback.format_exc()}")

    def record_failure(self, label, message):
        """Record a failure directly, for checks that are not expressed as an assertion."""
        self._failures.append(f"{label}: {message}")

    # ------------------------------------------------------------------ lifecycle

    def setUp(self):
        ensure_qapp()
        self._isolate_qsettings()
        self.tmp_root = tempfile.mkdtemp(prefix="automated_ui_test_")
        self.addCleanup(shutil.rmtree, self.tmp_root, True)

    def tearDown(self):
        # setUp may have failed part way through, so nothing here may assume it completed
        from qt_interaction_helpers import close_all_figures, process_events

        try:
            close_all_figures()
            process_events(2)
        finally:
            self._restore_data_search_dirs()
            self._restore_qsettings()
            self._clear_ads()

    def cleanup(self):
        # called by the framework after validation, outside the setUp/tearDown pair
        self._clear_ads()

    @staticmethod
    def _clear_ads():
        """Clear the ADS between classes.

        Necessary rather than tidy: the interfaces keep hidden ``__``-prefixed workspaces that
        would otherwise be silently reused by the next class in the same process.
        """
        from mantid.api import AnalysisDataService as ADS

        ADS.clear()

    # ------------------------------------------------------------------ settings isolation

    def _isolate_qsettings(self):
        """Redirect every bare ``QSettings()`` in this process into a temporary ini file.

        Must run before the interface is constructed - interfaces typically read and write their
        stored settings during ``__init__``.

        Patching the interface's own settings helper is not enough: helpers are normally imported
        by name into many modules, so one patch intercepts none of them, and it would still miss
        the settings that Qt widgets (e.g. a file finder's last-used directory) write for
        themselves. Redirecting at the QSettings level catches all of it.
        """
        from qtpy.QtCore import QCoreApplication, QSettings

        self._settings_dir = tempfile.mkdtemp(prefix="automated_ui_qsettings_")
        self._saved_qsettings_state = (
            QSettings.defaultFormat(),
            QCoreApplication.organizationName(),
            QCoreApplication.applicationName(),
        )
        # on Windows the default format is the registry, which setPath cannot redirect
        QSettings.setDefaultFormat(QSettings.IniFormat)
        QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, self._settings_dir)
        QCoreApplication.setOrganizationName(self.SETTINGS_ORG)
        QCoreApplication.setApplicationName(self.SETTINGS_APP)

    def _restore_qsettings(self):
        if self._saved_qsettings_state is None:
            return
        from qtpy.QtCore import QCoreApplication, QSettings

        fmt, org, app = self._saved_qsettings_state
        QSettings.setDefaultFormat(fmt)
        QCoreApplication.setOrganizationName(org)
        QCoreApplication.setApplicationName(app)
        self._saved_qsettings_state = None
        shutil.rmtree(self._settings_dir, ignore_errors=True)

    def settings_file(self):
        """Path of the isolated ini file, for asserting on what an interface stored."""
        return os.path.join(self._settings_dir, self.SETTINGS_ORG, f"{self.SETTINGS_APP}.ini")

    # ------------------------------------------------------------------ data search directories

    def add_data_search_dir(self, directory):
        """Add a directory to Mantid's data search path for the duration of this test.

        This is how a test gets the interface's own file finder to resolve fabricated run files:
        the interface then loads them exactly as it would a real run, rather than the test reaching
        past the view to inject a workspace.
        """
        from mantid.kernel import config

        if self._saved_data_dirs is None:
            self._saved_data_dirs = config["datasearch.directories"]
        config.appendDataSearchDir(directory)

    def _restore_data_search_dirs(self):
        if self._saved_data_dirs is None:
            return
        from mantid.kernel import config

        config["datasearch.directories"] = self._saved_data_dirs
        self._saved_data_dirs = None

    # ------------------------------------------------------------------ waiting

    def wait_for_async_task(self, worker, timeout=1800.0, what="worker"):
        """Block until an ``AsyncTask`` finishes, pumping the event loop throughout.

        ``AsyncTask.run`` calls its success/error callback on the worker thread. Mantid presenters
        typically notify their observers from that callback, and ``Observable.notify_subscribers``
        goes through ``QAppThreadCall(..., blocking=True)``, i.e. a ``BlockingQueuedConnection``.
        The worker is therefore parked until the GUI thread runs its event loop - so a plain
        ``worker.join()`` here would deadlock both threads. Short joins interleaved with
        ``processEvents`` are what make it terminate.
        """
        from qt_interaction_helpers import process_events

        if worker is None:
            raise RuntimeError(f"no {what} was started - was the click rejected by validation?")
        deadline = time.time() + timeout
        while worker.is_alive():
            process_events()
            worker.join(0.01)
            if time.time() > deadline:
                raise RuntimeError(f"{what} did not finish within {timeout}s")
        # drain the callbacks the worker queued on its way out (e.g. re-enabling controls)
        process_events(3)

    # ------------------------------------------------------------------ modal dialogs

    def patch_error_messages(self, modules, helper_name="create_error_message"):
        """Stop a module's error popup from blocking, and record what it would have said.

        An unattended test that pops a modal message box hangs until the suite times out, so every
        module that can raise one must be neutralised before the first click. The recorded text
        goes to ``self.message_box_messages``, which turns "the interface rejected this input" from
        a hang into something a check can assert on.

        ``modules`` are module paths that import the popup helper *by name*, so each one needs its
        own patch - patching the definition would not affect any of them.
        """
        self.message_box_messages = []

        def record(_parent, message):
            self.message_box_messages.append(str(message))

        for module in modules:
            patcher = mock.patch(f"{module}.{helper_name}", side_effect=record)
            patcher.start()
            self.addCleanup(patcher.stop)

    def patch_confirmation_box(self, module, answer):
        """Answer a blocking ``QMessageBox.warning`` confirmation prompt with ``answer``.

        Distinct from ``patch_error_messages`` because the interface *uses* the return value to
        decide whether to continue, so the test has to choose which button the user pressed.
        """
        if not hasattr(self, "message_box_messages"):
            self.message_box_messages = []

        def record(*args, **_kwargs):
            # QMessageBox.warning(parent, title, text, buttons, default_button)
            self.message_box_messages.append(str(args[2]) if len(args) > 2 else "")
            return answer

        patcher = mock.patch(f"{module}.QMessageBox")
        mocked = patcher.start()
        mocked.warning.side_effect = record
        mocked.Ok = answer
        mocked.Cancel = object()
        self.addCleanup(patcher.stop)
        return mocked

    @contextmanager
    def algorithm_dialog_runs(self, presenter_module, run_algorithm):
        """Emulate a user accepting one of the ``InterfaceManager`` algorithm dialogs.

        Several tabs open a generated algorithm dialog (``SetGoniometer``, ``LoadSampleShape``,
        ``SetSampleMaterial``, ...) and then react to the algorithm finishing. Showing a real
        dialog would block, so ``InterfaceManager`` is replaced and its dialog is told that, when
        shown, it should run ``run_algorithm`` for real and then notify the observer the presenter
        registered - which is exactly the sequence the real dialog drives on accept. Everything
        downstream of that (the presenter's ``finishHandle``, the queued redraw) runs unmodified.
        """
        from qt_interaction_helpers import process_events

        with mock.patch(f"{presenter_module}.InterfaceManager") as manager:
            dialog = manager.return_value.createDialogFromName.return_value

            def show_dialog():
                run_algorithm()
                dialog.addAlgorithmObserver.call_args.args[0].finishHandle()

            dialog.show.side_effect = show_dialog
            yield dialog
        # finishHandle emits a queued signal so the redraw happens on the GUI thread
        process_events(3)

    # ------------------------------------------------------------------ log capture

    @contextmanager
    def captured_logs(self, level="notice"):
        """Capture Mantid log output, for the checks that assert on what was reported.

        Yields a holder whose ``text`` attribute is filled in when the block exits, rather than the
        underlying buffer: ``mantid.utils.logging.capture_logs`` closes its buffer on the way out,
        so reading it afterwards - which is the natural way to write the assertion - raises
        "I/O operation on closed file". Reading ``text`` after the block is the supported use.
        """
        from mantid.utils.logging import capture_logs

        class _CapturedLogs:
            text = ""

            def getvalue(self):
                return self.text

        holder = _CapturedLogs()
        with capture_logs(level=level) as logs:
            try:
                yield holder
            finally:
                holder.text = logs.getvalue()

    # ------------------------------------------------------------------ filesystem assertions

    @staticmethod
    def files_under(directory, extension=None):
        """Every file below ``directory``, as paths relative to it, sorted.

        Used for the save-layout checks, where the point is both which files appear and which
        directories they appear in.
        """
        if not os.path.isdir(directory):
            return []
        found = []
        for root, _dirs, files in os.walk(directory):
            for name in files:
                if extension is None or name.endswith(extension):
                    found.append(os.path.relpath(os.path.join(root, name), directory))
        return sorted(found)

    @staticmethod
    def basenames_under(directory, extension=None):
        return sorted(os.path.basename(p) for p in AutomatedUITestBase.files_under(directory, extension))
