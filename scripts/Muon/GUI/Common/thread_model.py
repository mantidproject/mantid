from __future__ import (absolute_import, division, print_function)

from PyQt4.QtCore import QThread
from PyQt4 import QtCore
from Muon.GUI.Common import message_box


class ThreadModel(QThread):

    """
    A wrapper to allow threading with
    the MaxEnt models.
    """
    exceptionSignal = QtCore.pyqtSignal(object)

    def __init__(self, model):
        QThread.__init__(self)
        self.model = model

    def __del__(self):
        try:
            self.wait()
        except RuntimeError:
            pass

    def run(self):
        self.user_cancel = False
        try:
            self.model.execute()
            self.model.output()
        except KeyboardInterrupt:
            pass
        except Exception as error:
            if self.user_cancel:
                print("User ended job")
            else:
                self.sendSignal(error)
            pass

    def sendSignal(self, error):
        self.exceptionSignal.emit(error)

    def join(self):
        if self.exception is not None:
            raise self.exception

    def cancel(self):
        self.user_cancel = True
        self.model.cancel()

    # if there is one set of inputs (1 alg)
    def setInputs(self, inputs, runName):
        self.model.setInputs(inputs, runName)

    # if there are multiple inputs (alg>1)
    def loadData(self, inputs):
        self.model.loadData(inputs)

    def threadWrapperSetUp(self, startSlot, endSlot):
        self.started.connect(startSlot)
        self.finished.connect(endSlot)
        self.exceptionSignal.connect(message_box.warning)

    def threadWrapperTearDown(self, startSlot, endSlot):
        self.started.disconnect(startSlot)
        self.finished.disconnect(endSlot)
        self.exceptionSignal.disconnect(message_box.warning)
