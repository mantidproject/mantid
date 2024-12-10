# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import HFIRSANS2Wavelength, CreateWorkspace, AddSampleLog, mtd


class HFIRSANS2WavelengthTest(unittest.TestCase):
    def setUp(self):
        ws = CreateWorkspace(DataX="1,11,111,1,11,111", DataY="2,22,22,22", DataE="1,5,5,5", UnitX="TOF", NSpec=2)
        AddSampleLog(ws, LogName="wavelength", LogText="6.5", LogType="Number Series")
        AddSampleLog(ws, LogName="wavelength_spread", LogText="0.1", LogType="Number Series")

    def tearDown(self):
        mtd.clear()

    def testTOF(self):
        out = HFIRSANS2Wavelength(InputWorkspace="ws")
        self.assertTrue(out)
        self.assertEqual(out.blocksize(), 1)
        self.assertEqual(out.readX(0)[0], 6.175)
        self.assertEqual(out.readX(1)[1], 6.825)
        self.assertEqual(out.readY(0)[0], 24)
        self.assertAlmostEqual(out.readE(1)[0], 7.071067, 5)
        self.assertEqual(out.getAxis(0).getUnit().caption(), "Wavelength")


if __name__ == "__main__":
    unittest.main()
