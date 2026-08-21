# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""Prepare private local sessions for out-of-place QSettings writes."""

from collections.abc import Callable, Iterable, Iterator
from contextlib import contextmanager
from dataclasses import dataclass, field
from enum import Enum
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
from threading import Lock
from typing import Protocol
from uuid import uuid4

from mantidqt.utils.qt.qsettings_staging import QSettingsStagingEligibility


MANAGER_RELATIVE_PATH = Path("mantidproject/qsettings-stage")
MANIFEST_FILENAME = ".qsettings-staging-manifest.json"
COMPLETED_FILENAME = ".qsettings-staging-complete"
COORDINATOR_FILENAME = "coordinator.lock"
QT_PROJECT_SETTINGS_PATH = Path("QtProject.conf")
DEFAULT_SETTINGS_PATHS = (Path("mantidproject/mantidworkbench.ini"), QT_PROJECT_SETTINGS_PATH)
ERROR_REPORTER_SETTINGS_PATH = Path("mantidproject/mantid-error-reporter.ini")

_PRIVATE_DIRECTORY_MODE = 0o700
_PRIVATE_FILE_MODE = 0o600
_ACTIVATION_LOCK = Lock()
_QSETTINGS_PATH_ACTIVATED = False


class _Coordinator(Protocol):
    def tryLock(self, timeout: int = 0) -> bool: ...

    def unlock(self) -> None: ...


class StagingPreparationError(RuntimeError):
    """Raised when a local staging session cannot be prepared safely."""


class CoordinatorUnavailable(StagingPreparationError):
    """Raised when another process owns the local staging coordinator."""


class StagingActivationError(RuntimeError):
    """Raised when the process-global QSettings path cannot be activated."""


class StagingFinalizationError(RuntimeError):
    """Raised when a staging session cannot enter finalization."""


class CopyBackStatus(Enum):
    """Outcome for one staged settings path."""

    UNCHANGED = "unchanged"
    ALREADY_SYNCHRONIZED = "already_synchronized"
    COPIED = "copied"
    CONFLICT = "conflict"
    FAILED = "failed"


@dataclass(frozen=True)
class CopyBackFileResult:
    """Copy-back outcome without exposing settings values."""

    relative_path: Path
    status: CopyBackStatus
    error: str | None = None


@dataclass(frozen=True)
class QSettingsStagingFinalization:
    """Result of finalizing every file in a staging session."""

    successful: bool
    files: tuple[CopyBackFileResult, ...]
    error: str | None = None


@dataclass(frozen=True)
class StagedSettingsFile:
    """Startup state required to compare and copy back one settings file."""

    canonical_relative_path: Path
    staged_relative_path: Path
    canonical_sha256: str | None
    canonical_mode: int | None


@dataclass
class PreparedQSettingsSession:
    """A seeded staging session with a private local lifetime."""

    canonical_root: Path
    staging_root: Path
    manifest: tuple[StagedSettingsFile, ...]
    retained_session_roots: tuple[Path, ...]
    _coordinator_path: Path = field(repr=False)
    _lock_factory: Callable[[Path], _Coordinator] = field(repr=False)
    _coordinator: _Coordinator | None = field(default=None, repr=False)
    _released: bool = field(default=False, init=False, repr=False)
    _active: bool = field(default=False, init=False, repr=False)

    @property
    def active(self) -> bool:
        """Return whether this session installed the process QSettings path."""
        return self._active

    def activate(self) -> None:
        """Redirect user-scope INI and native settings to this session exactly once.

        The caller must invoke this before constructing any QSettings object.
        Existing instances cannot be detected reliably, so startup ordering is
        part of this method's contract.
        """
        global _QSETTINGS_PATH_ACTIVATED

        with _ACTIVATION_LOCK:
            if self._released:
                raise StagingActivationError("cannot activate a released QSettings staging session")
            if self._active or _QSETTINGS_PATH_ACTIVATED:
                raise StagingActivationError("the process QSettings staging path has already been activated")

            from qtpy.QtCore import QSettings

            QSettings.setPath(QSettings.NativeFormat, QSettings.UserScope, str(self.staging_root))
            QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, str(self.staging_root))
            _QSETTINGS_PATH_ACTIVATED = True
            self._active = True

    def abort(self) -> None:
        """Release ownership while retaining this session for recovery."""
        if not self._released:
            self._released = True

    def finalize(self) -> QSettingsStagingFinalization:
        """Copy changed staged files directly to canonical storage.

        This operation is deliberately not crash-atomic: it overwrites each
        destination through its own descriptor without an adjacent temporary
        file, rename, unlink, or QSettings operation.
        """
        if self._released:
            raise StagingFinalizationError("cannot finalize a released QSettings staging session")
        coordinator = self._lock_factory(self._coordinator_path)
        if not coordinator.tryLock(0):
            raise StagingFinalizationError("QSettings staging coordinator is busy; staged settings were retained")
        try:
            return _finalize_session(self.canonical_root, self.staging_root, self.manifest)
        finally:
            coordinator.unlock()
            self._released = True


