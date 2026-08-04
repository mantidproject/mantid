# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import tempfile
from pathlib import Path
import unittest

from mantidqt.utils.qt.qsettings_staging import (
    MountInfo,
    MountMatchStatus,
    discover_qsettings_storage,
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


if __name__ == "__main__":
    unittest.main()
