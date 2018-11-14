# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# To Run target this package with PyCharm, and __main__ will be executed

from mantidqt.widgets.samplelogs.presenter import SampleLogs
from mantid.simpleapi import Load
from qtpy.QtWidgets import QApplication

ws = Load('HRP39187.RAW')

app = QApplication([])
window = SampleLogs(ws)
app.exec_()
