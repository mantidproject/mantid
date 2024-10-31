# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import os
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.kernel import config
from mantid.simpleapi import CreateSampleWorkspace, MolDyn


class MolDynTest(unittest.TestCase):
    def test_load_version3_cdl(self):
        """
        Load a function from a nMOLDYN 3 .cdl file
        """

        moldyn_group = MolDyn(Data="NaF_DISF.cdl", Functions=["Sqw-total"], OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))

    def test_load_version3_dat(self):
        """
        Load a function from an nMOLDYN 3 .dat file
        """

        moldyn_ws = MolDyn(Data="WSH_test.dat", OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_ws, MatrixWorkspace))

    def test_load_version4(self):
        """
        Load a function from an nMOLDYN 4 export.
        """
        # This test requires the directory to be provided, this is in the
        # UnitTest directory so do get this from the serch directories
        data_dirs = config["datasearch.directories"].split(";")
        unit_test_data_dir = [p for p in data_dirs if "UnitTest" in p][0]
        data_directory = os.path.join(unit_test_data_dir, "nmoldyn4_data")

        function_ws = MolDyn(Data=data_directory, Functions=["fqt_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test")

        self.assertTrue(isinstance(function_ws, WorkspaceGroup))

    def test_loadSqwWithEMax(self):
        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Data="NaF_DISF.cdl", Functions=["Sqw-total"], MaxEnergy="1.0")

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), "NaF_DISF_Sqw-total")

        # Get max enery from result workspace
        x_data = moldyn_group[0].dataX(0)
        x_max = x_data[len(x_data) - 1]

        # Check that it is less that what was passed to algorithm
        self.assertLessEqual(x_max, 1.0)

    def test_loadSqwWithSymm(self):
        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Data="NaF_DISF.cdl", Functions=["Sqw-total"], SymmetriseEnergy=True)

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), "NaF_DISF_Sqw-total")

        # Get max and min energy from result workspace
        x_data = moldyn_group[0].dataX(0)
        x_max = x_data[len(x_data) - 1]
        x_min = x_data[0]

        # abs(min) should equal abs(max)
        self.assertEqual(x_max, -x_min)

    def test_loadSqwWithRes(self):
        # Create a sample workspace that looks like an instrument resolution, area under curve ~1
        sample_res = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=Gaussian, PeakCentre=0, Height=3.99, Sigma=0.1",
            NumBanks=1,
            BankPixelWidth=1,
            XUnit="Energy",
            XMin=-10,
            XMax=10,
            BinWidth=0.1,
        )

        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Data="NaF_DISF.cdl", Functions=["Sqw-total"], MaxEnergy="1.0", SymmetriseEnergy=True, Resolution=sample_res)

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), "NaF_DISF_Sqw-total")


if __name__ == "__main__":
    unittest.main()
