from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


class LoadUtils(object):

    def __init__(self, parent=None):
        self.options = mantid.AnalysisDataService.getObjectNames()
        self.options = [item.replace(" ","") for item in self.options]
        # if periods get the info from the first period
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis_1")
        else:
            tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis")
        self.instrument=tmpWS.getInstrument().getName()
        self.runName=self.instrument+str(tmpWS.getRunNumber()).zfill(8)

    def getCurrentWS(self):
        return self.runName, self.options

    def getRunName(self):
	    return self.runName

    def getInstrument(self):
        return self.instrument
