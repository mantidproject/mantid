from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


class LoadUtils(object):

    def __init__(self, parent=None):
        self.options = mantid.AnalysisDataService.getObjectNames()
        self.options = [item.replace(" ","") for item in self.options]
        # for some reason this doesn't work
        tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis")
        self.runName=tmpWS.getInstrument().getName()+str(tmpWS.getRunNumber()).zfill(8)

    def getCurrentWS(self):
        return self.runName, self.options
