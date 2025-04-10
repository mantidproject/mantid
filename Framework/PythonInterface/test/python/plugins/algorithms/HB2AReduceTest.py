# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import HB2AReduce, mtd
import os
import tempfile
import unittest
import numpy as np


class HB2AReduceTest(unittest.TestCase):
    def setUp(self):
        self._default_save_directory = tempfile.gettempdir()

    def test_IndividualDetectors(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat", IndividualDetectors=True, SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 44)
        self.assertEqual(HB2AReduce_ws.blocksize(), 121)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 4887)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 2.789331777)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 44)
        self.assertEqual(norm_time_wsk.blocksize(), 121)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 4819)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 707.946308236)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_mergeRuns(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0954_scan0017.dat,HB2A_exp0954_scan0018.dat,HB2A_exp0954_scan0022.dat", SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 724)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 2.316557925)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 2439)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 724)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 29.890032119)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_NotBinned(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat", BinData=False, SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 5324)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 4887)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 2.789331777)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 5324)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 4819)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 707.946308236)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_Binned(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat", SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 2.788603131)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 2439)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 2203)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 707.480233538)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_TwoFiles(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat,HB2A_exp0666_scan0025.dat", SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 2.780263137)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 2439)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 2203)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 712.465597607)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_Vanadium(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat", Vanadium="HB2A_exp0644_scan0018.dat", SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 2439)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 2203)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 78.50058933)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 2439)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 2203)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 19915.92659908)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_ExcludeDetectors(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0666_scan0024.dat", ExcludeDetectors="1-20,40-42", SaveData=False)
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 1)
        self.assertEqual(HB2AReduce_ws.blocksize(), 1360)
        self.assertEqual(np.argmax(HB2AReduce_ws.extractY()), 283)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.extractY()), 0.826432392)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 1)
        self.assertEqual(norm_time_wsk.blocksize(), 1360)
        self.assertEqual(np.argmax(norm_time_wsk.extractY()), 283)
        self.assertAlmostEqual(np.max(norm_time_wsk.extractY()), 209.011638786)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_anode_vs_temp(self):
        HB2AReduce_ws = HB2AReduce(
            "HB2A_exp0660_scan0146.dat", Vanadium="HB2A_exp0644_scan0018.dat", IndividualDetectors=True, SaveData=False
        )
        self.assertTrue(HB2AReduce_ws)
        self.assertEqual(HB2AReduce_ws.getNumberHistograms(), 44)
        self.assertEqual(HB2AReduce_ws.blocksize(), 56)
        self.assertEqual(np.argmax(HB2AReduce_ws.readY(7)), 2)
        self.assertAlmostEqual(np.max(HB2AReduce_ws.readY(7)), 2.13258433)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertEqual(norm_time_wsk.getNumberHistograms(), 44)
        self.assertEqual(norm_time_wsk.blocksize(), 56)
        self.assertEqual(np.argmax(norm_time_wsk.readY(7)), 2)
        self.assertAlmostEqual(np.max(norm_time_wsk.readY(7)), 1195.31351784)
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_saving_files(self):
        # Test for saving XYE data file.
        HB2AReduce_ws = HB2AReduce(
            "HB2A_exp0660_scan0146.dat",
            Vanadium="HB2A_exp0644_scan0018.dat",
            IndividualDetectors=True,
            OutputFormat="XYE",
            OutputDirectory=self._default_save_directory,
        )
        self.assertTrue(HB2AReduce_ws)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}.dat")))
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}_norm_time.dat")))
        # Test for saving GSAS data file.
        HB2AReduce_ws = HB2AReduce(
            "HB2A_exp0735_scan0016.dat",
            Vanadium="HB2A_exp0644_scan0018.dat",
            IndividualDetectors=True,
            OutputFormat="GSAS",
            OutputDirectory=self._default_save_directory,
        )
        self.assertTrue(HB2AReduce_ws)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}.gss")))
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)
        self.assertTrue(os.path.exists(os.path.join(self._default_save_directory, f"{HB2AReduce_ws}_norm_time.gss")))
        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

    def test_new_convention(self):
        HB2AReduce_ws_old = HB2AReduce("HB2A_exp0666_scan0024.dat", IndividualDetectors=True, SaveData=False)
        norm_time_wsk = mtd[HB2AReduce_ws_old.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)

        HB2AReduce_ws_old.delete()
        norm_time_wsk.delete()

        HB2AReduce_ws_new = HB2AReduce("HB2A_exp0742_scan0028.dat", IndividualDetectors=True, SaveData=False)
        norm_time_wsk = mtd[HB2AReduce_ws_new.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)

        HB2AReduce_ws_new.delete()
        norm_time_wsk.delete()

    def test_new_new_convention(self):
        HB2AReduce_ws = HB2AReduce("HB2A_exp0755_scan0027.dat", IndividualDetectors=True, SaveData=False)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)

        HB2AReduce_ws.delete()
        norm_time_wsk.delete()

        HB2AReduce_ws = HB2AReduce("HB2A_exp0882_scan0012.dat", IndividualDetectors=True, SaveData=False)
        norm_time_wsk = mtd[HB2AReduce_ws.name() + "_norm_time"]
        self.assertTrue(norm_time_wsk)

        HB2AReduce_ws.delete()
        norm_time_wsk.delete()


if __name__ == "__main__":
    unittest.main()
