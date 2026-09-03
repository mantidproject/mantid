# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""Linux storage discovery for staging user-scope QSettings files."""

from collections.abc import Callable, Iterable, Mapping
from dataclasses import dataclass
from enum import Enum
import os
from pathlib import Path
import re
import sys


MOUNTINFO_PATH = Path("/proc/self/mountinfo")
NFS_FILESYSTEM_TYPES = frozenset(("nfs", "nfs4"))
QSETTINGS_STAGING_ENV = "MANTID_QSETTINGS_STAGING"

_MOUNTINFO_ESCAPE_PATTERN = re.compile(r"\\(011|012|040|134)")
_MOUNTINFO_ESCAPES = {
    "011": "\t",
    "012": "\n",
    "040": " ",
    "134": "\\",
}


class MountMatchStatus(Enum):
    """Outcome of matching a path to a mount table."""

    FOUND = "found"
    NOT_FOUND = "not_found"
    AMBIGUOUS = "ambiguous"
    UNAVAILABLE = "unavailable"


class QSettingsStagingReason(Enum):
    """Stable reason codes returned by the staging eligibility decision."""

    ELIGIBLE = "eligible"
    DISABLED = "disabled"
    UNSUPPORTED_PLATFORM = "unsupported_platform"
    CONFIG_MOUNT_UNAVAILABLE = "config_mount_unavailable"
    CONFIG_MOUNT_NOT_FOUND = "config_mount_not_found"
    CONFIG_MOUNT_AMBIGUOUS = "config_mount_ambiguous"
    CONFIG_NOT_NFS = "config_not_nfs"
    ROOTS_NOT_DISTINCT = "roots_not_distinct"
    CACHE_MOUNT_UNAVAILABLE = "cache_mount_unavailable"
    CACHE_MOUNT_NOT_FOUND = "cache_mount_not_found"
    CACHE_MOUNT_AMBIGUOUS = "cache_mount_ambiguous"
    CACHE_IS_NFS = "cache_is_nfs"
    MOUNTS_NOT_DISTINCT = "mounts_not_distinct"
    CACHE_NOT_DIRECTORY = "cache_not_directory"
    CACHE_NOT_OWNED = "cache_not_owned"
    CACHE_NOT_WRITABLE = "cache_not_writable"


@dataclass(frozen=True)
class XdgPaths:
    """Canonical user roots used by QSettings staging."""

    config_root: Path
    cache_root: Path


@dataclass(frozen=True)
class MountInfo:
    """The mount information needed to classify a storage path."""

    mount_id: int
    mount_point: Path
    filesystem_type: str

    @property
    def is_nfs(self) -> bool:
        return self.filesystem_type.lower() in NFS_FILESYSTEM_TYPES


@dataclass(frozen=True)
class MountMatch:
    """A conservative mount-table lookup result."""

    status: MountMatchStatus
    mount: MountInfo | None = None


@dataclass(frozen=True)
class QSettingsStorageDiscovery:
    """XDG roots and their corresponding Linux mounts."""

    paths: XdgPaths
    config_mount: MountMatch
    cache_mount: MountMatch
    mountinfo_error: str | None = None


@dataclass(frozen=True)
class QSettingsStagingEligibility:
    """Whether staging may proceed, without performing any storage mutation."""

    active: bool
    reason: QSettingsStagingReason
    config_root: Path | None = None
    cache_root: Path | None = None
    config_filesystem: str | None = None
    cache_filesystem: str | None = None


def resolve_xdg_config_root(environ: Mapping[str, str] | None = None, home: Path | None = None) -> Path:
    """Resolve the XDG config root without consulting the cache setting."""
    environ = os.environ if environ is None else environ
    home = _resolve_home(home)
    return _resolve_xdg_root(environ.get("XDG_CONFIG_HOME"), home / ".config")


def resolve_xdg_cache_root(environ: Mapping[str, str] | None = None, home: Path | None = None) -> Path:
    """Resolve the XDG cache root independently of the config setting."""
    environ = os.environ if environ is None else environ
    home = _resolve_home(home)
    return _resolve_xdg_root(environ.get("XDG_CACHE_HOME"), home / ".cache")


def resolve_xdg_paths(environ: Mapping[str, str] | None = None, home: Path | None = None) -> XdgPaths:
    """Resolve XDG config/cache roots without creating either directory."""
    return XdgPaths(
        config_root=resolve_xdg_config_root(environ=environ, home=home),
        cache_root=resolve_xdg_cache_root(environ=environ, home=home),
    )


