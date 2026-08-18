# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import hashlib
import json
from pathlib import Path
import tempfile
import sys
import unittest

from unittest.mock import patch

from mantidqt.utils.qt.qsettings_staging import QSettingsStagingEligibility, QSettingsStagingReason
from mantidqt.utils.qt.qsettings_staging_session import (
    COMPLETED_FILENAME,
    MANAGER_RELATIVE_PATH,
    MANIFEST_FILENAME,
    CoordinatorUnavailable,
    QSettingsStagingSessionManager,
    StagingPreparationError,
)


class FakeCoordinator:
    def __init__(self, path: Path, acquired: bool = True):
        self.path = path
        self.acquired = acquired
        self.unlocked = False

    def tryLock(self, timeout: int = 0) -> bool:
        return self.acquired

    def unlock(self) -> None:
        self.unlocked = True


@unittest.skipUnless(sys.platform.startswith("linux"), "QSettings staging is Linux-only")
class QSettingsStagingSessionManagerTest(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.config_root = self.root / "config"
        self.cache_root = self.root / "cache"
        self.config_root.mkdir()
        self.cache_root.mkdir()
        self.eligibility = QSettingsStagingEligibility(
            True,
            QSettingsStagingReason.ELIGIBLE,
            config_root=self.config_root,
            cache_root=self.cache_root,
            config_filesystem="nfs4",
            cache_filesystem="ext4",
        )
        self.coordinators = []

    def tearDown(self):
        self.temporary_directory.cleanup()

    def make_coordinator(self, path: Path, acquired: bool = True) -> FakeCoordinator:
        coordinator = FakeCoordinator(path, acquired)
        self.coordinators.append(coordinator)
        return coordinator

    def manager(self, **kwargs) -> QSettingsStagingSessionManager:
        return QSettingsStagingSessionManager(
            self.eligibility,
            lock_factory=kwargs.pop("lock_factory", self.make_coordinator),
            **kwargs,
        )

    def test_prepares_private_unique_session_and_releases_coordinator(self):
        session = self.manager().prepare()

        self.assertEqual(0o700, session.staging_root.stat().st_mode & 0o777)
        self.assertEqual(0o700, (self.cache_root / MANAGER_RELATIVE_PATH).stat().st_mode & 0o777)
        self.assertEqual(self.cache_root / MANAGER_RELATIVE_PATH / "coordinator.lock", self.coordinators[0].path)
        self.assertTrue(self.coordinators[0].unlocked)
        self.assertTrue((session.staging_root / "mantidproject").is_dir())

        other_session = self.manager().prepare()
        self.assertNotEqual(session.staging_root, other_session.staging_root)
        session.abort()
        other_session.abort()

    def test_seeds_subtree_excludes_artifacts_and_records_hash_and_mode(self):
        organization_root = self.config_root / "mantidproject"
        nested_root = organization_root / "nested"
        nested_root.mkdir(parents=True)
        settings = organization_root / "mantidworkbench.ini"
        settings.write_bytes(b"[General]\nvalue=unchanged\n")
        settings.chmod(0o640)
        reporter_settings = organization_root / "mantid-error-reporter.ini"
        reporter_settings.write_bytes(b"[ContactInfo]\nName=Reporter\n")
        secondary = nested_root / "Mantid Reduction.ini"
        secondary.write_bytes(b"[Reduction]\nfacility=SNS\n")
        native_settings = self.config_root / "QtProject.conf"
        native_settings.write_bytes(b"[General]\nstyle=system\n")
        for artifact in ("mantidworkbench.ini.lock", "old.rmlock", "write.tmp", ".nfs123"):
            (organization_root / artifact).write_text("ignored", encoding="utf-8")

        session = self.manager().prepare()

        staged_settings = session.staging_root / "mantidproject/mantidworkbench.ini"
        staged_secondary = session.staging_root / "mantidproject/nested/Mantid Reduction.ini"
        self.assertEqual(settings.read_bytes(), staged_settings.read_bytes())
        self.assertEqual(secondary.read_bytes(), staged_secondary.read_bytes())
        self.assertEqual(native_settings.read_bytes(), (session.staging_root / "QtProject.conf").read_bytes())
        self.assertEqual(0o600, staged_settings.stat().st_mode & 0o777)
        self.assertFalse((session.staging_root / "mantidproject/mantidworkbench.ini.lock").exists())
        self.assertFalse((session.staging_root / "mantidproject/mantid-error-reporter.ini").exists())

        entries = {entry.canonical_relative_path: entry for entry in session.manifest}
        self.assertNotIn(Path("mantidproject/mantid-error-reporter.ini"), entries)
        entry = entries[Path("mantidproject/mantidworkbench.ini")]
        self.assertEqual(hashlib.sha256(settings.read_bytes()).hexdigest(), entry.canonical_sha256)
        self.assertEqual(0o640, entry.canonical_mode)
        self.assertEqual(entry.canonical_relative_path, entry.staged_relative_path)
        manifest = json.loads((session.staging_root / MANIFEST_FILENAME).read_text(encoding="utf-8"))
        self.assertNotIn("value=unchanged", json.dumps(manifest))
        self.assertEqual(3, len(manifest["files"]))
        session.abort()

    def test_does_not_record_error_reporter_when_supplied_as_an_expected_path(self):
        session = self.manager(expected_settings_paths=(Path("mantidproject/mantid-error-reporter.ini"),)).prepare()

        self.assertEqual((), session.manifest)
        session.abort()

    def test_records_expected_file_as_absent_without_creating_it(self):
        session = self.manager().prepare()

        entry = next(entry for entry in session.manifest if entry.canonical_relative_path == Path("mantidproject/mantidworkbench.ini"))
        self.assertEqual(Path("mantidproject/mantidworkbench.ini"), entry.canonical_relative_path)
        self.assertIsNone(entry.canonical_sha256)
        self.assertIsNone(entry.canonical_mode)
        self.assertFalse((session.staging_root / entry.staged_relative_path).exists())
        session.abort()

    def test_coordinator_contention_creates_no_session(self):
        manager = self.manager(lock_factory=lambda path: self.make_coordinator(path, acquired=False))

        with self.assertRaises(CoordinatorUnavailable):
            manager.prepare()

        manager_root = self.cache_root / MANAGER_RELATIVE_PATH
        self.assertEqual([], list(manager_root.glob("session-*")))
        self.assertFalse(self.coordinators[0].unlocked)

    def test_real_qlockfile_coordinator_allows_concurrent_sessions(self):
        first_session = QSettingsStagingSessionManager(self.eligibility).prepare()
        second_session = QSettingsStagingSessionManager(self.eligibility).prepare()

        self.assertNotEqual(first_session.staging_root, second_session.staging_root)
        first_session.abort()
        second_session.abort()

    def test_completed_session_is_cleaned_but_incomplete_and_unrelated_paths_are_retained(self):
        manager_root = self.cache_root / MANAGER_RELATIVE_PATH
        completed = manager_root / "session-completed"
        incomplete = manager_root / "session-incomplete"
        unrelated = manager_root / "unrelated"
        completed.mkdir(parents=True)
        incomplete.mkdir()
        unrelated.mkdir()
        (completed / COMPLETED_FILENAME).touch()
        (completed / "old.ini").touch()

        session = self.manager().prepare()

        self.assertFalse(completed.exists())
        self.assertTrue(incomplete.exists())
        self.assertTrue(unrelated.exists())
        self.assertEqual((incomplete,), session.retained_session_roots)
        session.abort()

    def test_cleanup_does_not_follow_session_symlink(self):
        manager_root = self.cache_root / MANAGER_RELATIVE_PATH
        outside = self.root / "outside"
        outside.mkdir()
        marker = outside / COMPLETED_FILENAME
        marker.touch()
        manager_root.mkdir(parents=True)
        symlink = manager_root / "session-symlink"
        symlink.symlink_to(outside, target_is_directory=True)

        session = self.manager().prepare()

        self.assertTrue(symlink.is_symlink())
        self.assertTrue(marker.exists())
        self.assertEqual((symlink,), session.retained_session_roots)
        session.abort()

    def test_canonical_symlink_aborts_without_reading_target(self):
        organization_root = self.config_root / "mantidproject"
        organization_root.mkdir()
        outside = self.root / "secret"
        outside.write_text("do not copy", encoding="utf-8")
        (organization_root / "linked.ini").symlink_to(outside)

        with self.assertRaises(StagingPreparationError):
            self.manager().prepare()

        self.assertTrue(self.coordinators[0].unlocked)
        self.assertEqual([], list((self.cache_root / MANAGER_RELATIVE_PATH).glob("session-*")))

    def test_failure_partway_through_preparation_removes_only_new_session_and_unlocks(self):
        old_session = self.cache_root / MANAGER_RELATIVE_PATH / "session-old"
        old_session.mkdir(parents=True)
        organization_root = self.config_root / "mantidproject"
        organization_root.mkdir()
        (organization_root / "first.ini").write_text("first", encoding="utf-8")
        (organization_root / "second.ini").write_text("second", encoding="utf-8")

        with (
            patch(
                "mantidqt.utils.qt.qsettings_staging_session._copy_and_hash",
                side_effect=[("first-hash", 0o600), StagingPreparationError("injected failure")],
            ),
            self.assertRaisesRegex(StagingPreparationError, "injected failure"),
        ):
            self.manager().prepare()

        self.assertTrue(old_session.exists())
        self.assertEqual([old_session], list((self.cache_root / MANAGER_RELATIVE_PATH).glob("session-*")))
        self.assertTrue(self.coordinators[0].unlocked)

    def test_cleanup_failure_still_releases_coordinator(self):
        organization_root = self.config_root / "mantidproject"
        organization_root.mkdir()
        (organization_root / "linked.ini").symlink_to(self.root / "missing")

        with (
            patch(
                "mantidqt.utils.qt.qsettings_staging_session._remove_owned_session",
                side_effect=StagingPreparationError("cleanup failed"),
            ),
            self.assertRaisesRegex(StagingPreparationError, "cleanup failed"),
        ):
            self.manager().prepare()

        self.assertTrue(self.coordinators[0].unlocked)

    def test_abort_is_idempotent_and_retains_recovery_session(self):
        session = self.manager().prepare()

        session.abort()
        session.abort()

        self.assertTrue(session.staging_root.exists())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_rejects_paths_outside_mantidproject(self):
        with self.assertRaises(ValueError):
            self.manager(expected_settings_paths=(Path("../outside.ini"),))

    def test_existing_manager_symlink_is_rejected(self):
        manager_root = self.cache_root / MANAGER_RELATIVE_PATH
        outside = self.root / "outside-manager"
        outside.mkdir()
        manager_root.parent.mkdir()
        manager_root.symlink_to(outside, target_is_directory=True)

        with self.assertRaises(StagingPreparationError):
            self.manager().prepare()

        self.assertEqual([], self.coordinators)

    def test_existing_manager_parent_symlink_is_rejected(self):
        manager_parent = self.cache_root / MANAGER_RELATIVE_PATH.parent
        outside = self.root / "outside-parent"
        outside.mkdir()
        manager_parent.symlink_to(outside, target_is_directory=True)

        with self.assertRaises(StagingPreparationError):
            self.manager().prepare()

        self.assertEqual([], self.coordinators)


if __name__ == "__main__":
    unittest.main()
