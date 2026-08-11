# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import errno
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from mantidqt.utils.qt.qsettings_staging import QSettingsStagingEligibility, QSettingsStagingReason
from mantidqt.utils.qt.qsettings_staging_session import (
    COMPLETED_FILENAME,
    CopyBackFileResult,
    CopyBackStatus,
    QSettingsStagingSessionManager,
    StagingFinalizationError,
)
import mantidqt.utils.qt.qsettings_staging_session as staging_module


class FakeCoordinator:
    def __init__(self, _path: Path):
        self.unlocked = False

    def tryLock(self, _timeout: int = 0) -> bool:
        return True

    def unlock(self) -> None:
        self.unlocked = True


class QSettingsStagingCopyBackTest(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.config_root = self.root / "config"
        self.cache_root = self.root / "cache"
        self.config_root.mkdir()
        self.cache_root.mkdir()
        self.canonical_directory = self.config_root / "mantidproject"
        self.canonical_file = self.canonical_directory / "mantidworkbench.ini"
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

    def lock_factory(self, path: Path) -> FakeCoordinator:
        coordinator = FakeCoordinator(path)
        self.coordinators.append(coordinator)
        return coordinator

    def prepare(self, canonical_contents: bytes | None = b"original"):
        if canonical_contents is not None:
            self.canonical_directory.mkdir(exist_ok=True)
            self.canonical_file.write_bytes(canonical_contents)
        return QSettingsStagingSessionManager(self.eligibility, lock_factory=self.lock_factory).prepare()

    def result_for(self, finalization, relative_path="mantidproject/mantidworkbench.ini"):
        return next(result for result in finalization.files if result.relative_path == Path(relative_path))

    def test_unchanged_file_is_not_opened_for_update(self):
        session = self.prepare()
        original_stat = self.canonical_file.stat()

        with patch(
            "mantidqt.utils.qt.qsettings_staging_session._copy_changed_file",
            side_effect=AssertionError("unchanged file must not be copied"),
        ):
            finalization = session.finalize()

        self.assertTrue(finalization.successful)
        self.assertEqual(CopyBackStatus.UNCHANGED, self.result_for(finalization).status)
        self.assertEqual(original_stat.st_ino, self.canonical_file.stat().st_ino)
        self.assertEqual(original_stat.st_mtime_ns, self.canonical_file.stat().st_mtime_ns)
        self.assertTrue((session.staging_root / COMPLETED_FILENAME).is_file())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_changed_file_is_overwritten_directly_and_preserves_mode_and_inode(self):
        session = self.prepare()
        self.canonical_file.chmod(0o640)
        original_inode = self.canonical_file.stat().st_ino
        staged_file = session.staging_root / "mantidproject/mantidworkbench.ini"
        staged_file.write_bytes(b"changed")

        finalization = session.finalize()

        self.assertTrue(finalization.successful)
        self.assertEqual(CopyBackStatus.COPIED, self.result_for(finalization).status)
        self.assertEqual(b"changed", self.canonical_file.read_bytes())
        self.assertEqual(original_inode, self.canonical_file.stat().st_ino)
        self.assertEqual(0o640, self.canonical_file.stat().st_mode & 0o777)
        self.assertEqual(["mantidworkbench.ini"], sorted(path.name for path in self.canonical_directory.iterdir()))
        self.assertTrue(self.coordinators[0].unlocked)

    def test_changed_qt_project_native_settings_are_copied_back(self):
        native_file = self.config_root / "QtProject.conf"
        native_file.write_bytes(b"native original")
        session = self.prepare()
        (session.staging_root / "QtProject.conf").write_bytes(b"native changed")
        (session.staging_root / "QtProject.conf.lock").write_bytes(b"local lock artifact")

        finalization = session.finalize()

        self.assertTrue(finalization.successful)
        self.assertEqual(CopyBackStatus.COPIED, self.result_for(finalization, "QtProject.conf").status)
        self.assertEqual(b"native changed", native_file.read_bytes())
        self.assertFalse((self.config_root / "QtProject.conf.lock").exists())

    def test_currently_identical_files_are_not_opened_for_update(self):
        session = self.prepare()
        staged_file = session.staging_root / "mantidproject/mantidworkbench.ini"
        staged_file.write_bytes(b"converged")
        self.canonical_file.write_bytes(b"converged")
        converged_stat = self.canonical_file.stat()

        with patch.object(os, "ftruncate", side_effect=AssertionError("identical file must not be truncated")):
            finalization = session.finalize()

        self.assertTrue(finalization.successful)
        self.assertEqual(CopyBackStatus.ALREADY_SYNCHRONIZED, self.result_for(finalization).status)
        self.assertEqual(converged_stat.st_ino, self.canonical_file.stat().st_ino)
        self.assertEqual(converged_stat.st_mtime_ns, self.canonical_file.stat().st_mtime_ns)

    def test_external_change_is_a_conflict(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"staged change")
        self.canonical_file.write_bytes(b"external change")

        finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.CONFLICT, self.result_for(finalization).status)
        self.assertEqual(b"external change", self.canonical_file.read_bytes())
        self.assertFalse((session.staging_root / COMPLETED_FILENAME).exists())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_new_nested_settings_file_is_created_privately(self):
        session = self.prepare(canonical_contents=None)
        staged_file = session.staging_root / "mantidproject/nested/Mantid Reduction.ini"
        staged_file.parent.mkdir()
        staged_file.write_bytes(b"reduction settings")

        finalization = session.finalize()

        copied = self.config_root / "mantidproject/nested/Mantid Reduction.ini"
        self.assertTrue(finalization.successful)
        self.assertEqual(CopyBackStatus.COPIED, self.result_for(finalization, "mantidproject/nested/Mantid Reduction.ini").status)
        self.assertEqual(b"reduction settings", copied.read_bytes())
        self.assertEqual(0o600, copied.stat().st_mode & 0o777)

    def test_unexpected_settings_organization_fails_finalization_and_retains_session(self):
        session = self.prepare()
        unexpected = session.staging_root / "Other Organization/settings.ini"
        unexpected.parent.mkdir()
        unexpected.write_text("[General]\nvalue=unexpected\n")

        finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertIn("outside mantidproject", finalization.error)
        self.assertTrue(unexpected.exists())
        self.assertFalse((session.staging_root / COMPLETED_FILENAME).exists())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_error_reporter_settings_are_never_seeded_or_copied_back(self):
        self.canonical_directory.mkdir()
        reporter_file = self.canonical_directory / "mantid-error-reporter.ini"
        reporter_file.write_bytes(b"canonical reporter settings")
        session = self.prepare()
        staged_reporter_file = session.staging_root / "mantidproject/mantid-error-reporter.ini"
        self.assertFalse(staged_reporter_file.exists())

        staged_reporter_file.write_bytes(b"transient staged reporter settings")
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed workbench settings")
        finalization = session.finalize()

        self.assertTrue(finalization.successful)
        self.assertEqual(b"canonical reporter settings", reporter_file.read_bytes())
        self.assertEqual(b"changed workbench settings", self.canonical_file.read_bytes())
        self.assertNotIn(Path("mantidproject/mantid-error-reporter.ini"), {result.relative_path for result in finalization.files})

    def test_missing_seeded_file_fails_without_deleting_canonical_file(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").unlink()

        finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization).status)
        self.assertEqual(b"original", self.canonical_file.read_bytes())

    def test_staged_symlink_fails_without_following_it(self):
        session = self.prepare()
        outside = self.root / "outside"
        outside.write_bytes(b"outside")
        (session.staging_root / "mantidproject/linked.ini").symlink_to(outside)

        finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertIn("symlink", finalization.error)
        self.assertEqual(b"outside", outside.read_bytes())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_short_write_is_reported_and_retains_session(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed")
        real_write = os.write
        writes = 0

        def short_write(descriptor, contents):
            nonlocal writes
            writes += 1
            if writes == 1:
                return real_write(descriptor, contents[:1])
            return 0

        with patch.object(os, "write", side_effect=short_write):
            finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization).status)
        self.assertTrue((session.staging_root / "mantidproject/mantidworkbench.ini").exists())
        self.assertFalse((session.staging_root / COMPLETED_FILENAME).exists())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_full_disk_write_error_is_reported(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed")

        with patch.object(os, "write", side_effect=OSError(errno.ENOSPC, "disk full")):
            finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization).status)
        self.assertIn("disk full", self.result_for(finalization).error)

    def test_partial_multi_file_finalization_reports_each_outcome(self):
        self.canonical_directory.mkdir()
        first_canonical = self.canonical_directory / "first.ini"
        second_canonical = self.canonical_directory / "second.ini"
        first_canonical.write_bytes(b"first original")
        second_canonical.write_bytes(b"second original")
        session = self.prepare(canonical_contents=None)
        (session.staging_root / "mantidproject/first.ini").write_bytes(b"first changed")
        (session.staging_root / "mantidproject/second.ini").write_bytes(b"second changed")
        real_copy = staging_module._copy_changed_file

        def fail_second(canonical_root, canonical_path, staged_path, relative_path, baseline_hash):
            if relative_path.name == "second.ini":
                return CopyBackFileResult(relative_path, CopyBackStatus.FAILED, "injected failure")
            return real_copy(canonical_root, canonical_path, staged_path, relative_path, baseline_hash)

        with patch.object(staging_module, "_copy_changed_file", side_effect=fail_second):
            finalization = session.finalize()

        statuses = {result.relative_path.name: result.status for result in finalization.files}
        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.COPIED, statuses["first.ini"])
        self.assertEqual(CopyBackStatus.FAILED, statuses["second.ini"])
        self.assertEqual(b"first changed", first_canonical.read_bytes())
        self.assertEqual(b"second original", second_canonical.read_bytes())

    def test_completion_marker_failure_retains_successfully_copied_session(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed")

        with patch.object(staging_module, "_write_completion_marker", side_effect=OSError("marker failure")):
            finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual("cannot mark staging session complete: marker failure", finalization.error)
        self.assertEqual(CopyBackStatus.COPIED, self.result_for(finalization).status)
        self.assertTrue(session.staging_root.exists())
        self.assertTrue(self.coordinators[0].unlocked)

    def test_fsync_error_is_reported_and_retains_session(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed")

        with patch.object(os, "fsync", side_effect=OSError("fsync failed")):
            finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization).status)
        self.assertIn("fsync failed", self.result_for(finalization).error)
        self.assertFalse((session.staging_root / COMPLETED_FILENAME).exists())

    def test_close_error_is_reported_and_retains_session(self):
        session = self.prepare()
        (session.staging_root / "mantidproject/mantidworkbench.ini").write_bytes(b"changed")
        real_close = os.close

        def fail_canonical_close(descriptor):
            descriptor_path = Path(f"/proc/self/fd/{descriptor}").resolve()
            real_close(descriptor)
            if descriptor_path == self.canonical_file:
                raise OSError("close failed")

        with patch.object(os, "close", side_effect=fail_canonical_close):
            finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization).status)
        self.assertIn("close failed", self.result_for(finalization).error)
        self.assertFalse((session.staging_root / COMPLETED_FILENAME).exists())

    def test_canonical_parent_symlink_is_rejected(self):
        session = self.prepare(canonical_contents=None)
        staged_file = session.staging_root / "mantidproject/nested/new.ini"
        staged_file.parent.mkdir()
        staged_file.write_bytes(b"new settings")
        outside = self.root / "outside-directory"
        outside.mkdir()
        self.canonical_directory.mkdir()
        (self.canonical_directory / "nested").symlink_to(outside, target_is_directory=True)

        finalization = session.finalize()

        self.assertFalse(finalization.successful)
        self.assertEqual(CopyBackStatus.FAILED, self.result_for(finalization, "mantidproject/nested/new.ini").status)
        self.assertEqual([], list(outside.iterdir()))

    def test_released_session_cannot_be_finalized(self):
        session = self.prepare()
        session.abort()

        with self.assertRaises(StagingFinalizationError):
            session.finalize()


if __name__ == "__main__":
    unittest.main()
