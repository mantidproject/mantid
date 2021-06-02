# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from qtpy.QtWidgets import QWidget, QProgressBar
from qtpy.QtCore import Qt, Slot, Signal

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

CRITICAL_PERCENTAGE = 90


def from_normal_to_critical(critical: int, current_value: int, new_value: int) -> bool:
    """
    Returns true if the memory usage is going from normal to critical stage
    or false otherwise
    :param critical: Critical limit for memory usage in percentage
    :param current_value: Used system memory in percentage from previous update
    :param new_value: Latest used system memory in percentage
    """
    return current_value < critical <= new_value


def from_critical_to_normal(critical: int, current_value: int, new_value: int) -> bool:
    """
    Returns true if the memory usage is going from critical to normal stage
    or false otherwise
    :param critical: Critical limit for memory usage in percentage
    :param current_value: Used system memory in percentage from previous update
    :param new_value: Latest used system memory in percentage
    """
    return current_value >= critical > new_value


class MemoryView(QWidget):
    set_value = Signal(int, float, float)
    """
    Initializes and updates the view of memory(progress) bar.
    """
    def __init__(self, parent):
        super(MemoryView, self).__init__(parent)
        self.critical = CRITICAL_PERCENTAGE
        self.memory_bar = QProgressBar(self)
        self.memory_bar.setAlignment(Qt.AlignCenter)
        self.set_value.connect(self._set_value)

    def set_bar_color(self, current_value: int, new_value: int):
        """
        Updates the memory(progress) bar style if needed
        :param current_value: Used system memory in percentage from previous update
        :param new_value: Latest used system memory in percentage
        """
        if from_normal_to_critical(self.critical, current_value, new_value):
            self.memory_bar.setStyleSheet(CRITICAL_STYLE)
        elif from_critical_to_normal(self.critical, current_value, new_value):
            self.memory_bar.setStyleSheet(NORMAL_STYLE)
        else:
            pass

    def invoke_set_value(self, new_value: int, mem_used: float, mem_avail: float):
        self.set_value.emit(new_value, mem_used, mem_avail)

    @Slot(int, float, float)
    def _set_value(self, new_value, mem_used, mem_avail):
        """
        Receives memory usage information passed by memory presenter
        and updates the displayed content as well as the style if needed
        :param new_value: Latest used system memory in percentage
        :param mem_used: Used system memory in Gigabytes(GB)
        :param mem_avail: Available system memory in GB
        """
        # newValue is the latest mem_used_percent
        current_value = self.memory_bar.value()
        if current_value == new_value:
            return
        self.set_bar_color(current_value, new_value)
        self.memory_bar.setValue(new_value)
        display_str = f"{mem_used:3.1f}/{mem_avail:3.1f} GB ({new_value:d}%)"
        self.memory_bar.setFormat(display_str)
