# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
System test for GenerateEventsFilter followed by FilterEvents. GenerateEventsFilter creates a SplittersWorkspace
with a high first target index. FilterEvents then shifts all target indexes down so that the output workspace names start from "_1".
"""

from mantid.simpleapi import *
import systemtesting


class FilterEventsWithHighFirstIndexSplitter(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 300

    def requiredFiles(self):
        return ["EQSANS_104088.nxs.h5"]

    def cleanup(self):
        return True

    def runTest(self):
        LoadEventNexus(Filename="EQSANS_104088.nxs.h5", OutputWorkspace="load_tmp")

        GenerateEventsFilter(
            InputWorkspace="load_tmp",
            OutputWorkspace="filter",
            InformationWorkspace="info",
            TimeInterval=None,
            LogName="SampleTemp",
            LogValueInterval=0.1,
        )

        FilterEvents(
            InputWorkspace="load_tmp",
            SplitterWorkspace="filter",
            OutputWorkspaceBaseName="EQSANS_104088",
            InformationWorkspace="info",
            FilterByPulseTime=True,
            GroupWorkspaces=True,
            OutputWorkspaceIndexedFrom1=True,
        )

        total_events = mtd["load_tmp"].getNumberEvents()
        self.assertEqual(total_events, 1368485)
        sum_events = 0
        for i in [1, 2, 3]:
            partial_ws_name = "EQSANS_104088" + "_" + str(i)
            event_count = mtd[partial_ws_name].getNumberEvents()
            sum_events += event_count
        self.assertEqual(total_events, sum_events)
        DeleteWorkspace("load_tmp")
        DeleteWorkspace("info")
        DeleteWorkspace("filter")
        DeleteWorkspace("EQSANS_104088")
        DeleteWorkspace("TOFCorrectWS")
