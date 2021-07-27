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


# Single instance. If minimized the menu should show it and not create a new one
ENGG_UI_INSTANCE = None


def _on_delete():
    global ENGG_UI_INSTANCE
    ENGG_UI_INSTANCE = None


if ENGG_UI_INSTANCE is not None and ENGG_UI_INSTANCE.isHidden():
    ENGG_UI_INSTANCE.setWindowState(ENGG_UI_INSTANCE.windowState() & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
    ENGG_UI_INSTANCE.activateWindow()
else:
    if 'workbench' in sys.modules:
        from workbench.config import get_window_config

        parent, flags = get_window_config()
    else:
        parent, flags = None, None
    ENGG_UI_INSTANCE = EngineeringDiffractionGui(parent=parent, window_flags=flags)
    ENGG_UI_INSTANCE.destroyed.connect(_on_delete)
    ENGG_UI_INSTANCE.show()
