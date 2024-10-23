# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from systemtesting import MantidSystemTest
from mantid.dataobjects import Workspace2D
from mantid.simpleapi import CompareWorkspaces, Load


def get_loader_name(workspace: Workspace2D) -> str:
    history = workspace.getHistory().getAlgorithmHistories()
    return history[0].getPropertyValue("LoaderName")


class LoadEMUWithFloatResolution(MantidSystemTest):
    """
    EMU03087 is an old data file produced by CONVERT_NEXUS from MCS binary files.
    Checked specifically because stores resolution (used to calculate FirstGoodData)
    as NX_FLOAT32 opposed to NX_INT32 in other Muon files.
    """

    def runTest(self):
        output = Load(Filename="EMU03087.nxs", StoreInADS=False)

        workspace = output[0]
        self.assertEquals("LoadMuonNexus", get_loader_name(workspace))

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
        self.assertEquals("LoadMuonNexus", get_loader_name(workspace))

        first_good_data = output[3]
        self.assertDelta(0.656, first_good_data, 0.0001)


class LoadHDF4AndHDF5NexusWhenUsingV2Suffix(MantidSystemTest):
    """
    In cycle 22_4, a HDF4 format file for each run was generated and named 'MUSR00087008.nxs',
    and a HDF5 format file was generated and named 'MUSR00087008.nxs_v2' (i.e. the HDF5 file
    was suffixed with _v2). The HDF4 file would be loaded by default when specifying a run
    number. This test checks that both these files can be loaded, and that the loaded workspaces
    are equivalent. Note that I have changed the position of '_v2' in this test so it is possible
    to load the HDF5 file.
    """

    def runTest(self):
        output_hdf4 = Load(Filename="MUSR00087008.nxs", StoreInADS=False)
        output_hdf5 = Load(Filename="MUSR00087008_v2.nxs", StoreInADS=False)

        workspace_hdf4 = output_hdf4[0]
        self.assertEquals("LoadMuonNexus", get_loader_name(workspace_hdf4))
        workspace_hdf5 = output_hdf5[0]
        self.assertEquals("LoadMuonNexusV2", get_loader_name(workspace_hdf5))

        first_good_data_hdf4 = output_hdf4[3]
        self.assertDelta(0.56, first_good_data_hdf4, 0.0001)
        first_good_data_hdf5 = output_hdf5[3]
        self.assertDelta(0.56, first_good_data_hdf5, 0.0001)

        result, _ = CompareWorkspaces(Workspace1=workspace_hdf4, Workspace2=workspace_hdf5, Tolerance=1e-5)
        self.assertTrue(result)


class LoadHDF4AndHDF5NexusWhenUsingV1Suffix(MantidSystemTest):
    """
    In cycle 24_2, a HDF4 format file for each run was generated and named 'MUSR00093260.nxs_v1',
    and a HDF5 format file was generated and named 'MUSR00093260.nxs' (i.e. the HDF4 file
    was suffixed with _v1). The HDF5 file would be loaded by default when specifying a run
    number. This test checks that both these files can be loaded, and that the loaded workspaces
    are equivalent. Note that I have changed the position of '_v1' in this test so it is possible
    to load the HDF5 file.
    """

    def runTest(self):
        output_hdf4 = Load(Filename="MUSR00093260_v1.nxs", StoreInADS=False)
        output_hdf5 = Load(Filename="MUSR00093260.nxs", StoreInADS=False)

        workspace_hdf4 = output_hdf4[0]
        self.assertEquals("LoadMuonNexus", get_loader_name(workspace_hdf4))
        workspace_hdf5 = output_hdf5[0]
        self.assertEquals("LoadMuonNexusV2", get_loader_name(workspace_hdf5))

        first_good_data_hdf4 = output_hdf4[3]
        self.assertDelta(0.56, first_good_data_hdf4, 0.0001)
        first_good_data_hdf5 = output_hdf5[3]
        self.assertDelta(0.56, first_good_data_hdf5, 0.0001)

        result, _ = CompareWorkspaces(Workspace1=workspace_hdf4, Workspace2=workspace_hdf5, Tolerance=1e-5)
        self.assertTrue(result)
