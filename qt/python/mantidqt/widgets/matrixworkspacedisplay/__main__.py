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
import matplotlib.pyplot as plt # noqa: F402

from qtpy.QtWidgets import QApplication # noqa: F402

from mantid.simpleapi import Load # noqa: F402
from mantidqt.widgets.matrixworkspacedisplay.presenter import MatrixWorkspaceDisplay # noqa: F402

app = QApplication([])
LOQ74044 = Load("LOQ74044.nxs")
window = MatrixWorkspaceDisplay(LOQ74044, plt)
app.exec_()
