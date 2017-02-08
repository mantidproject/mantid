#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *


class FilteredLoadvsLoadThenFilter(stresstesting.MantidStressTest):
    '''Tests that filtering with LoadEventNexus gives the same answer as loading the whole file and then filtering'''

    def runTest(self):
        filteredLoad = LoadEventNexus("CNCS_7860_event.nxs",FilterByTimeStart=60.0,FilterByTimeStop=120.0,
                                      FilterByTofMin=-1e10,FilterByTofMax=1e10)
        loadAll = LoadEventNexus("CNCS_7860_event.nxs",FilterByTimeStart=-1e10,FilterByTimeStop=1e10,
                                 FilterByTofMin=-1e10,FilterByTofMax=1e10)
        loadAndFilter = FilterByTime(loadAll,StartTime=60.0,StopTime=120.0)
        # This next step is needed otherwise the X boundaries are different causing CompareWorkspaces to fail
        loadAndFilter = RebinToWorkspace(WorkspaceToRebin=loadAndFilter,WorkspaceToMatch=filteredLoad)

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return 'filteredLoad','loadAndFilter'
