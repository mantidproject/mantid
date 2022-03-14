# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from ..memorywidget.memoryinfo import get_memory_info, get_mantid_memory_info
from ...utils.asynchronous import set_interval

TIME_INTERVAL_MEMORY_USAGE_UPDATE = 2.000  # in s


class MemoryPresenter(object):
    """
    Gets system memory usage information and passes it to the memory view this happens at the beginning as well as
    every TIME_INTERVAL_MEMORY_USAGE_UPDATE (s) using threading.Timer; Also, sets the style of the memory(progress) bar
    on construction.
    """
    def __init__(self, view):
        self.view = view
        self.update_allowed = True
        self.set_bar_color_at_start()
        self.update_memory_usage()
        self.update_mantid_memory_usage()
        self.thread_stopper = self.update_memory_usage_threaded()

    def __del__(self):
        self.cancel_memory_update()

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

    @set_interval(TIME_INTERVAL_MEMORY_USAGE_UPDATE)
    def update_memory_usage_threaded(self):
        """
        Calls update_memory_usage once every TIME_INTERVAL_MEMORY_USAGE_UPDATE
        """
        self.update_memory_usage()
        self.update_mantid_memory_usage()

    def update_memory_usage(self):
        """
        Gets memory usage information and passes it to the view
        """
        if self.update_allowed:
            mem_used_percent, mem_used, mem_avail = get_memory_info()
            self.view.invoke_set_value(mem_used_percent, mem_used, mem_avail)

    def update_mantid_memory_usage(self):
        """
        Gets memory usage information and passes it to the view
        """
        if self.update_allowed:
            mantid_mem_used_percent, mantid_mem_used, mem_avail = get_mantid_memory_info()
            self.view.invoke_mantid_set_value(mantid_mem_used_percent, mantid_mem_used, mem_avail)

    def cancel_memory_update(self):
        """
        Ensures that the thread will not restart after it finishes next, as well as attempting to cancel it.
        """
        self.update_allowed = False
        self.thread_stopper.set()
