from qtpy.QtCore import QTimer

from ..memorywidget.memoryinfo import get_memory_info


class MemoryPresenter(object):
    def __init__(self, view):
        self.view = view
        self.update_memory_usage()
        self.set_bar_color_at_start()
        self.update_at_regular_intervals()

    def update_at_regular_intervals(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_memory_usage)
        self.timer.start(10)

    def set_bar_color_at_start(self):
        current_value = self.view.memory_bar.value()
        if (current_value >= 90):
            self.view.set_bar_color(0, current_value)
        elif (current_value < 90):
            self.view.set_bar_color(100, current_value)
        else:
            pass

    def update_memory_usage(self):
        mem_used_percent, mem_used, mem_avail = get_memory_info()
        self.view.set_value(mem_used_percent, mem_used, mem_avail)
