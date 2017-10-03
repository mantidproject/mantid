from __future__ import (absolute_import, division, print_function)

from PyQt4.QtCore import QThread


class ThreadModel(QThread):
    # a wrapper to allow threading with
    # the MaxEnt algorithm.

    def __init__(self,model):
        QThread.__init__(self)
        self.model=model

    def __del__(self):
        self.wait()

    def run(self):
        self.model.execute()
        self.model.output()
        return

    def setInputs(self,inputs,runName):
        self.model.setInputs(inputs,runName)

    def loadData(self,inputs):
        self.model.loadData(inputs)
