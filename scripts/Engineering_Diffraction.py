# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui
from qtpy import QtCore
import sys


def _on_delete():
    del globals()['engineering_gui']


if 'engineering_gui' in globals() and not globals()['engineering_gui'].isHidden():
    engineering_gui = globals()['engineering_gui']
    engineering_gui.setWindowState(engineering_gui.windowState() & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
    engineering_gui.activateWindow()
else:
    if 'workbench' in sys.modules:
        from workbench.config import get_window_config

        parent, flags = get_window_config()
    else:
        parent, flags = None, None
    engineering_gui = EngineeringDiffractionGui(parent=parent, window_flags=flags)
    engineering_gui.destroyed.connect(_on_delete)
    engineering_gui.show()
