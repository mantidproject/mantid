# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtWidgets import QWidget, QProgressBar
from qtpy.QtCore import Signal
from mantidqt.utils.qt import load_ui

NORMAL_STYLE = """
QProgressBar::chunk {
    background-color: lightgreen;
}
"""

CRITICAL_STYLE = """
QProgressBar::chunk {
    background-color: red;
}
"""

def fromNormalToCritical(critical, currentValue, newValue)->bool:
    return (currentValue < critical and newValue >= critical)

def fromCriticalToNormal(critical, currentValue, newValue)->bool:
    return (currentValue >= critical and newValue < critical)

class MemoryView(QWidget):

    updateSignal = Signal()

    def __init__(self, parent):
        super(MemoryView, self).__init__(parent)

        self.critical = 90
        # For the future use, if needed
        #self.ui = load_ui(__file__, 'memorybar.ui', baseinstance=self)
        self.memory_bar = QProgressBar(self)

    def setBarColor(self, currentValue, newValue):
        if (fromNormalToCritical(self.critical, currentValue, newValue)):
            self.memory_bar.setStyleSheet(CRITICAL_STYLE)
        elif (fromCriticalToNormal(self.critical, currentValue, newValue)):
            self.memory_bar.setStyleSheet(NORMAL_STYLE)
        else:
            pass

    def setValue(self, newValue):
        currentValue = self.memory_bar.value()
        if currentValue != newValue:
            self.setBarColor(currentValue, newValue)
            self.memory_bar.setValue(newValue)

    def onUpdateRequest(self):
        self.updateSignal.emit()
