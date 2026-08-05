# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from workbench.app import workbench_process


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
            patch.object(workbench_process, "create_and_launch_workbench", side_effect=lambda *_: events.append("launch")) as launch,
            patch.object(workbench_process.atexit, "register"),
            patch.object(workbench_process, "setswitchinterval"),
        ):
            workbench_process.initialise_qapp_and_launch_workbench(options)

        self.assertEqual(["stage", "matplotlib", "initialize", "verify", "launch"], events)
        launch.assert_called_once_with(app, options, session)

    def test_inactive_startup_skips_staged_filename_verification(self):
        app = Mock()
        options = SimpleNamespace(script=None)

        with (
            patch.object(workbench_process, "_prepare_qsettings_staging", return_value=None),
            patch("workbench.plotting.config.init_mpl_gcf"),
            patch.object(workbench_process, "initialize", return_value=app),
            patch.object(workbench_process, "_verify_qsettings_staging") as verify,
            patch.object(workbench_process, "create_and_launch_workbench") as launch,
            patch.object(workbench_process.atexit, "register"),
            patch.object(workbench_process, "setswitchinterval"),
        ):
            workbench_process.initialise_qapp_and_launch_workbench(options)

        verify.assert_not_called()
        launch.assert_called_once_with(app, options, None)

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
