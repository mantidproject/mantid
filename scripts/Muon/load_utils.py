from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid


class LoadUtils(object):
    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """
    def __init__(self, parent=None):
        exists,tmpWS = self.MuonAnalysisExists()
        if exists:
            self.setUp(tmpWS)
        else:
            raise RuntimeError("No data loaded. \n Please load data using Muon Analysis")

    def setUp(self,tmpWS):
        # get everything from the ADS
        self.options = mantid.AnalysisDataService.getObjectNames()
        self.options = [item.replace(" ","") for item in self.options]
        self.N_points = len(tmpWS.readX(0))
        self.instrument=tmpWS.getInstrument().getName()
        self.runName=self.instrument+str(tmpWS.getRunNumber()).zfill(8)

    # get methods
    def getNPoints(self):
        return self.N_points

    def getCurrentWS(self):
        return self.runName, self.options

    def getRunName(self):
        return self.runName

    def getInstrument(self):
        return self.instrument

    # check if data matches current
    def digit(self,x):
        return int(filter(str.isdigit,x) or 0)

    def hasDataChanged(self):
        exists,ws = self.MuonAnalysisExists()
        if exists:
            current = ws.getInstrument().getName()+str(ws.getRunNumber()).zfill(8)
            if self.runName != current:
                mantid.logger.error("Active workspace has changed. Reloading the data")
                self.setUp(ws)
                return True
        return False

    # check if muon analysis exists
    def MuonAnalysisExists(self):
        # if period data look for the first period
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis_1")
            return True, tmpWS
            # if its not period data
        elif mantid.AnalysisDataService.doesExist("MuonAnalysis"):
            tmpWS=mantid.AnalysisDataService.retrieve("MuonAnalysis")
            return True,tmpWS
        else:
            return False,None

    # Get the groups/pairs for active WS
    # ignore raw files
    def getWorkspaceNames(self):
        # gets all WS in the ADS
        runName,options = self.getCurrentWS()
        final_options=[]
        # only keep the relevant WS (same run as Muon Analysis)
        for pick in options:
            if ";" in pick and "Raw" not in pick and runName in pick:
                final_options.append(pick)
        return final_options

    # Get the groups/pairs for active WS
    def getGroupedWorkspaceNames(self):
        # gets all WS in the ADS
        runName,options = self.getCurrentWS()
        final_options=[]
        # only keep the relevant WS (same run as Muon Analysis)
        for pick in options:
            if "MuonAnalysisGrouped_" in pick and ";" not in pick:
                final_options.append(pick)
        return final_options
