# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Restart this Python process with tbbmalloc preloaded, as the Workbench launcher does.

Imported by mantid-tbbmalloc.pth, so the site module runs it at startup before any user code.
LD_PRELOAD only takes effect when a process starts, so the interpreter restarts itself with
the same arguments. Set MANTID_DISABLE_TBBMALLOC=1 to turn this off.
"""

import os
import sys

_GUARD = "_MANTID_TBBMALLOC_REEXEC"


def _is_python_binary(path):
    """Return True if ``path`` names a Python interpreter binary, such as ``python3.13``."""
    name = os.path.basename(path)
    version = name.removeprefix("python")
    return version != name and (not version or version.replace(".", "").isdigit())


def _reexec_with_tbbmalloc():
    """Restart the interpreter with libtbbmalloc_proxy in LD_PRELOAD, unless that is unwanted or unsafe.

    Returns without doing anything when tbbmalloc is disabled, already loaded, or unavailable, when a
    sanitizer runtime is preloaded, or when Python is embedded in another program. If the restart
    fails, the environment is put back as it was and Python carries on with the system allocator.
    """
    if not sys.platform.startswith("linux"):
        return
    if os.environ.pop(_GUARD, None):
        return  # this is the restarted process; never loop, and keep the marker out of children
    if os.environ.get("MANTID_DISABLE_TBBMALLOC"):
        return
    proxy = os.path.join(sys.prefix, "lib", "libtbbmalloc_proxy.so.2")
    if not os.path.exists(proxy):
        return
    try:
        with open("/proc/self/maps") as maps:
            if "libtbbmalloc_proxy" in maps.read():
                return  # already preloaded, e.g. by the Workbench launcher
        # only restart a standalone interpreter, never a program that embeds Python. An embedding program
        # can point sys.executable at itself, so the running binary must also be a python binary.
        exe = os.path.realpath("/proc/self/exe")
        if os.path.realpath(sys.executable) != exe or not _is_python_binary(exe):
            return
    except OSError:
        return
    if not sys.orig_argv:
        return
    preload = os.environ.get("LD_PRELOAD")
    if preload and ("asan" in preload or "sanitizer" in preload):
        return  # a sanitizer build preloads its own runtime instead
    os.environ["LD_PRELOAD"] = f"{proxy}:{preload}" if preload else proxy
    os.environ[_GUARD] = "1"
    for stream in (sys.stdout, sys.stderr):
        if stream is not None:
            stream.flush()
    try:
        os.execv(sys.executable, sys.orig_argv)  # noqa: S606 same interpreter, same flags and arguments
    except OSError:
        # carry on without tbbmalloc, and do not hand the changed environment to child processes
        del os.environ[_GUARD]
        if preload is None:
            del os.environ["LD_PRELOAD"]
        else:
            os.environ["LD_PRELOAD"] = preload


_reexec_with_tbbmalloc()
