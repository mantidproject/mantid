# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import mtd, MatrixWorkspace, WorkspaceGroup
from mantid.kernel import config
from mantid.simpleapi import SANSILLIntegration, SANSILLReduction


class SANSILLIntegrationTest(unittest.TestCase):
    _facility = None

    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]
        cls._data_search_dirs = config["datasearch.directories"]
        config.appendDataSearchSubDir("ILL/D11/")
        config.appendDataSearchSubDir("ILL/D33/")
        config.setFacility("ILL")
        SANSILLReduction(Run="010569", ProcessAs="Sample", OutputWorkspace="sample", Version=1)

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._data_search_dirs
        mtd.clear()

    def test_monochromatic(self):
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq", CalculateResolution="MildnerCarpenter")
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 88)
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_monochromatic_with_wedges(self):
        SANSILLIntegration(
            InputWorkspace="sample", OutputWorkspace="iq", NumberOfWedges=2, WedgeWorkspace="wedges", CalculateResolution="MildnerCarpenter"
        )
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 88)
        self.assertTrue(mtd["iq"].hasDx(0))
        self.assertTrue(mtd["wedges"])
        self.assertTrue(isinstance(mtd["wedges"], WorkspaceGroup))
        self.assertEqual(mtd["wedges"].getNumberOfEntries(), 2)
        for wedge in range(2):
            self._check_output(mtd["wedges"].getItem(wedge))
            self.assertEqual(mtd["wedges"].getItem(wedge).blocksize(), 88)
            self.assertTrue(mtd["iq"].hasDx(0))

    def test_monochromatic_cake(self):
        SANSILLIntegration(
            InputWorkspace="sample", OutputWorkspace="iq", OutputType="I(Phi,Q)", NumberOfWedges=36, CalculateResolution="MildnerCarpenter"
        )
        self._check_output(mtd["iq"], 36)
        self.assertEqual(mtd["iq"].blocksize(), 88)
        azimuth_axis = mtd["iq"].getAxis(1)
        self.assertTrue(azimuth_axis.isNumeric())
        self.assertEqual(len(azimuth_axis), 36)
        self.assertEqual(azimuth_axis.getUnit().unitID(), "Phi")
        for phi in range(36):
            self.assertTrue(mtd["iq"].hasDx(phi))

    def test_monochromatic_2D(self):
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq", OutputType="I(Qx,Qy)", MaxQxy=0.03, DeltaQ=0.001)
        self._check_output(mtd["iq"], 60)
        self.assertEqual(mtd["iq"].blocksize(), 60)
        qy_axis = mtd["iq"].getAxis(1)
        self.assertTrue(qy_axis.isNumeric())
        self.assertEqual(len(qy_axis), 61)
        self.assertEqual(qy_axis.getUnit().unitID(), "MomentumTransfer")

    def test_with_bin_width(self):
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq", OutputBinning=-0.1, CalculateResolution="MildnerCarpenter")
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 51)
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_with_bin_range(self):
        SANSILLIntegration(
            InputWorkspace="sample", OutputWorkspace="iq", OutputBinning=[0.001, 0.03], CalculateResolution="MildnerCarpenter"
        )
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 73)
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_with_bin_width_and_range(self):
        SANSILLIntegration(
            InputWorkspace="sample", OutputWorkspace="iq", OutputBinning=[0.001, -0.1, 0.03], CalculateResolution="MildnerCarpenter"
        )
        self._check_output(mtd["iq"])
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_custom_binning(self):
        binning = [0.001, 0.005, 0.006, 0.01, 0.016]
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq", OutputBinning=binning, CalculateResolution="MildnerCarpenter")
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 2)
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_resolution_binning(self):
        SANSILLIntegration(
            InputWorkspace="sample", OutputWorkspace="iq", DefaultQBinning="ResolutionBased", CalculateResolution="MildnerCarpenter"
        )
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 37)
        self.assertTrue(mtd["iq"].hasDx(0))

    def test_tof(self):
        # D33 VTOF
        SANSILLReduction(Run="093410", ProcessAs="Sample", OutputWorkspace="tof_sample", Version=1)
        # TOF resolution is not yet implemented
        SANSILLIntegration(InputWorkspace="tof_sample", OutputWorkspace="iq")
        self._check_output(mtd["iq"])
        self.assertEqual(mtd["iq"].blocksize(), 162)

    def _check_output(self, ws, spectra=1):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertTrue(ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getHistory())


if __name__ == "__main__":
    unittest.main()
