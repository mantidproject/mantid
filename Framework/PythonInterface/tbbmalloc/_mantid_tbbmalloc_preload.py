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


def _reexec_with_tbbmalloc():
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
        # only restart a standalone interpreter, never a program that embeds Python
        if os.path.realpath(sys.executable) != os.path.realpath("/proc/self/exe"):
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
    os.execv(sys.executable, sys.orig_argv)  # noqa: S606 same interpreter, same flags and arguments


_reexec_with_tbbmalloc()