def parse_mountinfo(lines: Iterable[str]) -> tuple[MountInfo, ...]:
    """Parse usable records from Linux mountinfo lines.

    Malformed records are ignored. An empty result is handled conservatively by
    :func:`find_mount` and never implies a local filesystem.
    """
    mounts = []
    for line in lines:
        sections = line.rstrip("\n").split(" - ", maxsplit=1)
        if len(sections) != 2:
            continue
        mount_fields = sections[0].split()
        filesystem_fields = sections[1].split()
        if len(mount_fields) < 6 or not filesystem_fields:
            continue
        try:
            mount_id = int(mount_fields[0])
        except ValueError:
            continue
        mount_point = Path(_decode_mountinfo_path(mount_fields[4]))
        if not mount_point.is_absolute():
            continue
        mounts.append(
            MountInfo(
                mount_id=mount_id,
                mount_point=mount_point,
                filesystem_type=filesystem_fields[0],
            )
        )
    return tuple(mounts)


def find_mount(path: Path, mounts: Iterable[MountInfo]) -> MountMatch:
    """Return the longest component-wise mount match for *path*.

    Duplicate records at the most-specific mount point are treated as
    ambiguous. This is conservative for stacked mounts, where choosing an
    arbitrary record could incorrectly classify NFS as local storage.
    """
    path = Path(path).expanduser().resolve(strict=False)
    candidates = tuple(mount for mount in mounts if path == mount.mount_point or mount.mount_point in path.parents)
    if not candidates:
        return MountMatch(MountMatchStatus.NOT_FOUND)

    longest = max(len(mount.mount_point.parts) for mount in candidates)
    best_matches = tuple(mount for mount in candidates if len(mount.mount_point.parts) == longest)
    if len(best_matches) != 1:
        return MountMatch(MountMatchStatus.AMBIGUOUS)
    return MountMatch(MountMatchStatus.FOUND, best_matches[0])


def discover_qsettings_storage(
    environ: Mapping[str, str] | None = None,
    home: Path | None = None,
    mountinfo_path: Path = MOUNTINFO_PATH,
) -> QSettingsStorageDiscovery:
    """Resolve XDG roots and classify their mounts without changing storage."""
    paths = resolve_xdg_paths(environ=environ, home=home)
    mounts = _read_mountinfo(mountinfo_path)
    if mounts is None:
        unavailable = MountMatch(MountMatchStatus.UNAVAILABLE)
        return QSettingsStorageDiscovery(
            paths=paths,
            config_mount=unavailable,
            cache_mount=unavailable,
            mountinfo_error="unreadable",
        )

    return QSettingsStorageDiscovery(
        paths=paths,
        config_mount=find_mount(paths.config_root, mounts),
        cache_mount=find_mount(paths.cache_root, mounts),
    )


def evaluate_qsettings_staging(
    environ: Mapping[str, str] | None = None,
    home: Path | None = None,
    mountinfo_path: Path = MOUNTINFO_PATH,
    platform_name: str | None = None,
    effective_uid: int | None = None,
    access: Callable[[Path, int], bool] | None = None,
) -> QSettingsStagingEligibility:
    """Decide whether local staging is eligible without changing any path.

    Config storage is resolved and classified first. A confirmed non-NFS config
    returns immediately without resolving or inspecting the cache root.
    """
    environ = os.environ if environ is None else environ
    if environ.get(QSETTINGS_STAGING_ENV) != "1":
        return QSettingsStagingEligibility(False, QSettingsStagingReason.DISABLED)

    platform_name = sys.platform if platform_name is None else platform_name
    if not platform_name.startswith("linux"):
        return QSettingsStagingEligibility(False, QSettingsStagingReason.UNSUPPORTED_PLATFORM)

    config_root = resolve_xdg_config_root(environ=environ, home=home)
    mounts = _read_mountinfo(mountinfo_path)
    if mounts is None:
        return QSettingsStagingEligibility(
            False,
            QSettingsStagingReason.CONFIG_MOUNT_UNAVAILABLE,
            config_root=config_root,
        )

    config_match = find_mount(config_root, mounts)
    if config_match.status is not MountMatchStatus.FOUND:
        return QSettingsStagingEligibility(
            False,
            _mount_failure_reason(config_match.status, config=True),
            config_root=config_root,
        )

    config_mount = config_match.mount
    if not config_mount.is_nfs:
        return QSettingsStagingEligibility(
            False,
            QSettingsStagingReason.CONFIG_NOT_NFS,
            config_root=config_root,
            config_filesystem=config_mount.filesystem_type,
        )

    cache_root = resolve_xdg_cache_root(environ=environ, home=home)
    if cache_root == config_root:
        return _eligibility_with_mounts(
            False,
            QSettingsStagingReason.ROOTS_NOT_DISTINCT,
            config_root,
            cache_root,
            config_mount,
        )

    cache_match = find_mount(cache_root, mounts)
    if cache_match.status is not MountMatchStatus.FOUND:
        return _eligibility_with_mounts(
            False,
            _mount_failure_reason(cache_match.status, config=False),
            config_root,
            cache_root,
            config_mount,
        )

    cache_mount = cache_match.mount
    if cache_mount.is_nfs:
        return _eligibility_with_mounts(
            False,
            QSettingsStagingReason.CACHE_IS_NFS,
            config_root,
            cache_root,
            config_mount,
            cache_mount,
        )
    if cache_mount.mount_id == config_mount.mount_id:
        return _eligibility_with_mounts(
            False,
            QSettingsStagingReason.MOUNTS_NOT_DISTINCT,
            config_root,
            cache_root,
            config_mount,
            cache_mount,
        )

    cache_problem = _assess_cache_root(cache_root, effective_uid=effective_uid, access=access)
    if cache_problem is not None:
        return _eligibility_with_mounts(
            False,
            cache_problem,
            config_root,
            cache_root,
            config_mount,
            cache_mount,
        )
    return _eligibility_with_mounts(
        True,
        QSettingsStagingReason.ELIGIBLE,
        config_root,
        cache_root,
        config_mount,
        cache_mount,
    )


