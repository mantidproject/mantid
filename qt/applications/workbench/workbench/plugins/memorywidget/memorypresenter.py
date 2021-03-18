from qtpy.QtCore import QTimer

from ..memorywidget.memoryinfo import get_memory_info

class MemoryPresenter(object):
    def __init__(self, view):

        self.view = view
        self.view.memory_bar.setValue(100)
        self.update_memory_usage()

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_memory_usage)
        self.timer.start(10)

    def update_memory_usage(self):
        mem_used_percent, mem_used, mem_avail = get_memory_info()
        self.view.set_value(mem_used_percent, mem_used, mem_avail)
