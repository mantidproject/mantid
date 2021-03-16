from qtpy.QtCore import QTimer

from workbench.plugins.memoryinfo import getMemoryUsed

class MemoryPresenter(object):
    def __init__(self, view):

        self.view = view
        self.updateMemoryUsage()
        self.view.updateSignal.connect(self.updateMemoryUsage)

        self.timer = QTimer()
        self.timer.timeout.connect(self.view.onUpdateRequest)
        self.timer.start(10)

    def updateMemoryUsage(self):
        mem_used_percent, mem_used, mem_avail = getMemoryUsed()
        self.view.setValue(mem_used_percent, mem_used, mem_avail)

