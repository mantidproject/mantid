from qtpy.QtCore import QTimer

from ..memorywidget.memoryinfo import get_memory_info


class MemoryPresenter(object):
    def __init__(self, view):
        self.view = view
        self.update_memory_usage()
        
        # Initial bar color has to be set explicitly
        # as the initial value of the progress bar is
        # undefined
        current_value = self.view.memory_bar.value()
        if (current_value >= 90):
            self.view.set_bar_color(0, current_value)
        elif (current_value < 90):
            self.view.set_bar_color(100, current_value)
        else:
            pass

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_memory_usage)
        self.timer.start(10)

    def update_memory_usage(self):
        mem_used_percent, mem_used, mem_avail = get_memory_info()
        self.view.set_value(mem_used_percent, mem_used, mem_avail)
