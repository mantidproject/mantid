# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods
import os
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.kernel import config
from mantid.simpleapi import LoadNMoldyn3Ascii


class LoadNMoldyn3AsciiTest(unittest.TestCase):
    _cdl_filename = "NaF_DISF.cdl"
    _dat_filename = "WSH_test.dat"

    def test_load_fqt_from_cdl(self):
        """
        Load an F(Q, t) function from an nMOLDYN 3 .cdl file
        """

        moldyn_group = LoadNMoldyn3Ascii(Filename=self._cdl_filename, Functions=["Fqt-total"], OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), "NaF_DISF_Fqt-total")

        iqt_ws = moldyn_group[0]
        self.assertTrue(isinstance(iqt_ws, MatrixWorkspace))
        self.assertTrue(iqt_ws.getNumberHistograms(), 1)

        # X axis should be in TOF for an Fqt function
        units = iqt_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(units, "TOF")

    def test_load_sqw_from_cdl(self):
        """
        Load an S(Q, w) function from an nMOLDYN 3 .cdl file
        """
        moldyn_group = LoadNMoldyn3Ascii(Filename=self._cdl_filename, Functions=["Sqw-total"], OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(moldyn_group[0].name(), "NaF_DISF_Sqw-total")

        sqw_ws = moldyn_group[0]
        self.assertTrue(isinstance(sqw_ws, MatrixWorkspace))
        self.assertTrue(sqw_ws.getNumberHistograms(), 1)

        # X axis should be in Energy for an Sqw function
        units = sqw_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(units, "Energy")

    def test_load_dos_from_dat(self):
        """
        Load a density of states from an nMOLDYN 3 .dat file.
        """
        moldyn_ws = LoadNMoldyn3Ascii(Filename="CDOS_Croco_total_10K.dat", OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_ws, MatrixWorkspace))
        self.assertTrue(moldyn_ws.getNumberHistograms(), 1)

        self.assertEqual(moldyn_ws.getAxis(0).getUnit().label(), "THz")
        self.assertEqual(moldyn_ws.YUnitLabel(), "dos-total vs frequency")

    def test_load_from_dat(self):
        """
        Load a function from an nMOLDYN 3 .dat file
        """
        moldyn_ws = LoadNMoldyn3Ascii(Filename=self._dat_filename, OutputWorkspace="__LoadNMoldyn3Ascii_test")

        self.assertTrue(isinstance(moldyn_ws, MatrixWorkspace))
        self.assertTrue(moldyn_ws.getNumberHistograms(), 12)

        workdir = config["defaultsave.directory"]
        filename = "MolDyn_angles.txt"
        path = os.path.join(workdir, filename)
        if os.path.exists(path):
            os.remove(path)

    def test_function_validation_cdl(self):
        """
        Tests that the algorithm cannot be run when no functions are specified
        when loading a .cdl file.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Must specify at least one function when loading a CDL file",
            LoadNMoldyn3Ascii,
            Filename=self._cdl_filename,
            OutputWorkspace="__LoadNMoldyn3Ascii_test",
        )

    def test_function_validation_dat(self):
        """
        Tests that the algorithm cannot be run when functions are specified
        when loading a .dat file.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot specify functions when loading an ASCII file",
            LoadNMoldyn3Ascii,
            Filename=self._dat_filename,
            Functions=["Sqw-total"],
            OutputWorkspace="__LoadNMoldyn3Ascii_test",
        )


if __name__ == "__main__":
    unittest.main()
