from __future__ import (absolute_import, division, print_function)

from PyQt4.QtCore import QThread
from PyQt4 import QtCore
from Muon import message_box

def ThreadWrapperSetUp(thread,startSlot,endSlot):
    thread.started.connect(startSlot)
    thread.finished.connect(endSlot)
    thread.exceptionSignal.connect(message_box.warning)  

def ThreadWrapperTearDown(thread,startSlot,endSlot):
    thread.started.disconnect(startSlot)
    thread.finished.disconnect(endSlot)
    thread.exceptionSignal.disconnect(message_box.warning)  

 
class ThreadModel(QThread):

    """
    A wrapper to allow threading with
    the MaxEnt models.
    """
    exceptionSignal = QtCore.pyqtSignal(object)
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
            print ("moo ")
            pass
        except Exception as error:
            print("waa ",error.__class__.__name__)
            self.sendSignal(error)
            pass

    def sendSignal(self,error):
            self.exceptionSignal.emit(error)
            #pass
 
    def join(self):
        print(self.exception)
        if self.exception is not None:
            raise self.exception

    def cancel(self):
        self.model.cancel()

    # if there is one set of inputs (1 alg)
    def setInputs(self,inputs,runName):
        self.model.setInputs(inputs,runName)

    # if there are multiple inputs (alg>1)
    def loadData(self,inputs):
        self.model.loadData(inputs)