class QSettingsStagingSessionManager:
    """Create one private staging session; coordinate only short preparation and finalization critical sections."""

    def __init__(
        self,
        eligibility: QSettingsStagingEligibility,
        lock_factory: Callable[[Path], _Coordinator] | None = None,
        expected_settings_paths: Iterable[Path] = DEFAULT_SETTINGS_PATHS,
        effective_uid: int | None = None,
    ):
        if not eligibility.active or eligibility.config_root is None or eligibility.cache_root is None:
            raise ValueError("an active eligibility result with config and cache roots is required")
        self._canonical_root = eligibility.config_root
        self._cache_root = eligibility.cache_root
        self._lock_factory = _create_qlockfile if lock_factory is None else lock_factory
        self._expected_settings_paths = tuple(_validated_settings_path(path) for path in expected_settings_paths)
        self._effective_uid = (getattr(os, "geteuid", lambda: 0)()) if effective_uid is None else effective_uid

    def prepare(self) -> PreparedQSettingsSession:
        """Acquire the coordinator briefly, seed a unique session, release the coordinator, and record its manifest."""
        _ensure_directory(self._cache_root, self._effective_uid, private_if_created=True)
        manager_parent = self._cache_root / MANAGER_RELATIVE_PATH.parent
        _ensure_directory(manager_parent, self._effective_uid, private_if_created=True)
        manager_root = manager_parent / MANAGER_RELATIVE_PATH.name
        _ensure_directory(manager_root, self._effective_uid, private_if_created=True, force_private=True)

        coordinator = self._lock_factory(manager_root / COORDINATOR_FILENAME)
        if not coordinator.tryLock(0):
            raise CoordinatorUnavailable(f"QSettings staging coordinator is already held under {manager_root}")

        session_root: Path | None = None
        try:
            retained_sessions = _cleanup_completed_sessions(manager_root, self._effective_uid)
            session_root = manager_root / f"session-{uuid4().hex}"
            session_root.mkdir(mode=_PRIVATE_DIRECTORY_MODE)
            manifest = _seed_session(
                self._canonical_root,
                session_root,
                self._expected_settings_paths,
            )
            _write_manifest(session_root, manifest)
            prepared = PreparedQSettingsSession(
                canonical_root=self._canonical_root,
                staging_root=session_root,
                manifest=manifest,
                retained_session_roots=retained_sessions,
                _coordinator_path=manager_root / COORDINATOR_FILENAME,
                _lock_factory=self._lock_factory,
            )
            coordinator.unlock()
            return prepared
        except BaseException:
            try:
                if session_root is not None:
                    _remove_owned_session(session_root, manager_root, self._effective_uid)
            finally:
                coordinator.unlock()
            raise


def _create_qlockfile(path: Path) -> _Coordinator:
    # Keep Qt out of discovery and import-only launcher paths. The first actual
    # preparation is the first point at which QLockFile is needed.
    from qtpy.QtCore import QLockFile

    return QLockFile(str(path))


def _validated_settings_path(path: Path) -> Path:
    path = Path(path)
    if path.is_absolute() or ".." in path.parts or not path.parts:
        raise ValueError(f"settings path must be relative: {path}")
    if path != QT_PROJECT_SETTINGS_PATH and path.parts[0] != "mantidproject":
        raise ValueError(f"settings path must be below mantidproject or be {QT_PROJECT_SETTINGS_PATH}: {path}")
    return path


