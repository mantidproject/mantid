# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui
from qtpy import QtCore

# If the GUI has not been created yet, make a new one.
if 'engineering_gui' not in globals():
    engineering_gui = EngineeringDiffractionGui()

# Restore minimised and hidden windows without recreating the GUI.
if engineering_gui.isHidden():  # noqa
    engineering_gui.show()
else:
    engineering_gui.setWindowState(engineering_gui.windowState() & ~QtCore.Qt.WindowMinimized
                                   | QtCore.Qt.WindowActive)
    engineering_gui.activateWindow()
