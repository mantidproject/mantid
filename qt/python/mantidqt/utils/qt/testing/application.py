# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import

import atexit
import sys
import traceback

from qtpy import PYQT4
from qtpy.QtWidgets import QApplication
import sip

from mantidqt.utils.qt.plugins import setup_library_paths

# Hold on to QAPP reference to avoid garbage collection
_QAPP = QApplication.instance()


@atexit.register
def cleanup_qapp_ref():
    """
    Remove the global reference to the QApplication object here
    """
    global _QAPP
    if _QAPP is not None:
        sip.delete(_QAPP)
    del _QAPP


def get_application(name=''):
    """
    Initialise and return the global application object
    :param name: Optional application name
    :return: Global appliction object
    """
    global _QAPP

    def exception_handler(exctype, value, tb):
        traceback.print_exception(exctype, value, tb)
        sys.exit(1)

    if _QAPP is None:
        setup_library_paths()
        _QAPP = QApplication([name])
        if PYQT4:
            # Do not destroy C++ Qtcore objects on exit
            # Matches PyQt5 behaviour to avoid random segfaults
            # https://www.riverbankcomputing.com/static/Docs/PyQt5/pyqt4_differences.html#object-destruction-on-exit
            sip.setdestroyonexit(False)
        sys.excepthook = exception_handler

    return _QAPP