def _ensure_directory(path: Path, effective_uid: int, private_if_created: bool, force_private: bool = False) -> None:
    path = Path(path)
    created = False
    try:
        if not path.exists():
            path.mkdir(mode=_PRIVATE_DIRECTORY_MODE, parents=True)
            created = True
        path_stat = path.lstat()
    except OSError as error:
        raise StagingPreparationError(f"cannot prepare staging directory {path}: {error}") from error

    if stat.S_ISLNK(path_stat.st_mode) or not stat.S_ISDIR(path_stat.st_mode):
        raise StagingPreparationError(f"staging path is not a directory: {path}")
    if path_stat.st_uid != effective_uid:
        raise StagingPreparationError(f"staging directory is not owned by the current user: {path}")
    if (created and private_if_created) or force_private:
        try:
            path.chmod(_PRIVATE_DIRECTORY_MODE)
        except OSError as error:
            raise StagingPreparationError(f"cannot make staging directory private: {path}") from error


def _cleanup_completed_sessions(manager_root: Path, effective_uid: int) -> tuple[Path, ...]:
    retained = []
    try:
        children = tuple(manager_root.iterdir())
    except OSError as error:
        raise StagingPreparationError(f"cannot inspect staging manager directory: {manager_root}") from error

    for child in children:
        if not child.name.startswith("session-"):
            continue
        try:
            child_stat = child.lstat()
        except OSError:
            retained.append(child)
            continue
        if stat.S_ISLNK(child_stat.st_mode) or not stat.S_ISDIR(child_stat.st_mode) or child_stat.st_uid != effective_uid:
            retained.append(child)
            continue
        completion_marker = child / COMPLETED_FILENAME
        try:
            marker_stat = completion_marker.lstat()
        except OSError:
            retained.append(child)
            continue
        if not stat.S_ISREG(marker_stat.st_mode) or marker_stat.st_uid != effective_uid:
            retained.append(child)
            continue
        _remove_owned_session(child, manager_root, effective_uid)
    return tuple(retained)


def _remove_owned_session(session_root: Path, manager_root: Path, effective_uid: int) -> None:
    try:
        root_stat = session_root.lstat()
    except FileNotFoundError:
        return
    if (
        session_root.parent != manager_root
        or not session_root.name.startswith("session-")
        or stat.S_ISLNK(root_stat.st_mode)
        or not stat.S_ISDIR(root_stat.st_mode)
        or root_stat.st_uid != effective_uid
    ):
        raise StagingPreparationError(f"refusing to remove unvalidated staging path: {session_root}")
    shutil.rmtree(session_root)


def _seed_session(
    canonical_root: Path,
    session_root: Path,
    expected_settings_paths: tuple[Path, ...],
) -> tuple[StagedSettingsFile, ...]:
    staged_organization_root = session_root / "mantidproject"
    staged_organization_root.mkdir(mode=_PRIVATE_DIRECTORY_MODE)
    source_organization_root = canonical_root / "mantidproject"
    entries: dict[Path, StagedSettingsFile] = {}

    if source_organization_root.exists() or source_organization_root.is_symlink():
        source_stat = source_organization_root.lstat()
        if stat.S_ISLNK(source_stat.st_mode) or not stat.S_ISDIR(source_stat.st_mode):
            raise StagingPreparationError(f"canonical settings root is not a real directory: {source_organization_root}")
        _seed_directory(source_organization_root, staged_organization_root, canonical_root, session_root, entries)

    for relative_path in expected_settings_paths:
        if _is_excluded_settings_path(relative_path):
            continue
        if relative_path not in entries:
            entries[relative_path] = _seed_expected_file(canonical_root, session_root, relative_path)
    return tuple(entries[path] for path in sorted(entries))


def _seed_expected_file(canonical_root: Path, session_root: Path, relative_path: Path) -> StagedSettingsFile:
    source = canonical_root / relative_path
    if not source.exists() and not source.is_symlink():
        return StagedSettingsFile(relative_path, relative_path, None, None)

    source_stat = source.lstat()
    if stat.S_ISLNK(source_stat.st_mode) or not stat.S_ISREG(source_stat.st_mode):
        raise StagingPreparationError(f"canonical settings path is not a regular file: {source}")
    staged = session_root / relative_path
    staged.parent.mkdir(mode=_PRIVATE_DIRECTORY_MODE, parents=True, exist_ok=True)
    digest, canonical_mode = _copy_and_hash(source, staged)
    return StagedSettingsFile(relative_path, relative_path, digest, canonical_mode)


