# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import mtd, config, ILLLagrange
import unittest


class ILLLagrangeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]
        cls._dirs = config["datasearch.directories"]
        config.appendDataSearchSubDir("ILL/IN1/")
        config["default.facility"] = "ILL"

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._dirs

    def tearDown(self):
        mtd.clear()

    def test_simple_reduction(self):
        sample_file = '012882'
        ILLLagrange(OutputWorkspace="out",
                    SampleRuns=sample_file)

        ws = mtd["out"]

        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.getNumberBins(0), 160)

    def test_multiple_files_reduction(self):
        sample_files = '012882:012884'
        ILLLagrange(OutputWorkspace="out",
                    SampleRuns=sample_files)

        ws = mtd["out"]

        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.getNumberBins(0), 430)

    def test_reduction_single_monochromator(self):
        """
        Full reduction of data from a single monochromator
        """
        sample_files = '012882:012884'
        water_correction = 'correction-water-cu220-2020.txt'
        empty_cell = '012869:012871'

        ILLLagrange(OutputWorkspace="out",
                    SampleRuns=sample_files,
                    CorrectionFile=water_correction,
                    ContainerRuns=empty_cell)

        ws = mtd["out"]

        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.getNumberBins(0), 430)


if __name__ == "__main__":
    unittest.main()
