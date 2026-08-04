# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""Linux storage discovery for staging user-scope QSettings files."""

from collections.abc import Iterable, Mapping
from dataclasses import dataclass
from enum import Enum
import os
from pathlib import Path
import re


MOUNTINFO_PATH = Path("/proc/self/mountinfo")
NFS_FILESYSTEM_TYPES = frozenset(("nfs", "nfs4"))

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


def resolve_xdg_paths(environ: Mapping[str, str] | None = None, home: Path | None = None) -> XdgPaths:
    """Resolve XDG config/cache roots without creating either directory."""
    environ = os.environ if environ is None else environ
    home = (Path.home() if home is None else Path(home)).expanduser().resolve(strict=False)
    return XdgPaths(
        config_root=_resolve_xdg_root(environ.get("XDG_CONFIG_HOME"), home / ".config"),
        cache_root=_resolve_xdg_root(environ.get("XDG_CACHE_HOME"), home / ".cache"),
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
    try:
        with Path(mountinfo_path).open(encoding="utf-8") as handle:
            mounts = parse_mountinfo(handle)
    except OSError:
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


def _resolve_xdg_root(value: str | None, fallback: Path) -> Path:
    candidate = Path(value) if value else fallback
    if not candidate.is_absolute():
        candidate = fallback
    return candidate.resolve(strict=False)


def _decode_mountinfo_path(value: str) -> str:
    return _MOUNTINFO_ESCAPE_PATTERN.sub(lambda match: _MOUNTINFO_ESCAPES[match.group(1)], value)