def _seed_directory(
    source_directory: Path,
    staged_directory: Path,
    canonical_root: Path,
    session_root: Path,
    entries: dict[Path, StagedSettingsFile],
) -> None:
    try:
        children = tuple(source_directory.iterdir())
    except OSError as error:
        raise StagingPreparationError(f"cannot inspect canonical settings directory: {source_directory}") from error

    for source in children:
        relative_path = source.relative_to(canonical_root)
        if _is_excluded_settings_path(relative_path) or _is_excluded_artifact(source.name):
            continue
        source_stat = source.lstat()
        if stat.S_ISLNK(source_stat.st_mode):
            raise StagingPreparationError(f"refusing to follow a canonical settings symlink: {source}")
        staged = staged_directory / source.name
        if stat.S_ISDIR(source_stat.st_mode):
            staged.mkdir(mode=_PRIVATE_DIRECTORY_MODE)
            _seed_directory(source, staged, canonical_root, session_root, entries)
        elif stat.S_ISREG(source_stat.st_mode):
            digest, canonical_mode = _copy_and_hash(source, staged)
            entries[relative_path] = StagedSettingsFile(
                canonical_relative_path=relative_path,
                staged_relative_path=staged.relative_to(session_root),
                canonical_sha256=digest,
                canonical_mode=canonical_mode,
            )


def _is_excluded_artifact(name: str) -> bool:
    lowered = name.lower()
    return (
        lowered == MANIFEST_FILENAME
        or lowered == COMPLETED_FILENAME
        or lowered == "qsettings-stage"
        or lowered.startswith(".nfs")
        or lowered.endswith((".lock", ".rmlock", ".tmp", ".temp", ".swp", "~"))
        or ".rmlock." in lowered
    )


def _is_excluded_settings_path(relative_path: Path) -> bool:
    return relative_path == ERROR_REPORTER_SETTINGS_PATH


def _copy_and_hash(source: Path, staged: Path) -> tuple[str, int]:
    digest = hashlib.sha256()
    source_flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    destination_flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    try:
        source_descriptor = os.open(source, source_flags)
        try:
            opened_source_stat = os.fstat(source_descriptor)
            if not stat.S_ISREG(opened_source_stat.st_mode):
                raise StagingPreparationError(f"canonical settings path changed type while seeding: {source}")
            destination_descriptor = os.open(staged, destination_flags, _PRIVATE_FILE_MODE)
            try:
                while chunk := os.read(source_descriptor, 1024 * 1024):
                    digest.update(chunk)
                    _write_all(destination_descriptor, chunk)
            finally:
                os.close(destination_descriptor)
        finally:
            os.close(source_descriptor)
    except OSError as error:
        raise StagingPreparationError(f"cannot seed canonical settings file {source}: {error}") from error
    return digest.hexdigest(), stat.S_IMODE(opened_source_stat.st_mode)


def _write_all(descriptor: int, contents: bytes) -> None:
    view = memoryview(contents)
    while view:
        written = os.write(descriptor, view)
        if written == 0:
            raise OSError("short write while writing settings file")
        view = view[written:]


def _write_manifest(session_root: Path, manifest: tuple[StagedSettingsFile, ...]) -> None:
    records = [
        {
            "canonical_relative_path": str(entry.canonical_relative_path),
            "staged_relative_path": str(entry.staged_relative_path),
            "canonical_sha256": entry.canonical_sha256,
            "canonical_mode": entry.canonical_mode,
        }
        for entry in manifest
    ]
    encoded = (json.dumps({"version": 1, "files": records}, indent=2, sort_keys=True) + "\n").encode("utf-8")
    descriptor = os.open(session_root / MANIFEST_FILENAME, os.O_WRONLY | os.O_CREAT | os.O_EXCL, _PRIVATE_FILE_MODE)
    try:
        _write_all(descriptor, encoded)
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def _finalize_session(
    canonical_root: Path,
    staging_root: Path,
    manifest: tuple[StagedSettingsFile, ...],
) -> QSettingsStagingFinalization:
    manifest_by_path = {
        entry.canonical_relative_path: entry for entry in manifest if not _is_excluded_settings_path(entry.canonical_relative_path)
    }
    try:
        staged_paths = _discover_staged_settings(staging_root)
    except (OSError, StagingFinalizationError) as error:
        return QSettingsStagingFinalization(False, (), str(error))

    relative_paths = sorted(set(manifest_by_path) | staged_paths)
    results = []
    for relative_path in relative_paths:
        entry = manifest_by_path.get(relative_path)
        staged_relative_path = entry.staged_relative_path if entry is not None else relative_path
        baseline_hash = entry.canonical_sha256 if entry is not None else None
        results.append(
            _finalize_file(
                canonical_root,
                staging_root,
                relative_path,
                staged_relative_path,
                baseline_hash,
            )
        )

    successful = all(result.status not in (CopyBackStatus.CONFLICT, CopyBackStatus.FAILED) for result in results)
    finalization_error = None
    if successful:
        try:
            _write_completion_marker(staging_root)
        except OSError as error:
            successful = False
            finalization_error = f"cannot mark staging session complete: {error}"
    return QSettingsStagingFinalization(successful, tuple(results), finalization_error)


