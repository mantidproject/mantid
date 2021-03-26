# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import HB2AReduce
import os
import tempfile
import unittest
import numpy as np


class HB2AReduceTest(unittest.TestCase):
    def setUp(self):
        self._default_save_directory = tempfile.gettempdir()

    def test_IndividualDetectors(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat', IndividualDetectors=True, SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 44)
        self.assertEqual(HB2AReduce_ws.blocksize(), 121)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 4887)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 2.789331777)
        HB2AReduce_ws.delete()

    def test_NotBinned(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat', BinData=False, SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 5324)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 4887)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 2.789331777)
        HB2AReduce_ws.delete()

    def test_Binned(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat', SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 2.788603131)
        HB2AReduce_ws.delete()

    def test_TwoFiles(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat,HB2A_exp0666_scan0025.dat', SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 2.780263137)
        HB2AReduce_ws.delete()

    def test_Vanadium(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat', Vanadium='HB2A_exp0644_scan0018.dat', SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 78.50058933)
        HB2AReduce_ws.delete()

    def test_ExcludeDetectors(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0666_scan0024.dat', ExcludeDetectors='1-20,40-42', SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 1360)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 283)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.extractY()), 0.826432392)
        HB2AReduce_ws.delete()

    def test_anode_vs_temp(self):
        HB2AReduce_ws = HB2AReduce('HB2A_exp0660_scan0146.dat',
                                   Vanadium='HB2A_exp0644_scan0018.dat',
                                   IndividualDetectors=True,
                                   SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 44)
        self.assertEqual(HB2AReduce_ws.blocksize(), 56)
        self.assertEqual(np.argmax(HB2AReduce_ws.readY(7)), 2)
        self.assertAlmostEquals(np.max(HB2AReduce_ws.readY(7)), 2.13258433)
        HB2AReduce_ws.delete()

    def test_saving_files(self):
        # Test for saving XYE data file.
        HB2AReduce_ws = HB2AReduce(
            'HB2A_exp0660_scan0146.dat',
            Vanadium='HB2A_exp0644_scan0018.dat',
            IndividualDetectors=True,
            OutputFormat='XYE',
            OutputDirectory=self._default_save_directory,
        )
        self.assertTrue(HB2AReduce_ws)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}.dat")))
        # Test for saving GSAS data file.
        HB2AReduce_ws = HB2AReduce(
            'HB2A_exp0735_scan0016.dat',
            Vanadium='HB2A_exp0644_scan0018.dat',
            IndividualDetectors=True,
            OutputFormat='GSAS',
            OutputDirectory=self._default_save_directory,
        )
        self.assertTrue(HB2AReduce_ws)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}.gss")))
        HB2AReduce_ws.delete()


if __name__ == '__main__':
    unittest.main()
