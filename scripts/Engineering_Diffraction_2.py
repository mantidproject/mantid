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


# Check if the interface is loaded and if so show it while maintaining its state.
if 'engineering_diffraction' in globals():
    engineering_diffraction = globals()['engineering_diffraction']
    # Restore a minimised window.
    if not engineering_diffraction.isHidden():
        engineering_diffraction.setWindowState(engineering_diffraction.windowState() & ~QtCore.Qt.WindowMinimized
                                               | QtCore.Qt.WindowActive)
        engineering_diffraction.activateWindow()
    else:
        engineering_diffraction = EngineeringDiffractionGui()
        engineering_diffraction.show()
else:  # Reload GUI if not currently loaded.
    engineering_diffraction = EngineeringDiffractionGui()
    engineering_diffraction.show()
