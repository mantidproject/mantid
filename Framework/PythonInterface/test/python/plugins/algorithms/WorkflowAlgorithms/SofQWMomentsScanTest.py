# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import SofQWMomentsScan, DeleteWorkspace
from mantid import mtd
from mantid.api import AlgorithmManager


class SofQWMomentsScanTest(unittest.TestCase):
    def test_accepts_osiris_silicon_analyser(self):
        alg = AlgorithmManager.createUnmanaged("SofQWMomentsScan")
        alg.initialize()

        alg.setProperty("Instrument", "OSIRIS")
        alg.setProperty("Analyser", "silicon")
        alg.setProperty("Reflection", "111")

        self.assertEqual(alg.getPropertyValue("Instrument"), "OSIRIS")
        self.assertEqual(alg.getPropertyValue("Analyser"), "silicon")
        self.assertEqual(alg.getPropertyValue("Reflection"), "111")

    def test_sqw_moments_scan(self):
        SofQWMomentsScan(
            InputFiles="OSIRIS100320",
            Instrument="OSIRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange="963,1004",
            QRange="0,0.1,2",
            EnergyRange="-0.4,0.01,0.4",
        )

        sqw = mtd["Sqw"][0]
        self.assertEqual(sqw.getNumberHistograms(), 20)
        self.assertEqual(sqw.blocksize(), 80)

    def test_multiple_scan(self):
        SofQWMomentsScan(
            InputFiles="OSIRIS100320, OSIRIS100321",
            Instrument="OSIRIS",
            Analyser="graphite",
            Reflection="002",
            SpectraRange="963,1004",
            QRange="0,0.1,2",
            EnergyRange="-0.4,0.01,0.4",
        )

    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        DeleteWorkspace(mtd["reduced"])
        DeleteWorkspace(mtd["sqw"])


if __name__ == "__main__":
    unittest.main()
