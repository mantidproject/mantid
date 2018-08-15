from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QThread
from qtpy import QtWidgets

from PyQt5.QtCore import pyqtSignal

def warning(error):
    print("WARNING FUNCTION")
    QtWidgets.QMessageBox.warning(QtWidgets.QWidget(), "Error", str(error))

class ThreadModel(QThread):
    """
    A wrapper to allow threading with
    the MaxEnt models.
    """
    exceptionSignal = pyqtSignal(object)

    def __init__(self, model):
        super(ThreadModel, self).__init__()
        self.model = model
    #     self.started.connect(self.do_a_print)
    #     self.finished.connect(self.do_a_print_fin)
    #
    # def do_a_print(self):
    #     print("started signal emitted!")
    # def do_a_print_fin(self):
    #     print("finished signal emitted!")

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
        #print("\tSetting up thread wrapper")
        self.started.connect(startSlot)
        self.finished.connect(endSlot)
        self.exceptionSignal.connect(warning)

    def threadWrapperTearDown(self, startSlot, endSlot):
        #print("\tTearing down thread wrapper")
        self.started.disconnect(startSlot)
        self.finished.disconnect(endSlot)
        self.exceptionSignal.disconnect(warning)