def _resolve_home(home: Path | None) -> Path:
    return (Path.home() if home is None else Path(home)).expanduser().resolve(strict=False)


def _resolve_xdg_root(value: str | None, fallback: Path) -> Path:
    candidate = Path(value) if value else fallback
    if not candidate.is_absolute():
        candidate = fallback
    return candidate.resolve(strict=False)


def _decode_mountinfo_path(value: str) -> str:
    return _MOUNTINFO_ESCAPE_PATTERN.sub(lambda match: _MOUNTINFO_ESCAPES[match.group(1)], value)


def _read_mountinfo(mountinfo_path: Path) -> tuple[MountInfo, ...] | None:
    try:
        with Path(mountinfo_path).open(encoding="utf-8") as handle:
            return parse_mountinfo(handle)
    except OSError:
        return None


def _mount_failure_reason(status: MountMatchStatus, config: bool) -> QSettingsStagingReason:
    config_reasons = {
        MountMatchStatus.NOT_FOUND: QSettingsStagingReason.CONFIG_MOUNT_NOT_FOUND,
        MountMatchStatus.AMBIGUOUS: QSettingsStagingReason.CONFIG_MOUNT_AMBIGUOUS,
        MountMatchStatus.UNAVAILABLE: QSettingsStagingReason.CONFIG_MOUNT_UNAVAILABLE,
    }
    cache_reasons = {
        MountMatchStatus.NOT_FOUND: QSettingsStagingReason.CACHE_MOUNT_NOT_FOUND,
        MountMatchStatus.AMBIGUOUS: QSettingsStagingReason.CACHE_MOUNT_AMBIGUOUS,
        MountMatchStatus.UNAVAILABLE: QSettingsStagingReason.CACHE_MOUNT_UNAVAILABLE,
    }
    return (config_reasons if config else cache_reasons)[status]


def _assess_cache_root(
    cache_root: Path,
    effective_uid: int | None,
    access: Callable[[Path, int], bool] | None,
) -> QSettingsStagingReason | None:
    access = os.access if access is None else access
    try:
        if cache_root.exists():
            if not cache_root.is_dir():
                return QSettingsStagingReason.CACHE_NOT_DIRECTORY
            effective_uid = os.geteuid() if effective_uid is None else effective_uid
            if cache_root.stat().st_uid != effective_uid:
                return QSettingsStagingReason.CACHE_NOT_OWNED
            if not access(cache_root, os.W_OK | os.X_OK):
                return QSettingsStagingReason.CACHE_NOT_WRITABLE
            return None

        existing_parent = cache_root.parent
        while not existing_parent.exists() and existing_parent != existing_parent.parent:
            existing_parent = existing_parent.parent
        if not existing_parent.is_dir() or not access(existing_parent, os.W_OK | os.X_OK):
            return QSettingsStagingReason.CACHE_NOT_WRITABLE
    except OSError:
        return QSettingsStagingReason.CACHE_NOT_WRITABLE
    return None


def _eligibility_with_mounts(
    active: bool,
    reason: QSettingsStagingReason,
    config_root: Path,
    cache_root: Path,
    config_mount: MountInfo,
    cache_mount: MountInfo | None = None,
) -> QSettingsStagingEligibility:
    return QSettingsStagingEligibility(
        active=active,
        reason=reason,
        config_root=config_root,
        cache_root=cache_root,
        config_filesystem=config_mount.filesystem_type,
        cache_filesystem=cache_mount.filesystem_type if cache_mount is not None else None,
    )
