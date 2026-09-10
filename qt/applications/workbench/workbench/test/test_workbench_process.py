# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from qtpy.QtCore import QSettings

from mantidqt.utils.qt.testing import start_qapplication
from workbench.app import workbench_process


@start_qapplication
class WorkbenchProcessQSettingsStagingTest(unittest.TestCase):
    def setUp(self):
        workbench_process._QSETTINGS_STAGING_WARNING_EMITTED = False

    @patch.object(workbench_process, "_prepare_and_activate_qsettings_staging")
    @patch.object(workbench_process, "_evaluate_qsettings_staging")
    def test_inactive_local_config_does_not_prepare_or_warn(self, evaluate, prepare):
        evaluate.return_value = self._eligibility(False, "config_not_nfs")

        with patch.object(workbench_process, "Logger") as logger:
            self.assertIsNone(workbench_process._prepare_qsettings_staging())

        prepare.assert_not_called()
        logger.assert_not_called()

    @patch.object(workbench_process, "_evaluate_qsettings_staging")
    def test_nfs_cache_warns_only_once(self, evaluate):
        evaluate.return_value = self._eligibility(False, "cache_is_nfs")

        with patch.object(workbench_process, "Logger") as logger:
            workbench_process._prepare_qsettings_staging()
            workbench_process._prepare_qsettings_staging()

        logger.return_value.warning.assert_called_once()

    @patch.object(workbench_process, "_prepare_and_activate_qsettings_staging")
    @patch.object(workbench_process, "_evaluate_qsettings_staging")
    def test_eligible_storage_returns_activated_session(self, evaluate, prepare):
        eligibility = self._eligibility(True, "eligible")
        session = Mock()
        evaluate.return_value = eligibility
        prepare.return_value = (session, None)

        self.assertIs(session, workbench_process._prepare_qsettings_staging())

        prepare.assert_called_once_with(eligibility)

    @patch("mantidqt.utils.qt.qsettings_staging_session.QSettingsStagingSessionManager")
    def test_prepare_and_activate_activates_prepared_session(self, manager):
        eligibility = self._eligibility(True, "eligible")
        session = manager.return_value.prepare.return_value

        result, error = workbench_process._prepare_and_activate_qsettings_staging(eligibility)

        self.assertIs(session, result)
        self.assertIsNone(error)
        manager.assert_called_once_with(eligibility)
        session.activate.assert_called_once_with()

    @patch.object(workbench_process, "_prepare_and_activate_qsettings_staging")
    @patch.object(workbench_process, "_evaluate_qsettings_staging")
    def test_preparation_failure_warns_and_uses_canonical_path(self, evaluate, prepare):
        evaluate.return_value = self._eligibility(True, "eligible")
        prepare.return_value = (None, RuntimeError("cannot acquire coordinator"))

        with patch.object(workbench_process, "Logger") as logger:
            self.assertIsNone(workbench_process._prepare_qsettings_staging())

        logger.return_value.warning.assert_called_once()

    def test_startup_activates_and_verifies_staging_before_launch(self):
        events = []
        session = Mock()
        app = Mock()
        options = SimpleNamespace(script=None)

        with (
            patch.object(workbench_process, "_prepare_qsettings_staging", side_effect=lambda: events.append("stage") or session),
            patch("workbench.plotting.config.init_mpl_gcf", side_effect=lambda: events.append("matplotlib")),
            patch.object(workbench_process, "initialize", side_effect=lambda: events.append("initialize") or app),
            patch.object(workbench_process, "_verify_qsettings_staging", side_effect=lambda _: events.append("verify")),
            patch.object(
                workbench_process,
                "create_and_launch_workbench",
                side_effect=lambda *_: events.append("launch") or workbench_process.WorkbenchLaunchResult(0, True),
            ) as launch,
            patch.object(
                workbench_process, "_sync_and_finalize_qsettings_staging", side_effect=lambda _: events.append("finalize")
            ) as finalize,
            patch.object(workbench_process, "ORIGINAL_SYS_EXIT") as exit_process,
            patch.object(workbench_process.atexit, "register"),
            patch.object(workbench_process, "setswitchinterval"),
        ):
            workbench_process.initialise_qapp_and_launch_workbench(options)

        self.assertEqual(["stage", "matplotlib", "initialize", "verify", "launch", "finalize"], events)
        launch.assert_called_once_with(app, options, session)
        finalize.assert_called_once_with(session)
        exit_process.assert_called_once_with(0)

    def test_inactive_startup_skips_staged_filename_verification(self):
        app = Mock()
        options = SimpleNamespace(script=None)

        with (
            patch.object(workbench_process, "_prepare_qsettings_staging", return_value=None),
            patch("workbench.plotting.config.init_mpl_gcf"),
            patch.object(workbench_process, "initialize", return_value=app),
            patch.object(workbench_process, "_verify_qsettings_staging") as verify,
            patch.object(
                workbench_process,
                "create_and_launch_workbench",
                return_value=workbench_process.WorkbenchLaunchResult(7, True),
            ) as launch,
            patch.object(workbench_process, "ORIGINAL_SYS_EXIT") as exit_process,
            patch.object(workbench_process.atexit, "register"),
            patch.object(workbench_process, "setswitchinterval"),
        ):
            workbench_process.initialise_qapp_and_launch_workbench(options)

        verify.assert_not_called()
        launch.assert_called_once_with(app, options, None)
        exit_process.assert_called_once_with(7)

    def test_unclean_shutdown_aborts_staging_and_preserves_exit_code(self):
        session = Mock()
        app = Mock()
        options = SimpleNamespace(script=None)

        with (
            patch.object(workbench_process, "_prepare_qsettings_staging", return_value=session),
            patch("workbench.plotting.config.init_mpl_gcf"),
            patch.object(workbench_process, "initialize", return_value=app),
            patch.object(workbench_process, "_verify_qsettings_staging"),
            patch.object(
                workbench_process,
                "create_and_launch_workbench",
                return_value=workbench_process.WorkbenchLaunchResult(-1, False),
            ),
            patch.object(workbench_process, "_abort_qsettings_staging") as abort,
            patch.object(workbench_process, "_sync_and_finalize_qsettings_staging") as finalize,
            patch.object(workbench_process, "ORIGINAL_SYS_EXIT") as exit_process,
            patch.object(workbench_process.atexit, "register"),
            patch.object(workbench_process, "setswitchinterval"),
        ):
            workbench_process.initialise_qapp_and_launch_workbench(options)

        abort.assert_called_once_with(session, "Workbench did not complete a clean shutdown")
        finalize.assert_not_called()
        exit_process.assert_called_once_with(-1)

    def test_startup_failure_aborts_staging_and_propagates(self):
        session = Mock()
        options = SimpleNamespace(script=None)

        with (
            patch.object(workbench_process, "_prepare_qsettings_staging", return_value=session),
            patch("workbench.plotting.config.init_mpl_gcf", side_effect=RuntimeError("startup failed")),
            patch.object(workbench_process, "_abort_qsettings_staging") as abort,
            patch.object(workbench_process, "ORIGINAL_SYS_EXIT") as exit_process,
        ):
            with self.assertRaisesRegex(RuntimeError, "startup failed"):
                workbench_process.initialise_qapp_and_launch_workbench(options)

        abort.assert_called_once_with(session, "Workbench startup failed")
        exit_process.assert_not_called()

    def test_clean_staging_syncs_and_finalizes(self):
        session = Mock()
        session.finalize.return_value = SimpleNamespace(successful=True, files=(), error=None)

        with patch("workbench.config.CONF") as conf:
            conf.qsettings.status.return_value = QSettings.NoError
            workbench_process._sync_and_finalize_qsettings_staging(session)

        conf.qsettings.sync.assert_called_once_with()
        session.finalize.assert_called_once_with()
        session.abort.assert_not_called()

    def test_failed_staging_sync_aborts_without_finalizing(self):
        session = Mock(staging_root=Path("/local/cache/session"))

        with (
            patch("workbench.config.CONF") as conf,
            patch.object(workbench_process, "_abort_qsettings_staging") as abort,
        ):
            conf.qsettings.status.return_value = 1
            workbench_process._sync_and_finalize_qsettings_staging(session)

        conf.qsettings.sync.assert_called_once_with()
        abort.assert_called_once()
        session.finalize.assert_not_called()

    def test_finalization_exception_uses_abort_recovery_path(self):
        session = Mock(staging_root=Path("/local/cache/session"))
        session.finalize.side_effect = OSError("copy failed")

        with (
            patch("workbench.config.CONF") as conf,
            patch.object(workbench_process, "_abort_qsettings_staging") as abort,
        ):
            conf.qsettings.status.return_value = QSettings.NoError
            workbench_process._sync_and_finalize_qsettings_staging(session)

        abort.assert_called_once_with(session, "copy-back failed: copy failed")

    def test_incomplete_finalization_warns_with_failed_path(self):
        session = Mock(staging_root=Path("/local/cache/session"))
        failed_file = SimpleNamespace(relative_path=Path("mantidproject/mantidworkbench.ini"), status=SimpleNamespace(value="conflict"))
        session.finalize.return_value = SimpleNamespace(successful=False, files=(failed_file,), error=None)

        with (
            patch("workbench.config.CONF") as conf,
            patch.object(workbench_process, "_warn_about_qsettings_staging") as warning,
        ):
            conf.qsettings.status.return_value = QSettings.NoError
            workbench_process._sync_and_finalize_qsettings_staging(session)

        warning.assert_called_once()
        self.assertIn(str(Path("mantidproject/mantidworkbench.ini")), warning.call_args.args[0])
        self.assertIn(str(Path("/local/cache/session")), warning.call_args.args[0])

    def test_normal_launch_returns_clean_result_after_accepted_close(self):
        app = Mock()
        app.exec.return_value = 5
        main_window = Mock(shutdown_accepted=True, splash=None)
        main_window.project_recovery.check_for_recover_checkpoint.return_value = False
        options = SimpleNamespace(script=None, execute=False, quit=False)
        about = Mock()
        modules = {
            "workbench.app.mainwindow": SimpleNamespace(MainWindow=Mock(return_value=main_window)),
            "workbench.widgets.about.presenter": SimpleNamespace(AboutPresenter=about),
            "workbench.plugins.exception_handler": SimpleNamespace(exception_logger=Mock()),
            "workbench.plotting.config": SimpleNamespace(initialize_matplotlib=Mock()),
        }

        with (
            patch.dict(sys.modules, modules),
            patch("workbench.config.set_additional_windows_parent"),
            patch.object(workbench_process, "FrameworkManagerImpl"),
        ):
            about.should_show_on_startup.return_value = False
            result = workbench_process.create_and_launch_workbench(app, options)

        self.assertEqual(workbench_process.WorkbenchLaunchResult(5, True), result)
        app.exec.assert_called_once_with()

    def test_script_quit_returns_task_exit_code_and_close_result_without_starting_event_loop(self):
        app = Mock()
        task = Mock(exit_code=4)
        main_window = Mock(shutdown_accepted=False, splash=None)
        main_window.editor.execute_current_async.return_value = task
        main_window.close.side_effect = lambda: setattr(main_window, "shutdown_accepted", True)
        options = SimpleNamespace(script=Path("script.py"), execute=True, quit=True)
        modules = {
            "workbench.app.mainwindow": SimpleNamespace(MainWindow=Mock(return_value=main_window)),
            "workbench.widgets.about.presenter": SimpleNamespace(AboutPresenter=Mock()),
            "workbench.plugins.exception_handler": SimpleNamespace(exception_logger=Mock()),
            "workbench.plotting.config": SimpleNamespace(initialize_matplotlib=Mock()),
        }

        with (
            patch.dict(sys.modules, modules),
            patch("workbench.config.set_additional_windows_parent"),
            patch.object(workbench_process, "FrameworkManagerImpl"),
        ):
            result = workbench_process.create_and_launch_workbench(app, options)

        self.assertEqual(workbench_process.WorkbenchLaunchResult(4, True), result)
        task.join.assert_called_once_with()
        main_window.close.assert_called_once_with()
        app.exec.assert_not_called()

    def test_staged_filename_must_match_expected_workbench_settings_file(self):
        session = SimpleNamespace(staging_root=Path("/local/cache/session"))
        expected = session.staging_root / "mantidproject" / "mantidworkbench.ini"

        workbench_process._verify_qsettings_filename(session, expected)

        with self.assertRaises(RuntimeError):
            workbench_process._verify_qsettings_filename(session, Path("/nfs/config/mantidproject/mantidworkbench.ini"))

    @staticmethod
    def _eligibility(active, reason):
        return SimpleNamespace(active=active, reason=SimpleNamespace(value=reason))


if __name__ == "__main__":
    unittest.main()
