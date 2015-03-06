import stresstesting
from mantid.simpleapi import *

'''Tests that filtering with LoadEventNexus gives the same answer as loading the whole file and then filtering'''
class FilteredLoadvsLoadThenFilter(stresstesting.MantidStressTest):
    
  def runTest(self):
    filteredLoad = LoadEventNexus("CNCS_7860_event.nxs",FilterByTimeStart=60.0,FilterByTimeStop=120.0,FilterByTofMin=-1e10,FilterByTofMax=1e10)
    loadAll = LoadEventNexus("CNCS_7860_event.nxs",FilterByTimeStart=-1e10,FilterByTimeStop=1e10,FilterByTofMin=-1e10,FilterByTofMax=1e10)
    loadAndFilter = FilterByTime(loadAll,StartTime=60.0,StopTime=120.0)
    # This next step is needed otherwise the X boundaries are different causing CheckWorkspacesMatch to fail
    loadAndFilter = RebinToWorkspace(WorkspaceToRebin=loadAndFilter,WorkspaceToMatch=filteredLoad)

  def validateMethod(self):
    return "ValidateWorkspaceToWorkspace"

  def validate(self):
    return 'filteredLoad','loadAndFilter'
