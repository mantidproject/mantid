# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# To Run - target this package with PyCharm, and __main__ will be executed

import os
import matplotlib
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import Load
from mantidqt.widgets.matrixworkspacedisplay.presenter import MatrixWorkspaceDisplay

app = QApplication([])
LOQ74044 = Load(os.path.join(p, r"LOQ74044.nxs"))
window = MatrixWorkspaceDisplay(LOQ74044, plt)
app.exec_()
