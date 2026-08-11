# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from pathlib import Path
import subprocess
import sys
import textwrap
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from qtpy.QtCore import QSettings

from mantidqtinterfaces import reduction_application


class ReductionApplicationQSettingsStagingTest(unittest.TestCase):
    def setUp(self):
        reduction_application._QSETTINGS_STAGING_WARNING_EMITTED = False

    def test_reduction_settings_use_explicit_ini_user_scope(self):
        with (
            patch.object(reduction_application, "QSettings") as qsettings,
            patch.object(reduction_application.QCoreApplication, "organizationName", return_value="mantidproject"),
            patch.object(reduction_application.QCoreApplication, "applicationName", return_value="Mantid Reduction"),
        ):
            qsettings.IniFormat = QSettings.IniFormat
            qsettings.UserScope = QSettings.UserScope
            reduction_application._create_application_settings(explicit_ini=True)

        qsettings.assert_called_once_with(QSettings.IniFormat, QSettings.UserScope, "mantidproject", "Mantid Reduction")

    def test_default_reduction_settings_preserve_platform_format(self):
        with (
            patch.object(reduction_application, "QSettings") as qsettings,
            patch.object(reduction_application.QCoreApplication, "organizationName", return_value="legacy-organization"),
            patch.object(reduction_application.QCoreApplication, "applicationName", return_value="Mantid Reduction"),
        ):
            reduction_application._create_application_settings()

        qsettings.assert_called_once_with("legacy-organization", "Mantid Reduction")

    def test_close_saves_through_retained_settings_and_marks_shutdown_clean(self):
        settings = Mock()
        gui = SimpleNamespace(
            _clear_and_restart=False,
            _settings=settings,
            _shutdown_accepted=False,
            _instrument="EQSANS",
            _filename="reduction.xml",
            _recent_files=["reduction.xml"],
            _last_directory=Path("/data"),
            _last_export_directory=Path("/exports"),
        )
        event = Mock()

        with patch.object(reduction_application, "QSettingsChangeAware") as writer_type:
            reduction_application.ReductionGUI.closeEvent(gui, event)

        writer_type.assert_called_once_with(settings)
        self.assertTrue(gui._shutdown_accepted)
        event.accept.assert_called_once_with()

    def test_standalone_start_finalizes_after_clean_close(self):
        events = []
        app = Mock()
        app.exec.return_value = 6
        session = Mock(staging_root=Path("/local/cache/session"))
        settings = Mock()
        reducer = Mock(shutdown_accepted=True, application_settings=settings)

        with (
            patch.object(reduction_application.QApplication, "instance", return_value=None),
            patch.object(reduction_application, "_prepare_qsettings_staging", side_effect=lambda: events.append("stage") or session),
            patch.object(reduction_application, "get_qapplication", side_effect=lambda: events.append("app") or (app, False)),
            patch.object(
                reduction_application,
                "_create_application_settings",
                side_effect=lambda **_: events.append("settings") or settings,
            ) as create_settings,
            patch.object(reduction_application, "ReductionGUI", side_effect=lambda **_: events.append("gui") or reducer) as gui_type,
            patch.object(reduction_application, "_verify_qsettings_staging", side_effect=lambda *_: events.append("verify")),
            patch.object(
                reduction_application,
                "_sync_and_finalize_qsettings_staging",
                side_effect=lambda *_: events.append("finalize"),
            ) as finalize,
            patch.object(reduction_application.sys, "exit") as exit_process,
        ):
            reduction_application.start()

        self.assertEqual(["stage", "app", "settings", "gui", "verify", "finalize"], events)
        app.setOrganizationName.assert_called_once_with("mantidproject")
        app.setApplicationName.assert_called_once_with("Mantid Reduction")
        create_settings.assert_called_once_with(explicit_ini=True)
        gui_type.assert_called_once_with(application_settings=settings)
        finalize.assert_called_once_with(session, reducer.application_settings)
        exit_process.assert_called_once_with(6)

    def test_standalone_unclean_shutdown_aborts(self):
        app = Mock()
        app.exec.return_value = -1
        session = Mock()
        reducer = Mock(shutdown_accepted=False)

        with (
            patch.object(reduction_application.QApplication, "instance", return_value=None),
            patch.object(reduction_application, "_prepare_qsettings_staging", return_value=session),
            patch.object(reduction_application, "get_qapplication", return_value=(app, False)),
            patch.object(reduction_application, "_create_application_settings", return_value=Mock()),
            patch.object(reduction_application, "ReductionGUI", return_value=reducer),
            patch.object(reduction_application, "_verify_qsettings_staging"),
            patch.object(reduction_application, "_abort_qsettings_staging") as abort,
            patch.object(reduction_application.sys, "exit"),
        ):
            reduction_application.start()

        abort.assert_called_once_with(session, "Mantid Reduction did not complete a clean shutdown")

    def test_hosted_start_inherits_workbench_staging_and_does_not_own_session(self):
        app = Mock()
        reducer = Mock()

        with (
            patch.object(reduction_application.QApplication, "instance", return_value=app),
            patch.object(reduction_application, "_prepare_qsettings_staging") as prepare,
            patch.object(reduction_application, "get_qapplication", return_value=(app, True)),
            patch.object(reduction_application, "ReductionGUI", return_value=reducer) as gui_type,
            patch.object(reduction_application.sys, "exit") as exit_process,
        ):
            reduction_application.start()

        prepare.assert_not_called()
        app.setOrganizationName.assert_not_called()
        app.setApplicationName.assert_not_called()
        gui_type.assert_called_once_with(application_settings=None)
        exit_process.assert_not_called()

    def test_failed_sync_aborts_without_finalizing(self):
        session = Mock(staging_root=Path("/local/cache/session"))
        settings = Mock()
        settings.status.return_value = QSettings.AccessError

        with patch.object(reduction_application, "_abort_qsettings_staging") as abort:
            reduction_application._sync_and_finalize_qsettings_staging(session, settings)

        settings.sync.assert_called_once_with()
        abort.assert_called_once()
        session.finalize.assert_not_called()

    def test_staging_preparation_uses_reduction_settings_path(self):
        eligibility = SimpleNamespace(active=True)
        session = Mock()
        manager = Mock()
        manager.prepare.return_value = session

        with (
            patch("mantidqt.utils.qt.qsettings_staging.evaluate_qsettings_staging", return_value=eligibility),
            patch("mantidqt.utils.qt.qsettings_staging_session.QSettingsStagingSessionManager", return_value=manager) as manager_type,
        ):
            result = reduction_application._prepare_qsettings_staging()

        from mantidqt.utils.qt.qsettings_staging_session import QT_PROJECT_SETTINGS_PATH

        manager_type.assert_called_once_with(
            eligibility, expected_settings_paths=(reduction_application.REDUCTION_SETTINGS_PATH, QT_PROJECT_SETTINGS_PATH)
        )
        session.activate.assert_called_once_with()
        self.assertIs(session, result)

    @unittest.skipUnless(sys.platform.startswith("linux"), "QSettings staging is Linux-only")
    def test_real_reduction_settings_round_trip(self):
        result = subprocess.run(
            [
                sys.executable,
                "-c",
                textwrap.dedent(
                    """
                    from pathlib import Path
                    import tempfile

                    from qtpy.QtCore import QCoreApplication, QSettings

                    from mantidqt.utils.qt.qsettings_staging import QSettingsStagingEligibility, QSettingsStagingReason
                    from mantidqt.utils.qt.qsettings_staging_session import (
                        COMPLETED_FILENAME,
                        QT_PROJECT_SETTINGS_PATH,
                        QSettingsStagingSessionManager,
                    )
                    from mantidqtinterfaces import reduction_application

                    with tempfile.TemporaryDirectory() as temporary_directory:
                        root = Path(temporary_directory)
                        config_root = root / "config"
                        cache_root = root / "cache"
                        canonical_directory = config_root / "mantidproject"
                        canonical_directory.mkdir(parents=True)
                        cache_root.mkdir()
                        canonical_file = canonical_directory / "Mantid Reduction.ini"
                        canonical_contents = b"[General]\\ninstrument_name=EQSANS\\nunchanged=canonical\\n"
                        canonical_file.write_bytes(canonical_contents)

                        eligibility = QSettingsStagingEligibility(
                            True,
                            QSettingsStagingReason.ELIGIBLE,
                            config_root=config_root,
                            cache_root=cache_root,
                            config_filesystem="nfs4",
                            cache_filesystem="ext4",
                        )
                        session = QSettingsStagingSessionManager(
                            eligibility,
                            expected_settings_paths=(reduction_application.REDUCTION_SETTINGS_PATH, QT_PROJECT_SETTINGS_PATH),
                        ).prepare()
                        session.activate()

                        QSettings.setDefaultFormat(QSettings.IniFormat)
                        QCoreApplication.setOrganizationName(reduction_application.REDUCTION_ORGANIZATION)
                        QCoreApplication.setApplicationName(reduction_application.REDUCTION_APPLICATION)
                        settings = reduction_application._create_application_settings(explicit_ini=True)
                        expected_staged_file = session.staging_root / reduction_application.REDUCTION_SETTINGS_PATH

                        assert Path(settings.fileName()) == expected_staged_file
                        assert settings.value("instrument_name") == "EQSANS"
                        assert settings.value("unchanged") == "canonical"
                        assert canonical_file.read_bytes() == canonical_contents

                        settings.setValue("instrument_name", "GPSANS")
                        settings.setValue("recent_files", ["one.xml", "two.xml"])
                        reduction_application._sync_and_finalize_qsettings_staging(session, settings)

                        canonical_settings = QSettings(str(canonical_file), QSettings.IniFormat)
                        assert canonical_settings.value("instrument_name") == "GPSANS"
                        assert canonical_settings.value("recent_files", type=list) == ["one.xml", "two.xml"]
                        assert canonical_settings.value("unchanged") == "canonical"
                        assert (session.staging_root / COMPLETED_FILENAME).exists()
                        assert sorted(path.name for path in canonical_directory.iterdir()) == ["Mantid Reduction.ini"]
                    """
                ),
            ],
            capture_output=True,
            check=False,
            text=True,
        )

        self.assertEqual(0, result.returncode, msg=result.stderr)


if __name__ == "__main__":
    unittest.main()