def _is_qt_project_lock_artifact(name: str) -> bool:
    lock_name = f"{QT_PROJECT_SETTINGS_PATH.name}.lock"
    return name == lock_name or name.startswith(f"{lock_name}.rmlock")


def _discover_staged_settings(staging_root: Path) -> set[Path]:
    discovered = set()
    for path in staging_root.iterdir():
        if path.name in {MANIFEST_FILENAME, COMPLETED_FILENAME} or _is_qt_project_lock_artifact(path.name):
            continue
        if path.name == "mantidproject":
            continue
        if path.name == QT_PROJECT_SETTINGS_PATH.name:
            path_stat = path.lstat()
            if stat.S_ISLNK(path_stat.st_mode) or not stat.S_ISREG(path_stat.st_mode):
                raise StagingFinalizationError(f"staged settings path is not a regular file: {path}")
            discovered.add(QT_PROJECT_SETTINGS_PATH)
        else:
            raise StagingFinalizationError(f"unexpected staged settings path outside mantidproject: {path}")

    organization_root = staging_root / "mantidproject"
    organization_stat = organization_root.lstat()
    if stat.S_ISLNK(organization_stat.st_mode) or not stat.S_ISDIR(organization_stat.st_mode):
        raise StagingFinalizationError(f"staged settings root is not a real directory: {organization_root}")

    _discover_staged_directory(organization_root, staging_root, discovered)
    return discovered


def _discover_staged_directory(directory: Path, staging_root: Path, discovered: set[Path]) -> None:
    for path in directory.iterdir():
        relative_path = path.relative_to(staging_root)
        if _is_excluded_settings_path(relative_path) or _is_excluded_artifact(path.name):
            continue
        path_stat = path.lstat()
        if stat.S_ISLNK(path_stat.st_mode):
            raise StagingFinalizationError(f"refusing to follow a staged settings symlink: {path}")
        if stat.S_ISDIR(path_stat.st_mode):
            _discover_staged_directory(path, staging_root, discovered)
        elif stat.S_ISREG(path_stat.st_mode):
            discovered.add(relative_path)
        else:
            raise StagingFinalizationError(f"staged settings path is not a regular file: {path}")


def _finalize_file(
    canonical_root: Path,
    staging_root: Path,
    canonical_relative_path: Path,
    staged_relative_path: Path,
    baseline_hash: str | None,
) -> CopyBackFileResult:
    staged_path = staging_root / staged_relative_path
    try:
        staged_hash = _snapshot_file(staged_path)
    except (OSError, StagingFinalizationError) as error:
        return _failed_copy(canonical_relative_path, error)

    if staged_hash is None:
        if baseline_hash is None:
            return CopyBackFileResult(canonical_relative_path, CopyBackStatus.UNCHANGED)
        return _failed_copy(canonical_relative_path, "staged file is missing")
    if staged_hash == baseline_hash:
        return CopyBackFileResult(canonical_relative_path, CopyBackStatus.UNCHANGED)

    return _copy_changed_file(
        canonical_root,
        canonical_root / canonical_relative_path,
        staged_path,
        canonical_relative_path,
        baseline_hash,
    )


