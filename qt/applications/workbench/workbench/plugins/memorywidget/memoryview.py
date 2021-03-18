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

def from_normal_to_critical(critical, currentValue, newValue)->bool:
    return (currentValue < critical and newValue >= critical)

def from_critical_to_normal(critical, currentValue, newValue)->bool:
    return (currentValue >= critical and newValue < critical)

class MemoryView(QWidget):


    def __init__(self, parent):
        super(MemoryView, self).__init__(parent)

        self.critical = 90
        self.memory_bar = QProgressBar(self)

    def set_bar_color(self, currentValue, newValue):
        if (from_normal_to_critical(self.critical, currentValue, newValue)):
            self.memory_bar.setStyleSheet(CRITICAL_STYLE)
        elif (from_critical_to_normal(self.critical, currentValue, newValue)):
            self.memory_bar.setStyleSheet(NORMAL_STYLE)
        else:
            pass

    def set_value(self, newValue, mem_used, mem_avail):
        # newValue is the mem_used_percent(int)
        currentValue = self.memory_bar.value()
        if currentValue != newValue:
            self.set_bar_color(currentValue, newValue)
            self.memory_bar.setValue(newValue)
            display_str = "%3.1f"%mem_used + "/" + "%3.1f"%mem_avail + " GB " + \
                        "(" + "%d"%newValue+"%" +")"
            self.memory_bar.setFormat(display_str)
