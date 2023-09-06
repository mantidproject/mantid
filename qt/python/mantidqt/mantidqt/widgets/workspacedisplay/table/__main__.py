# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.

# To Run - target this package with PyCharm, and __main__ will be executed

import matplotlib

matplotlib.use("Qt5Agg")

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import Load
from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay
import matplotlib.pyplot as plt

app = QApplication([])
ws = Load("SavedTableWorkspace.nxs")
window = TableWorkspaceDisplay(ws, plt)
window.show_view()
app.exec_()
