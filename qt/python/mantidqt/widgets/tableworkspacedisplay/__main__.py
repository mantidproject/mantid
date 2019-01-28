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

import matplotlib

matplotlib.use('Qt5Agg')


from qtpy.QtWidgets import QApplication  # noqa: F402

from mantid.simpleapi import Load  # noqa: F402
from mantidqt.widgets.tableworkspacedisplay.presenter import TableWorkspaceDisplay  # noqa: F402
import matplotlib.pyplot as plt  # noqa: F402

app = QApplication([])
ws = Load("SavedTableWorkspace.nxs")
window = TableWorkspaceDisplay(ws, plt)
app.exec_()
