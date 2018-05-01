from __future__ import (absolute_import, division, print_function)

from PyQt4.QtCore import QThread


class ThreadModel(QThread):

    """
    A wrapper to allow threading with
    the MaxEnt models.
    """
    def __init__(self,model):
        QThread.__init__(self)
        self.model=model

    def __del__(self):
        self.wait()

    def run(self):
        try:
            self.model.execute()
            self.model.output()
        except KeyboardInterrupt:
            pass

    def cancel(self):
        self.model.cancel()

    # if there is one set of inputs (1 alg)
    def setInputs(self,inputs,runName):
        self.model.setInputs(inputs,runName)

    # if there are multiple inputs (alg>1)
    def loadData(self,inputs):
        self.model.loadData(inputs)
