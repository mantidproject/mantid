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

        self.start_slot = None
        self.end_slot = None

        self.check_model_has_correct_attributes()
        self.exceptionSignal.connect(message_box.warning)
        self.finished.connect(self.disconnect_exception_slot)

    def check_model_has_correct_attributes(self):
        if hasattr(self.model, "execute") and hasattr(self.model, "output"):
            return
        raise AttributeError("Please ensure the model passed to ThreadModel has implemented"
                             " execute() and output() methods")

    def disconnect_exception_slot(self):
        self.exceptionSignal.disconnect(message_box.warning)

    def __del__(self):
        self.wait()

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
        if not hasattr(self.model, "loadData"):
            raise AttributeError("The model passed to ThreadModel has not implemented"
                                 " loadData() method, which it is attempting to call.")
        self.model.loadData(inputs)

    def threadWrapperSetUp(self, startSlot, endSlot):
        assert hasattr(startSlot, '__call__')
        assert hasattr(endSlot, '__call__')
        self.start_slot, self.end_slot = startSlot, endSlot
        self.started.connect(self.start_slot)
        self.finished.connect(self.end_slot)

        self.finished.connect(self.threadWrapperTearDown)

    def threadWrapperTearDown(self):
        self.started.disconnect(self.start_slot)
        self.finished.disconnect(self.end_slot)
