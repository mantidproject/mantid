from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


class LoadUtils(object):

    def __init__(self, parent=None):
        if self.MuonAnalysisExists():
            # get everything from the ADS
            self.options = mantid.AnalysisDataService.getObjectNames()
            self.options = [item.replace(" ","") for item in self.options]
            tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis")
            self.instrument=tmpWS.getInstrument().getName()
            self.runName=self.instrument+str(tmpWS.getRunNumber()).zfill(8)

        else:
            mantid.logger.error("Muon Analysis workspace does not exist - no data loaded")

    def getCurrentWS(self):
        return self.runName, self.options

    def getRunName(self):
        return self.runName

    def getInstrument(self):
        return self.instrument

    def MuonAnalysisExists(self):
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            return True
        else:
            return mantid.AnalysisDataService.doesExist("MuonAnalysis")