def _copy_changed_file(
    canonical_root: Path,
    canonical_path: Path,
    staged_path: Path,
    relative_path: Path,
    baseline_hash: str | None,
) -> CopyBackFileResult:
    no_follow = getattr(os, "O_NOFOLLOW", 0)
    try:
        with _opened_descriptor(staged_path, os.O_RDONLY | no_follow) as staged_descriptor:
            staged_hash = _hash_descriptor(staged_descriptor, staged_path)
            canonical_hash = _snapshot_file(canonical_path)
            if canonical_hash == staged_hash:
                return CopyBackFileResult(relative_path, CopyBackStatus.ALREADY_SYNCHRONIZED)
            if canonical_hash != baseline_hash:
                return CopyBackFileResult(relative_path, CopyBackStatus.CONFLICT)

            _ensure_destination_parent(canonical_path.parent, canonical_root)
            destination_existed = canonical_hash is not None
            flags = os.O_RDWR | no_follow
            if not destination_existed:
                flags |= os.O_CREAT | os.O_EXCL
            with _opened_descriptor(canonical_path, flags, _PRIVATE_FILE_MODE) as canonical_descriptor:
                current_hash = _hash_descriptor(canonical_descriptor, canonical_path) if destination_existed else None
                if current_hash == staged_hash:
                    return CopyBackFileResult(relative_path, CopyBackStatus.ALREADY_SYNCHRONIZED)
                if current_hash != baseline_hash:
                    return CopyBackFileResult(relative_path, CopyBackStatus.CONFLICT)

                os.ftruncate(canonical_descriptor, 0)
                os.lseek(staged_descriptor, 0, os.SEEK_SET)
                while contents := os.read(staged_descriptor, 1024 * 1024):
                    _write_all(canonical_descriptor, contents)
                os.fsync(canonical_descriptor)
        return CopyBackFileResult(relative_path, CopyBackStatus.COPIED)
    except (OSError, StagingFinalizationError) as error:
        return _failed_copy(relative_path, error)


def _snapshot_file(path: Path) -> str | None:
    try:
        with _opened_descriptor(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)) as descriptor:
            return _hash_descriptor(descriptor, path)
    except FileNotFoundError:
        return None


def _hash_descriptor(descriptor: int, path: Path) -> str:
    path_stat = os.fstat(descriptor)
    if not stat.S_ISREG(path_stat.st_mode):
        raise StagingFinalizationError(f"settings path is not a regular file: {path}")
    os.lseek(descriptor, 0, os.SEEK_SET)
    digest = hashlib.sha256()
    while contents := os.read(descriptor, 1024 * 1024):
        digest.update(contents)
    os.lseek(descriptor, 0, os.SEEK_SET)
    return digest.hexdigest()


def _ensure_destination_parent(destination_parent: Path, canonical_root: Path) -> None:
    try:
        relative_parent = destination_parent.relative_to(canonical_root)
    except ValueError as error:
        raise StagingFinalizationError(f"canonical destination escapes its root: {destination_parent}") from error

    try:
        canonical_root.mkdir(mode=_PRIVATE_DIRECTORY_MODE, parents=True)
    except FileExistsError:
        pass
    _validate_real_directory(canonical_root)
    current = canonical_root
    for component in relative_parent.parts:
        current /= component
        try:
            current.mkdir(mode=_PRIVATE_DIRECTORY_MODE)
        except FileExistsError:
            pass
        _validate_real_directory(current)


def _validate_real_directory(path: Path) -> None:
    path_stat = path.lstat()
    if stat.S_ISLNK(path_stat.st_mode) or not stat.S_ISDIR(path_stat.st_mode):
        raise StagingFinalizationError(f"canonical settings parent is not a real directory: {path}")


@contextmanager
def _opened_descriptor(path: Path, flags: int, mode: int | None = None) -> Iterator[int]:
    descriptor = os.open(path, flags) if mode is None else os.open(path, flags, mode)
    try:
        yield descriptor
    finally:
        os.close(descriptor)


def _failed_copy(relative_path: Path, error: object) -> CopyBackFileResult:
    return CopyBackFileResult(relative_path, CopyBackStatus.FAILED, str(error))


def _write_completion_marker(staging_root: Path) -> None:
    marker = staging_root / COMPLETED_FILENAME
    with _opened_descriptor(marker, os.O_WRONLY | os.O_CREAT | os.O_EXCL, _PRIVATE_FILE_MODE) as descriptor:
        os.fsync(descriptor)
