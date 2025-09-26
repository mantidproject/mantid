# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import os
from pathlib import Path
import shutil
import subprocess
import sys

from workbench.app.main import main


def _patch_qtwebengine_files(prefix: str):
    """
    Copy Qt WebEngine executable, resources, locales, and ICU data into the environment
    root so Qt can find them when launching from a conda environment on Windows.
    """
    env_root = Path(prefix)  # Python executable directory
    library = env_root / "Library"

    # Copy QtWebEngineProcess.exe
    src_exe = library / "bin" / "QtWebEngineProcess.exe"
    dst_exe = env_root / "QtWebEngineProcess.exe"
    if src_exe.exists() and not dst_exe.exists():
        shutil.copy2(src_exe, dst_exe)

    # Copy resources directory
    src_resources = library / "resources"
    for f in src_resources.glob("*"):
        dst = env_root / f.name
        if not dst.exists():
            shutil.copy2(f, dst)

    # Copy locales directory
    src_locales = library / "translations" / "qtwebengine_locales"
    dst_locales = env_root / "qtwebengine_locales"
    if src_locales.exists() and not dst_locales.exists():
        shutil.copytree(src_locales, dst_locales)


def launch(args=None):
    if args is None:
        args = sys.argv
    if sys.platform.startswith("linux"):
        command = ["launch_mantidworkbench"] + args[1:]
        subprocess.run(command)
    else:
        if sys.platform.startswith("win"):
            # Windows conda installs require us to set the QT_PLUGIN_PATH variable and copy QtWebEngine files
            # to the root directory of the conda environment.
            if "CONDA_PREFIX" in os.environ:
                prefix = os.environ.get("CONDA_PREFIX")
                os.environ["QT_PLUGIN_PATH"] = f"{prefix}\\Library\\plugins"
                _patch_qtwebengine_files(prefix)
        main(args[1:])


if __name__ == "__main__":
    sys.exit(launch(sys.argv))
