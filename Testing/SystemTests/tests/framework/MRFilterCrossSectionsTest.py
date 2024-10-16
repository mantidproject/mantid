# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
System test for Magnetism Reflectometer cross-sections algorithm utilizing FilterEvents
on a big data set comprising ~1.2*10^6 events being split by ~15*10^6 time boundaries
"""

from mantid.api import mtd
from mantid.simpleapi import DeleteWorkspace, MRFilterCrossSections
import systemtesting


class MRFilterCrossSectionsTest(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 100

    def requiredFiles(self):
        return ["REF_M_40380.nxs.h5"]

    def cleanup(self):
        return True

    def runTest(self):
        # Data information
        # Log = WaveTriggerDownstairs
        # Int32TimeSeriesProperty with 15211333 values (either 0 or 1)
        # 1,282,919 events, acquired in 2 hours
        ws_stem_name = "SazKfLZFoIvn_entry"
        MRFilterCrossSections(
            Filename="REF_M_40380.nxs.h5",
            PolState="WaveTriggerDownstairs",
            AnaState="",
            PolVeto="",
            AnaVeto="",
            CheckDevices=False,
            CrossSectionWorkspaces=ws_stem_name,
        )

        expected_event_count = {"on_on": 2, "on_off": 635929, "off_on": 1, "off_off": 646987}
        for suffix in ["on_on", "on_off", "off_on", "off_off"]:
            partial_ws_name = ws_stem_name + "_" + suffix
            event_count = mtd[partial_ws_name].getNumberEvents()
            self.assertEqual(event_count, expected_event_count[suffix])
            DeleteWorkspace(partial_ws_name)
