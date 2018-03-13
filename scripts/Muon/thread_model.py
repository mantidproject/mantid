from __future__ import (absolute_import, division, print_function)

from PyQt4.QtCore import QThread
import mantid.simpleapi as mantid


class ThreadModel(QThread):

    """
    A wrapper to allow threading with
    the MaxEnt models.
    """
    def __init__(self,model,run):
        QThread.__init__(self)
        self.model=model
        self.run = run

    def __del__(self):
        self.wait()
 
    def digit(self,x):
        return int(filter(str.isdigit,x) or 0)

    def hasDataChanged(self):
        ws = mantid.AnalysisDataService.retrieve("MuonAnalysis")
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            ws = mantid.AnalysisDataService.retrieve("MuonAnalysis_1")

        current = ws.getInstrument().getName()+str(ws.getRunNumber()).zfill(8)
        if self.run != current:
            mantid.logger.error("Active workspace has changed. Restart this interface")
            return True
        return False

    def run(self):
       if self.hasDataChanged():
           return
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
