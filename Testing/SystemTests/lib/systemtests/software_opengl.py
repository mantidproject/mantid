# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Software OpenGL fallback for system tests on Windows.

VTK, which draws the new instrument view, needs OpenGL 3.2. Machines without a GPU, such as the VMs that host the
Windows CI runners, do not provide it. Qt ships a Mesa llvmpipe software renderer (opengl32sw.dll) and falls back
to it on its own, but VTK always loads "opengl32.dll". Windows reuses an already loaded module of that name, so
loading a copy of Mesa named opengl32.dll before anything else loads OpenGL makes VTK (and Qt) render with it.

This must run before the native opengl32.dll is loaded into the process, because it cannot be replaced afterwards.
For the same reason the native OpenGL is probed in a separate process.
"""

import ctypes
import os
import shutil
import subprocess
import sys
import tempfile

# Forces the choice: "1" uses software OpenGL, "0" uses the native OpenGL. When unset, the native OpenGL is probed.
SOFTWARE_OPENGL_ENV_VAR = "MANTID_SYSTEMTEST_SOFTWARE_OPENGL"
MESA_DLL_NAME = "opengl32sw.dll"

_PROBE_SCRIPT = (
    "import vtkmodules.vtkRenderingOpenGL2\n"
    "from vtkmodules.vtkRenderingCore import vtkRenderWindow\n"
    "print('supported' if vtkRenderWindow().SupportsOpenGL() else 'unsupported')\n"
)


def native_opengl_supports_vtk(python_executable: str = sys.executable) -> bool:
    try:
        result = subprocess.run([python_executable, "-c", _PROBE_SCRIPT], capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.SubprocessError):
        return False
    # A failed probe usually means the native OpenGL crashed VTK, so it counts as unsupported
    return result.returncode == 0 and result.stdout.split()[-1:] == ["supported"]


def load_software_opengl(mesa_dll: str, cache_dir: str) -> bool:
    """Load mesa_dll into this process under the name opengl32.dll. Returns True if it was loaded."""
    if not os.path.isfile(mesa_dll):
        print(f"{SOFTWARE_OPENGL_ENV_VAR}: cannot use software OpenGL, {mesa_dll} was not found")
        return False
    os.makedirs(cache_dir, exist_ok=True)
    renamed_dll = os.path.join(cache_dir, "opengl32.dll")
    if not os.path.isfile(renamed_dll) or os.path.getsize(renamed_dll) != os.path.getsize(mesa_dll):
        # Copy then rename, so that parallel test processes never load a partially written file
        partial_dll = f"{renamed_dll}.{os.getpid()}"
        shutil.copyfile(mesa_dll, partial_dll)
        try:
            os.replace(partial_dll, renamed_dll)
        except OSError:
            # Another test process has the same copy loaded already, so it cannot be overwritten
            os.remove(partial_dll)
    ctypes.WinDLL(renamed_dll)
    print(f"{SOFTWARE_OPENGL_ENV_VAR}: using the Mesa software OpenGL renderer")
    return True


def use_software_opengl_if_needed() -> bool:
    """On Windows, load software OpenGL if the native OpenGL cannot run VTK. Returns True if it was loaded."""
    if not sys.platform.startswith("win"):
        return False
    choice = os.environ.get(SOFTWARE_OPENGL_ENV_VAR)
    if choice is None:
        choice = "0" if native_opengl_supports_vtk() else "1"
        # Test processes started from this one inherit the decision rather than probing again
        os.environ[SOFTWARE_OPENGL_ENV_VAR] = choice
    if choice != "1":
        return False
    mesa_dll = os.path.join(sys.prefix, "Library", "bin", MESA_DLL_NAME)
    cache_dir = os.path.join(tempfile.gettempdir(), "mantid_systemtest_software_opengl")
    return load_software_opengl(mesa_dll, cache_dir)
