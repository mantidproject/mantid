# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import Load


class LoadEMUWithFloatResolution(MantidSystemTest):
    """
    EMU03087 is an old data file produced by CONVERT_NEXUS from MCS binary files.
    Checked specifically because stores resolution (used to calculate FirstGoodData)
    as NX_FLOAT32 opposed to NX_INT32 in other Muon files.
    """

    def runTest(self):
        output = Load(Filename="EMU03087.nxs", StoreInADS=False)

        workspace = output[0]
        history = workspace.getHistory().getAlgorithmHistories()
        self.assertEquals("LoadMuonNexus", history[0].getPropertyValue("LoaderName"))

        first_good_data = output[3]
        self.assertDelta(0.416, first_good_data, 0.0001)


class LoadHDF4NexusFromMUSR(MantidSystemTest):
    """
    MUSR00032900 is an old data file which uses the HDF4 format. It was generated in
    cycle 11_1, which was back when Muon scientists only generated data in HDF4 format.
    """

    def runTest(self):
        output = Load(Filename="MUSR00032900.nxs", StoreInADS=False)

        workspace = output[0]
        history = workspace.getHistory().getAlgorithmHistories()
        self.assertEquals("LoadMuonNexus", history[0].getPropertyValue("LoaderName"))

        first_good_data = output[3]
        self.assertDelta(0.656, first_good_data, 0.0001)
