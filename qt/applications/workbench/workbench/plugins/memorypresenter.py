from qtpy.QtCore import QTimer

from random import random
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
        memory_used = getMemoryUsed()
        #memory_used = int(random() * 100)
        self.view.setValue(memory_used)

