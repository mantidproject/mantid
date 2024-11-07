# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import MatrixWorkspace
from mantid.simpleapi import SANSILLParameterScan, config, mtd


class SANSILLParameterScanTest(unittest.TestCase):
    _facility = None

    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]
        cls._data_search_dirs = config["datasearch.directories"]
        config.appendDataSearchSubDir("ILL/D16/")

    def setUp(self):
        self._facility = config["default.facility"]
        config.setFacility("ILL")

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._data_search_dirs
        mtd.clear()

    def test_D16_omega(self):
        output_name = "output2d"
        SANSILLParameterScan(
            SampleRun="066416.nxs",
            OutputWorkspace=output_name,
            OutputJoinedWorkspace="reduced",
            Observable="Omega.value",
            PixelYmin=3,
            PixelYMax=189,
        )

        ws = mtd[output_name]
        self._check_output(ws, 1, 1152)
        self.assertAlmostEqual(ws.getAxis(0).getValue(0), -9.8136, delta=4)
        self.assertAlmostEqual(ws.getAxis(0).getValue(1151), 76.2048, delta=4)
        self.assertAlmostEqual(ws.getAxis(1).getValue(0), 0.0, delta=3)
        self.assertTrue("reduced" in mtd)

    def test_D16_omega_container(self):
        output_name = "output2d"
        SANSILLParameterScan(
            SampleRun="213_exp_9-13-1035_25786-26046.nxs",
            ContainerRun="003674.nxs",
            OutputWorkspace=output_name,
            Observable="Omega.value",
            PixelYmin=120,
            PixelYMax=160,
        )
        ws = mtd[output_name]
        self._check_output(ws, 261, 260)
        self.assertAlmostEqual(ws.getAxis(0).getValue(0), 3.1672, delta=4)
        self.assertAlmostEqual(ws.getAxis(0).getValue(260), 22.2328, delta=4)
        self.assertAlmostEqual(ws.getAxis(1).getValue(0), -1.0, delta=3)
        self.assertAlmostEqual(ws.getAxis(1).getValue(1), -0.95, delta=3)

    def test_D16_omega_scanning_container(self):
        output_name = "output2d"
        SANSILLParameterScan(
            SampleRun="212_8-02-927_023094-23354_sample.nxs",
            ContainerRun="212_8-02-927_046364-046624_container.nxs",
            OutputWorkspace=output_name,
            Observable="Omega.value",
            PixelYmin=120,
            PixelYMax=160,
        )
        ws = mtd[output_name]
        self._check_output(ws, 261, 319)
        self.assertAlmostEqual(ws.getAxis(0).getValue(0), 2.7702, delta=4)
        self.assertAlmostEqual(ws.getAxis(0).getValue(319), 21.8357, delta=4)
        self.assertAlmostEqual(ws.getAxis(1).getValue(0), -1.0, delta=3)
        self.assertAlmostEqual(ws.getAxis(1).getValue(1), -0.95, delta=3)
        self.assertAlmostEqual(ws.getAxis(1).getValue(260), 12.0, delta=3)

    def _check_output(self, ws, spectra, blocksize):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertEqual(str(ws.getAxis(0).getUnit().symbol()), "degrees")
        self.assertEqual(ws.getAxis(0).getUnit().caption(), "Scattering angle")
        self.assertEqual(str(ws.getAxis(1).getUnit().symbol()), "degrees")
        self.assertEqual(ws.getAxis(1).getUnit().caption(), "Omega.value")
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getHistory())
        self.assertTrue(ws.blocksize(), blocksize)
        self.assertTrue(not ws.isHistogramData())
        self.assertTrue(not ws.isDistribution())


if __name__ == "__main__":
    unittest.main()
