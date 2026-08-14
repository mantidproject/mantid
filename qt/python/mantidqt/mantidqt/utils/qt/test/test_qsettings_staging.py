# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import tempfile
from pathlib import Path
import sys
import unittest

if not sys.platform.startswith("linux"):
    raise unittest.SkipTest("QSettings staging is Linux-only")
from unittest.mock import patch

from mantidqt.utils.qt.qsettings_staging import (
    MountInfo,
    MountMatchStatus,
    QSettingsStagingReason,
    discover_qsettings_storage,
    evaluate_qsettings_staging,
    find_mount,
    parse_mountinfo,
    resolve_xdg_paths,
)


class ResolveXdgPathsTest(unittest.TestCase):
    def test_uses_defaults_when_environment_is_empty(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)

            paths = resolve_xdg_paths(environ={}, home=home)

            self.assertEqual(home / ".config", paths.config_root)
            self.assertEqual(home / ".cache", paths.cache_root)

    def test_uses_absolute_environment_overrides(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)
            config = home / "config override"
            cache = home / "cache override"

            paths = resolve_xdg_paths(
                environ={"XDG_CONFIG_HOME": str(config), "XDG_CACHE_HOME": str(cache)},
                home=home,
            )

            self.assertEqual(config, paths.config_root)
            self.assertEqual(cache, paths.cache_root)

    def test_ignores_relative_environment_overrides(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)

            paths = resolve_xdg_paths(
                environ={"XDG_CONFIG_HOME": "relative-config", "XDG_CACHE_HOME": "relative-cache"},
                home=home,
            )

            self.assertEqual(home / ".config", paths.config_root)
            self.assertEqual(home / ".cache", paths.cache_root)

    def test_resolves_roots_that_do_not_exist(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)
            config = home / "missing" / "config"
            cache = home / "missing" / "cache"

            paths = resolve_xdg_paths(
                environ={"XDG_CONFIG_HOME": str(config), "XDG_CACHE_HOME": str(cache)},
                home=home,
            )

            self.assertEqual(config, paths.config_root)
            self.assertEqual(cache, paths.cache_root)
            self.assertFalse(config.exists())
            self.assertFalse(cache.exists())


class ParseMountInfoTest(unittest.TestCase):
    def test_parses_mount_id_point_and_filesystem(self):
        mounts = parse_mountinfo(["36 25 0:32 / /home rw,relatime - nfs4 server:/home rw\n"])

        self.assertEqual((MountInfo(36, Path("/home"), "nfs4"),), mounts)
        self.assertTrue(mounts[0].is_nfs)

    def test_decodes_mountinfo_path_escapes(self):
        mounts = parse_mountinfo([r"42 25 0:40 / /path\040with\011tab\134slash rw - ext4 /dev/sda rw" + "\n"])

        self.assertEqual("/path with\ttab\\slash", str(mounts[0].mount_point))

    def test_ignores_malformed_records(self):
        mounts = parse_mountinfo(
            [
                "not a mountinfo record\n",
                "not-an-id 25 0:32 / /home rw - nfs4 server:/home rw\n",
                "36 25 0:32 / relative rw - nfs4 server:/home rw\n",
            ]
        )

        self.assertEqual((), mounts)


class FindMountTest(unittest.TestCase):
    def test_selects_longest_component_wise_mount(self):
        mounts = (
            MountInfo(1, Path("/"), "ext4"),
            MountInfo(2, Path("/home"), "nfs4"),
            MountInfo(3, Path("/home/user/.cache"), "xfs"),
        )

        config_match = find_mount(Path("/home/user/.config/mantidproject"), mounts)
        cache_match = find_mount(Path("/home/user/.cache/mantidproject"), mounts)

        self.assertEqual(MountMatchStatus.FOUND, config_match.status)
        self.assertEqual(Path("/home"), config_match.mount.mount_point)
        self.assertTrue(config_match.mount.is_nfs)
        self.assertEqual(MountMatchStatus.FOUND, cache_match.status)
        self.assertEqual(Path("/home/user/.cache"), cache_match.mount.mount_point)
        self.assertFalse(cache_match.mount.is_nfs)

    def test_does_not_use_a_partial_path_component_match(self):
        mounts = (MountInfo(1, Path("/home/user"), "nfs4"),)

        match = find_mount(Path("/home/username/.config"), mounts)

        self.assertEqual(MountMatchStatus.NOT_FOUND, match.status)
        self.assertIsNone(match.mount)

    def test_reports_duplicate_most_specific_mounts_as_ambiguous(self):
        mounts = (
            MountInfo(1, Path("/home"), "nfs4"),
            MountInfo(2, Path("/home"), "ext4"),
        )

        match = find_mount(Path("/home/user/.config"), mounts)

        self.assertEqual(MountMatchStatus.AMBIGUOUS, match.status)
        self.assertIsNone(match.mount)


