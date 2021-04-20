# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from qtpy.QtCore import QTimer

from ..memorywidget.memoryinfo import get_memory_info

TIME_INTERVAL_MEMORY_USAGE_UPDATE = 2000  # in ms


class MemoryPresenter(object):
    """
    Gets system memory usage information and passes it to
    the memory view
    This happens at the beginning as well as every
    TIME_INTERVAL_MEMORY_USAGE_UPDATE (ms) using QTimer
    Besides, sets the style of the memory(progress) bar
    at the beginning
    """
    def __init__(self, view):
        self.view = view
        self.timer = QTimer()
        self.update_memory_usage()
        self.set_bar_color_at_start()
        self.update_at_regular_intervals()

    def update_at_regular_intervals(self):
        """
        Sets timer so that the memory usage is updated
        every TIME_INTERVAL_MEMORY_USAGE_UPDATE (ms)
        """
        self.timer.timeout.connect(self.update_memory_usage)
        self.timer.start(TIME_INTERVAL_MEMORY_USAGE_UPDATE)

    def set_bar_color_at_start(self):
        """
        Sets style of the memory(progress) bar at the start
        """
        current_value = self.view.memory_bar.value()
        if current_value >= self.view.critical:
            self.view.set_bar_color(0, current_value)
        elif current_value < self.view.critical:
            self.view.set_bar_color(100, current_value)
        else:
            pass

    def update_memory_usage(self):
        """
        Gets memory usage information and passes it to the view
        """
        mem_used_percent, mem_used, mem_avail = get_memory_info()
        self.view.set_value(mem_used_percent, mem_used, mem_avail)
