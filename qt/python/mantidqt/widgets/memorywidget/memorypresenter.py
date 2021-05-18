# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from threading import Timer

from ..memorywidget.memoryinfo import get_memory_info

TIME_INTERVAL_MEMORY_USAGE_UPDATE = 2.000  # in s


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
        self.timer = Timer(TIME_INTERVAL_MEMORY_USAGE_UPDATE, self.update_memory_usage)
        self.thread_on = False
        self.updating_cancelled = False
        self.closing_workbench = False
        self.update_memory_usage()
        self.set_bar_color_at_start()
        self.start_memory_widget_update_thread()

    def __del__(self):
        self.cancel_memory_update()

    def start_memory_widget_update_thread(self):
        """
        Sets timer so that the memory usage is updated
        every TIME_INTERVAL_MEMORY_USAGE_UPDATE (ms)
        """
        if not self.thread_on:
            self.timer.start()

    def _spin_off_another_time_thread(self):
        """
        Spins off another timer thread, by creating a new Timer thread object and starting it
        """
        if not self.thread_on and not self.closing_workbench:
            self.timer = Timer(TIME_INTERVAL_MEMORY_USAGE_UPDATE, self.update_memory_usage)
            self.timer.start()
            self.thread_on = True

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
        if not self.updating_cancelled:
            self.thread_on = False
            mem_used_percent, mem_used, mem_avail = get_memory_info()
            self.view.set_value(mem_used_percent, mem_used, mem_avail)
            self._spin_off_another_time_thread()

    def cancel_memory_update(self):
        self.timer.cancel()
        self.updating_cancelled = True
        self.thread_on = False