class DiscoverQSettingsStorageTest(unittest.TestCase):
    def test_discovers_config_and_cache_mounts(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)
            config = home / "remote" / "config"
            cache = home / "local" / "cache"
            mountinfo = home / "mountinfo"
            mountinfo.write_text(
                f"1 0 0:1 / / rw - ext4 /dev/root rw\n"
                f"2 1 0:2 / {home / 'remote'} rw - nfs4 server:/remote rw\n"
                f"3 1 0:3 / {home / 'local'} rw - xfs /dev/local rw\n",
                encoding="utf-8",
            )

            discovery = discover_qsettings_storage(
                environ={"XDG_CONFIG_HOME": str(config), "XDG_CACHE_HOME": str(cache)},
                home=home,
                mountinfo_path=mountinfo,
            )

            self.assertEqual(config, discovery.paths.config_root)
            self.assertEqual(cache, discovery.paths.cache_root)
            self.assertEqual("nfs4", discovery.config_mount.mount.filesystem_type)
            self.assertTrue(discovery.config_mount.mount.is_nfs)
            self.assertEqual("xfs", discovery.cache_mount.mount.filesystem_type)
            self.assertFalse(discovery.cache_mount.mount.is_nfs)
            self.assertIsNone(discovery.mountinfo_error)

    def test_reports_unreadable_mountinfo_as_unavailable(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)

            discovery = discover_qsettings_storage(environ={}, home=home, mountinfo_path=home / "missing-mountinfo")

            self.assertEqual(MountMatchStatus.UNAVAILABLE, discovery.config_mount.status)
            self.assertEqual(MountMatchStatus.UNAVAILABLE, discovery.cache_mount.status)
            self.assertEqual("unreadable", discovery.mountinfo_error)

    def test_preserves_an_ambiguous_match(self):
        with tempfile.TemporaryDirectory() as temporary_home:
            home = Path(temporary_home)
            mountinfo = home / "mountinfo"
            mountinfo.write_text(
                f"1 0 0:1 / {home} rw - nfs4 server:/home rw\n2 0 0:2 / {home} rw - ext4 /dev/local rw\n",
                encoding="utf-8",
            )

            discovery = discover_qsettings_storage(environ={}, home=home, mountinfo_path=mountinfo)

            self.assertEqual(MountMatchStatus.AMBIGUOUS, discovery.config_mount.status)
            self.assertEqual(MountMatchStatus.AMBIGUOUS, discovery.cache_mount.status)


