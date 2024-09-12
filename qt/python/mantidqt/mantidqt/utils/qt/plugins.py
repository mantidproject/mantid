#  This file is part of the mantid workbench.
#
#
"""Defines functions to setup paths to Qt plugins at runtime. This is
a generated file. Any changes made here will be lost. See
qt/python/mantidqt/utils/qt/plugins.py.in
"""

import os

# Qt needs to be able to find its plugins and by default it looks in the
# directories where they were built. On Linux the libraries are in fixed
# locations.
#
#  On Windows the package can be installed anywhere so the locations must
#  be determined at runtime. The two options are:
#    1. using a qt.conf file: requires it to be next to the application
#       executable but this would be python.exe and then would not allow
#       switching between Qt Major versions
#    2. add extra paths to the LibraryPath list here
# As we can dynamically determine the Qt version here we choose option 2.


def setup_library_paths_win():
    """Adds the build-time configured directory to the Qt library path.
    The buildsystem generates the path at build time.
    A %V marker can be used that will be replaced by the major version of Qt
    at runtime.
    """
    import os.path as osp
    import sys
    from qtpy import QT_VERSION
    from qtpy.QtCore import QCoreApplication

    conda_prefix = os.environ.get("CONDA_PREFIX", "")
    if conda_prefix != "":
        conda_qt_plugins = osp.join(conda_prefix, "Library", "plugins")
        QCoreApplication.addLibraryPath(conda_qt_plugins)
        return

    # package build
    pkg_qt_plugins = osp.dirname(sys.executable) + f"\\..\\plugins\\qt{QT_VERSION[0]}"
    if osp.isdir(pkg_qt_plugins):
        QCoreApplication.addLibraryPath(pkg_qt_plugins)
        return

    # development build
    dev_qt_plugins = osp.dirname(sys.executable) + f"\\..\\qt{QT_VERSION[0]}\\plugins"
    if osp.isdir(dev_qt_plugins):
        QCoreApplication.addLibraryPath(dev_qt_plugins)
        return


# Set the implementation appropriate for the platform
if os.name == "nt":
    setup_library_paths = setup_library_paths_win
else:

    def setup_library_paths():
        None
