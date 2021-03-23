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


def from_normal_to_critical(critical, current_value, new_value) -> bool:
    return current_value < critical <= new_value


def from_critical_to_normal(critical, current_value, new_value) -> bool:
    return current_value >= critical > new_value


class MemoryView(QWidget):
    def __init__(self, parent):
        super(MemoryView, self).__init__(parent)

        self.critical = 90
        self.memory_bar = QProgressBar(self)

    def set_bar_color(self, current_value, new_value):
        if from_normal_to_critical(self.critical, current_value, new_value):
            self.memory_bar.setStyleSheet(CRITICAL_STYLE)
        elif from_critical_to_normal(self.critical, current_value, new_value):
            self.memory_bar.setStyleSheet(NORMAL_STYLE)
        else:
            pass

    def set_value(self, new_value, mem_used, mem_avail):
        # newValue is the mem_used_percent(int)
        current_value = self.memory_bar.value()
        if current_value != new_value:
            self.set_bar_color(current_value, new_value)
            self.memory_bar.setValue(new_value)
            display_str = "%3.1f" % mem_used + "/" + "%3.1f" % mem_avail + " GB " \
                          + "(" + "%d" % new_value + "%" + ")"
            self.memory_bar.setFormat(display_str)
