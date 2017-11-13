"""Defines methods to check requirements for the application
"""
from __future__ import absolute_import

from distutils.version import LooseVersion


def show_warning(message):
    """Show warning using Tkinter if available"""
    try:
        # If Tkinter is installed (highly probable), showing an error pop-up
        import Tkinter
        import tkMessageBox
        root = Tkinter.Tk()
        root.withdraw()
        tkMessageBox.showerror("Mantid Workbench", message)
    except ImportError:
        pass
    raise RuntimeError(message)


def check_qt():
    """Check the qt requirements are as expected
    """
    package_name, required_ver = ("PyQt5", "5.2")
    try:
        import qtpy
        actual_ver = qtpy.PYQT_VERSION
        if LooseVersion(actual_ver) < LooseVersion(required_ver):
            show_warning(
              "Please check installation requirements:\n"
              "{} {}+ is required (found v{}).".format(
                                                  package_name,
                                                  required_ver,
                                                  actual_ver)
            )
    except ImportError as exc:
        show_warning("Failed to import qtpy: '{}'\n".format(str(exc)) +
                     "Please check the installation.")
