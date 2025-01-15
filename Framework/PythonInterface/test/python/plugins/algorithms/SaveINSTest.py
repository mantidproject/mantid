# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SaveINS, CreateSampleWorkspace, ClearUB, SetUB, SetSample, AnalysisDataService as ADS
from mantid.geometry import CrystalStructure
import tempfile
from os import path
import shutil


class SaveINSTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._tmp_directory = tempfile.mkdtemp()
        cls.xtal = CrystalStructure("1 1 1", "P 1 21/n 1", "")  # only used for spacegroup
        cls.file_start = [
            "TITL ws\n",
            "REM This file was produced by mantid using SaveINS\n",
            "CELL 1.0 7.6508 13.2431 11.6243 90.0000 104.1183 90.0000\n",
            "ZERR 4 0.0000 0.0000 0.0000 0.0000 0.0000 0.0000\n",
            "LATT 1\n",
            "SYMM -x+1/2,y+1/2,-z+1/2\n",
            "NEUT\n",
        ]
        cls.file_end = ["UNIT 48 36 12 8 4\n", "MERG 0\n", "HKLF 2\n", "END"]

    @classmethod
    def tearDownClass(cls):
        ADS.clear()

    def setUp(self):
        self.ws = CreateSampleWorkspace(OutputWorkspace="ws", NumBanks=1, BankPixelWidth=1, BinWidth=20000)  # 1 bin
        SetUB(Workspace=self.ws, a=7.6508, b=13.2431, c=11.6243, alpha=90, beta=104.1183, gamma=90)
        ndensity = 4 / self.ws.sample().getOrientedLattice().volume()
        SetSample(InputWorkspace=self.ws, Material={"ChemicalFormula": "C12 H9 N3 O2 S1", "SampleNumberDensity": ndensity})

    def tearDown(self):
        shutil.rmtree(self._tmp_directory)

    def test_save_ins_throws_if_invalid_spgr(self):
        output_file = path.join(self._tmp_directory, "test.ins")

        self.assertRaisesRegex(
            RuntimeError,
            "Not a valid spacegroup symbol",
            SaveINS,
            Filename=output_file,
            InputWorkspace=self.ws,
            Spacegroup="invalid",
        )

        self.assertFalse(path.exists(output_file))

    def test_save_ins_throws_if_no_oriented_lattice(self):
        ClearUB(self.ws)
        output_file = path.join(self._tmp_directory, "test.ins")

        self.assertRaisesRegex(
            RuntimeError,
            "Workspace must have an oriented lattice defined.",
            SaveINS,
            Filename=output_file,
            InputWorkspace=self.ws,
            Spacegroup="P 1 21/n 1",
        )

        self.assertFalse(path.exists(output_file))

    def test_save_ins_throws_if_no_spgr_or_crystal_structure(self):
        output_file = path.join(self._tmp_directory, "test.ins")

        self.assertRaisesRegex(
            RuntimeError, "The workspace does not have a crystal structure defined", SaveINS, InputWorkspace=self.ws, Filename=output_file
        )
        self.assertFalse(path.exists(output_file))

    def test_save_ins_throws_if_no_sample_material(self):
        ws_nomat = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1, BinWidth=20000)
        SetUB(Workspace=ws_nomat)
        output_file = path.join(self._tmp_directory, "test.ins")

        self.assertRaisesRegex(
            RuntimeError, "Workspace must have a sample material set.", SaveINS, InputWorkspace=ws_nomat, Filename=output_file
        )
        self.assertFalse(path.exists(output_file))

    def test_save_ins_natural_isotopic_abundance_true(self):
        output_file = path.join(self._tmp_directory, "test1.ins")

        SaveINS(InputWorkspace=self.ws, Filename=output_file, Spacegroup="P 1 21/n 1")

        expected_lines = [*self.file_start, "SFAC C H N O S\n", *self.file_end]
        self._assert_file_contents(output_file, expected_lines)

    def test_save_ins_get_spacegroup_from_crystal_structure(self):
        self.ws.sample().setCrystalStructure(self.xtal)
        output_file = path.join(self._tmp_directory, "test2.ins")

        SaveINS(InputWorkspace=self.ws, Filename=output_file)

        expected_lines = [*self.file_start, "SFAC C H N O S\n", *self.file_end]
        self._assert_file_contents(output_file, expected_lines)

    def test_save_ins_natural_isotopic_abundance_false(self):
        output_file = path.join(self._tmp_directory, "test3.ins")

        SaveINS(InputWorkspace=self.ws, Filename=output_file, Spacegroup="P 1 21/n 1", UseNaturalIsotopicAbundances=False)

        expected_lines = [
            *self.file_start,
            "SFAC C 0 0 0 0 0 0 0 0 6.6460 0 0 5.5529 1.0 12.0107\n",
            "SFAC H 0 0 0 0 0 0 0 0 3.7390 0 0 39.8000 1.0 1.0079\n",
            "SFAC N 0 0 0 0 0 0 0 0 9.3600 0 0 12.5667 1.0 14.0067\n",
            "SFAC O 0 0 0 0 0 0 0 0 5.8030 0 0 4.2321 1.0 15.9994\n",
            "SFAC S 0 0 0 0 0 0 0 0 2.8470 0 0 1.3208 1.0 32.0650\n",
            *self.file_end,
        ]

        self._assert_file_contents(output_file, expected_lines)

    def test_save_ins_constant_wavelength(self):
        output_file = path.join(self._tmp_directory, "test4.ins")
        wl = 2.5
        self.ws.run().addProperty("wavelength", wl, True)

        SaveINS(InputWorkspace=self.ws, Filename=output_file, Spacegroup="P 1 21/n 1")

        expected_lines = [
            *self.file_start[0:2],
            f"CELL {wl:.1f} 7.6508 13.2431 11.6243 90.0000 104.1183 90.0000\n",
            *self.file_start[3:],
            "SFAC C H N O S\n",
            *self.file_end,
        ]

        self._assert_file_contents(output_file, expected_lines)

    def _assert_file_contents(self, filepath, expected_lines):
        with open(filepath, "r") as f:
            lines = f.readlines()
        self.assertEqual(len(lines), len(expected_lines))
        for iline, line in enumerate(lines):
            self.assertEqual(line, expected_lines[iline])


if __name__ == "__main__":
    unittest.main()
