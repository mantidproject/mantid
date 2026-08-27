# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
#  This file is part of the mantid workbench.

import subprocess
import sys
import textwrap
import unittest


_SETUP = """
from pathlib import Path
import tempfile

from mantidqt.utils.qt.qsettings_staging import QSettingsStagingEligibility, QSettingsStagingReason
from mantidqt.utils.qt.qsettings_staging_session import QSettingsStagingSessionManager, StagingActivationError

temporary_directory = tempfile.TemporaryDirectory()
root = Path(temporary_directory.name)
config_root = root / "config"
cache_root = root / "cache"
config_root.mkdir()
cache_root.mkdir()
eligibility = QSettingsStagingEligibility(
    True,
    QSettingsStagingReason.ELIGIBLE,
    config_root=config_root,
    cache_root=cache_root,
    config_filesystem="nfs4",
    cache_filesystem="ext4",
)
"""


class QSettingsStagingActivationTest(unittest.TestCase):
    def run_in_subprocess(self, body: str) -> None:
        result = subprocess.run(
            [sys.executable, "-c", textwrap.dedent(_SETUP + body)],
            capture_output=True,
            check=False,
            text=True,
        )

        self.assertEqual(0, result.returncode, msg=result.stderr)

    @unittest.skipUnless(sys.platform.startswith("linux"), "QSettings staging is Linux-only")
    def test_explicit_and_default_writes_and_lock_are_redirected(self):
        self.run_in_subprocess(
            """
import ctypes
import os
import struct

from qtpy.QtCore import QCoreApplication, QSettings

canonical_directory = config_root / "mantidproject"
canonical_directory.mkdir()
canonical_file = canonical_directory / "mantidworkbench.ini"
canonical_contents = b"[General]\\nseeded=canonical\\n"
canonical_file.write_bytes(canonical_contents)
canonical_native_file = config_root / "QtProject.conf"
canonical_native_contents = b"[General]\\nseeded=canonical-native\\n"
canonical_native_file.write_bytes(canonical_native_contents)

session = QSettingsStagingSessionManager(eligibility).prepare()
assert not session.active
session.activate()
assert session.active

QSettings.setDefaultFormat(QSettings.IniFormat)
QCoreApplication.setOrganizationName("mantidproject")
QCoreApplication.setApplicationName("mantidworkbench")
expected_file = session.staging_root / "mantidproject/mantidworkbench.ini"
expected_native_file = session.staging_root / "QtProject.conf"

explicit_settings = QSettings(QSettings.IniFormat, QSettings.UserScope, "mantidproject", "mantidworkbench")
assert Path(explicit_settings.fileName()) == expected_file
assert explicit_settings.value("seeded") == "canonical"

native_settings = QSettings(QSettings.NativeFormat, QSettings.UserScope, "QtProject", "")
assert Path(native_settings.fileName()) == expected_native_file
assert native_settings.value("seeded") == "canonical-native"

libc = ctypes.CDLL(None, use_errno=True)
inotify_descriptor = libc.inotify_init1(os.O_NONBLOCK | os.O_CLOEXEC)
IN_ALL_EVENTS = 0x00000FFF
watching = inotify_descriptor >= 0
if watching:
    staged_watch = libc.inotify_add_watch(inotify_descriptor, os.fsencode(expected_file.parent), IN_ALL_EVENTS)
    staged_root_watch = libc.inotify_add_watch(inotify_descriptor, os.fsencode(session.staging_root), IN_ALL_EVENTS)
    canonical_watch = libc.inotify_add_watch(inotify_descriptor, os.fsencode(canonical_directory), IN_ALL_EVENTS)
    canonical_root_watch = libc.inotify_add_watch(inotify_descriptor, os.fsencode(config_root), IN_ALL_EVENTS)
    watching = min(staged_watch, staged_root_watch, canonical_watch, canonical_root_watch) >= 0

explicit_settings.setValue("explicit", "staged")
explicit_settings.sync()
assert explicit_settings.status() == QSettings.NoError

default_settings = QSettings()
assert Path(default_settings.fileName()) == expected_file
default_settings.setValue("default", "staged")
default_settings.sync()
assert default_settings.status() == QSettings.NoError

native_settings.setValue("native", "staged")
native_settings.sync()
assert native_settings.status() == QSettings.NoError

if watching:
    events = os.read(inotify_descriptor, 65536)
    event_header = struct.Struct("iIII")
    names_by_watch = {}
    offset = 0
    while offset < len(events):
        watch, _mask, _cookie, name_length = event_header.unpack_from(events, offset)
        offset += event_header.size
        name = events[offset : offset + name_length].split(b"\\0", 1)[0].decode()
        offset += name_length
        names_by_watch.setdefault(watch, []).append(name)

    assert "mantidworkbench.ini.lock" in names_by_watch.get(staged_watch, []), names_by_watch
    assert "QtProject.conf.lock" in names_by_watch.get(staged_root_watch, []), names_by_watch
    assert names_by_watch.get(canonical_watch, []) == [], names_by_watch
    assert names_by_watch.get(canonical_root_watch, []) == [], names_by_watch
assert canonical_file.read_bytes() == canonical_contents
assert canonical_native_file.read_bytes() == canonical_native_contents
assert sorted(path.name for path in canonical_directory.iterdir()) == ["mantidworkbench.ini"]
assert explicit_settings.value("explicit") == "staged"
assert explicit_settings.value("default") == "staged"
session.abort()
"""
        )

    def test_process_guard_rejects_repeated_activation(self):
        self.run_in_subprocess(
            """
first_session = QSettingsStagingSessionManager(eligibility).prepare()
first_session.activate()
try:
    first_session.activate()
except StagingActivationError:
    pass
else:
    raise AssertionError("the same session activated twice")

first_session.abort()
second_session = QSettingsStagingSessionManager(eligibility).prepare()
try:
    second_session.activate()
except StagingActivationError:
    pass
else:
    raise AssertionError("a second session replaced the process path")
second_session.abort()
"""
        )

    def test_released_session_is_rejected_without_consuming_guard(self):
        self.run_in_subprocess(
            """
released_session = QSettingsStagingSessionManager(eligibility).prepare()
released_session.abort()
try:
    released_session.activate()
except StagingActivationError:
    pass
else:
    raise AssertionError("a released session activated")

active_session = QSettingsStagingSessionManager(eligibility).prepare()
active_session.activate()
assert active_session.active
active_session.abort()
"""
        )

    def test_failed_qt_call_does_not_activate_session_or_consume_guard(self):
        self.run_in_subprocess(
            """
from qtpy import QtCore

real_qsettings = QtCore.QSettings

class FailingQSettings:
    IniFormat = real_qsettings.IniFormat
    NativeFormat = real_qsettings.NativeFormat
    UserScope = real_qsettings.UserScope

    @staticmethod
    def setPath(_format, _scope, _path):
        raise RuntimeError("injected setPath failure")

session = QSettingsStagingSessionManager(eligibility).prepare()
QtCore.QSettings = FailingQSettings
try:
    session.activate()
except RuntimeError as error:
    assert str(error) == "injected setPath failure"
else:
    raise AssertionError("the injected setPath failure was ignored")
assert not session.active

QtCore.QSettings = real_qsettings
session.activate()
assert session.active
session.abort()
"""
        )


if __name__ == "__main__":
    unittest.main()