class EvaluateQSettingsStagingTest(unittest.TestCase):
    def setUp(self):
        self.temporary_home = tempfile.TemporaryDirectory()
        self.home = Path(self.temporary_home.name)
        self.remote = self.home / "remote"
        self.local = self.home / "local"
        self.remote.mkdir()
        self.local.mkdir()
        self.config = self.remote / "config"
        self.cache = self.local / "cache"
        self.mountinfo = self.home / "mountinfo"
        self.environ = {
            "MANTID_QSETTINGS_STAGING": "1",
            "XDG_CONFIG_HOME": str(self.config),
            "XDG_CACHE_HOME": str(self.cache),
        }

    def tearDown(self):
        self.temporary_home.cleanup()

    def write_mountinfo(self, config_filesystem="nfs4", cache_filesystem="xfs", cache_mount_id=3):
        self.mountinfo.write_text(
            f"1 0 0:1 / / rw - ext4 /dev/root rw\n"
            f"2 1 0:2 / {self.remote} rw - {config_filesystem} server:/remote rw\n"
            f"{cache_mount_id} 1 0:3 / {self.local} rw - {cache_filesystem} /dev/local rw\n",
            encoding="utf-8",
        )

    def evaluate(self, **kwargs):
        return evaluate_qsettings_staging(
            environ=self.environ,
            home=self.home,
            mountinfo_path=self.mountinfo,
            platform_name="linux",
            **kwargs,
        )

    def test_requires_explicit_opt_in_before_resolving_config(self):
        with patch(
            "mantidqt.utils.qt.qsettings_staging.resolve_xdg_config_root",
            side_effect=AssertionError("config must not be resolved"),
        ):
            result = evaluate_qsettings_staging(environ={}, home=self.home, platform_name="linux")

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.DISABLED, result.reason)
        self.assertIsNone(result.config_root)
        self.assertIsNone(result.cache_root)

    def test_rejects_non_linux_before_resolving_config(self):
        with patch(
            "mantidqt.utils.qt.qsettings_staging.resolve_xdg_config_root",
            side_effect=AssertionError("config must not be resolved"),
        ):
            result = evaluate_qsettings_staging(environ=self.environ, home=self.home, platform_name="darwin")

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.UNSUPPORTED_PLATFORM, result.reason)

    def test_non_nfs_config_returns_without_any_cache_access(self):
        self.write_mountinfo(config_filesystem="ext4")
        with (
            patch(
                "mantidqt.utils.qt.qsettings_staging.resolve_xdg_cache_root",
                side_effect=AssertionError("cache must not be resolved"),
            ),
            patch(
                "mantidqt.utils.qt.qsettings_staging._assess_cache_root",
                side_effect=AssertionError("cache must not be inspected"),
            ),
        ):
            result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CONFIG_NOT_NFS, result.reason)
        self.assertEqual("ext4", result.config_filesystem)
        self.assertIsNone(result.cache_root)
        self.assertIsNone(result.cache_filesystem)

    def test_unreadable_mountinfo_returns_without_cache_access(self):
        with patch(
            "mantidqt.utils.qt.qsettings_staging.resolve_xdg_cache_root",
            side_effect=AssertionError("cache must not be resolved"),
        ):
            result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CONFIG_MOUNT_UNAVAILABLE, result.reason)
        self.assertIsNone(result.cache_root)

    def test_missing_config_mount_returns_without_cache_access(self):
        self.mountinfo.write_text("1 0 0:1 / /unrelated rw - ext4 /dev/root rw\n", encoding="utf-8")
        with patch(
            "mantidqt.utils.qt.qsettings_staging.resolve_xdg_cache_root",
            side_effect=AssertionError("cache must not be resolved"),
        ):
            result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CONFIG_MOUNT_NOT_FOUND, result.reason)

    def test_ambiguous_config_mount_returns_without_cache_access(self):
        self.mountinfo.write_text(
            f"1 0 0:1 / {self.remote} rw - nfs4 server:/remote rw\n2 0 0:2 / {self.remote} rw - ext4 /dev/local rw\n",
            encoding="utf-8",
        )
        with patch(
            "mantidqt.utils.qt.qsettings_staging.resolve_xdg_cache_root",
            side_effect=AssertionError("cache must not be resolved"),
        ):
            result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CONFIG_MOUNT_AMBIGUOUS, result.reason)

    def test_reports_nfs_cache_with_a_dedicated_reason(self):
        self.write_mountinfo(cache_filesystem="nfs")

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_IS_NFS, result.reason)
        self.assertEqual("nfs4", result.config_filesystem)
        self.assertEqual("nfs", result.cache_filesystem)

    def test_reports_missing_cache_mount(self):
        self.mountinfo.write_text(f"2 1 0:2 / {self.remote} rw - nfs4 server:/remote rw\n", encoding="utf-8")

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_MOUNT_NOT_FOUND, result.reason)

    def test_reports_ambiguous_cache_mount(self):
        self.mountinfo.write_text(
            f"2 1 0:2 / {self.remote} rw - nfs4 server:/remote rw\n"
            f"3 1 0:3 / {self.local} rw - xfs /dev/local rw\n"
            f"4 1 0:4 / {self.local} rw - ext4 /dev/other rw\n",
            encoding="utf-8",
        )

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_MOUNT_AMBIGUOUS, result.reason)

    def test_reports_identical_roots(self):
        self.write_mountinfo()
        self.environ["XDG_CACHE_HOME"] = str(self.config)

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.ROOTS_NOT_DISTINCT, result.reason)

    def test_reports_identical_mount_ids(self):
        self.write_mountinfo(cache_mount_id=2)

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.MOUNTS_NOT_DISTINCT, result.reason)

    def test_reports_existing_cache_path_that_is_not_a_directory(self):
        self.write_mountinfo()
        self.cache.write_text("not a directory", encoding="utf-8")

        result = self.evaluate()

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_NOT_DIRECTORY, result.reason)

    def test_reports_cache_not_owned_by_current_user(self):
        self.write_mountinfo()
        self.cache.mkdir()

        result = self.evaluate(effective_uid=self.cache.stat().st_uid + 1)

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_NOT_OWNED, result.reason)

    def test_reports_existing_cache_without_write_access(self):
        self.write_mountinfo()
        self.cache.mkdir()

        result = self.evaluate(effective_uid=self.cache.stat().st_uid, access=lambda _path, _mode: False)

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_NOT_WRITABLE, result.reason)

    def test_reports_missing_cache_with_unwritable_existing_parent(self):
        self.write_mountinfo()

        result = self.evaluate(access=lambda _path, _mode: False)

        self.assertFalse(result.active)
        self.assertEqual(QSettingsStagingReason.CACHE_NOT_WRITABLE, result.reason)

    def test_is_eligible_for_nfs_config_and_writable_local_cache_parent(self):
        self.write_mountinfo()

        result = self.evaluate(access=lambda path, _mode: path == self.local)

        self.assertTrue(result.active)
        self.assertEqual(QSettingsStagingReason.ELIGIBLE, result.reason)
        self.assertEqual(self.config, result.config_root)
        self.assertEqual(self.cache, result.cache_root)
        self.assertEqual("nfs4", result.config_filesystem)
        self.assertEqual("xfs", result.cache_filesystem)
        self.assertFalse(self.cache.exists())


if __name__ == "__main__":
    unittest.main()
