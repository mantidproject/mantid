# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import LoadMuonNexus


class LoadMuonNexusTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # EMU03087 is an old data file produced by CONVERT_NEXUS from MCS binary files.
        # Checked specifically because stores resolution (used to calculate FirstGoodData)
        # as NX_FLOAT32 opposed to NX_INT32 in other Muon files.
        loadResult = LoadMuonNexus(Filename="EMU03087.nxs", OutputWorkspace="EMU03087")

        firstGoodData = loadResult[3]
        self.assertDelta(firstGoodData, 0.416, 0.0001)

    def cleanup(self):
        mtd.remove("EMU03087")
