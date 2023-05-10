# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, Run
from mantid.simpleapi import SANSILLReduction, config, mtd
from mantid.geometry import Instrument

import numpy as np


class SANSILLReductionTest(unittest.TestCase):
    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        cls._data_search_dirs = config["datasearch.directories"]
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]

        config.appendDataSearchSubDir("ILL/D11/")
        config.appendDataSearchSubDir("ILL/D16/")
        config.appendDataSearchSubDir("ILL/D33/")
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"

    def tearDown(self):
        mtd.clear()

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._data_search_dirs

    def test_absorber(self):
        SANSILLReduction(Run="010462", ProcessAs="Absorber", OutputWorkspace="Cd", Version=1)
        self._check_output(mtd["Cd"], True, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["Cd"], "Absorber")

    def test_beam(self):
        SANSILLReduction(Run="010414", ProcessAs="Beam", OutputWorkspace="Db", FluxOutputWorkspace="Fl", Version=1)
        self._check_output(mtd["Db"], True, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["Db"], "Beam")
        run = mtd["Db"].getRun()
        self.assertAlmostEqual(run.getLogData("BeamCenterX").value, 0.0048, delta=1e-4)
        self.assertAlmostEqual(run.getLogData("BeamCenterY").value, -0.0027, delta=1e-4)
        self._check_output(mtd["Fl"], False, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["Fl"], "Beam")
        self.assertAlmostEqual(mtd["Fl"].readY(0)[0], 6628249, delta=1)
        self.assertAlmostEqual(mtd["Fl"].readE(0)[0], 8566, delta=1)

    def test_transmission(self):
        SANSILLReduction(Run="010414", ProcessAs="Beam", OutputWorkspace="Db", Version=1)
        SANSILLReduction(Run="010585", ProcessAs="Transmission", BeamInputWorkspace="Db", OutputWorkspace="Tr", Version=1)
        self.assertAlmostEqual(mtd["Tr"].readY(0)[0], 0.642, delta=1e-3)
        self.assertAlmostEqual(mtd["Tr"].readE(0)[0], 0.0019, delta=1e-4)
        self._check_process_flag(mtd["Tr"], "Transmission")

    def test_container(self):
        SANSILLReduction(Run="010460", ProcessAs="Container", OutputWorkspace="can", Version=1)
        self._check_output(mtd["can"], True, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["can"], "Container")

    def test_reference(self):
        SANSILLReduction(Run="010453", ProcessAs="Sample", SensitivityOutputWorkspace="sens", OutputWorkspace="water", Version=1)
        self._check_output(mtd["water"], True, 1, 128 * 128 + 2)
        self._check_output(mtd["sens"], False, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["water"], "Sample")
        self._check_process_flag(mtd["sens"], "Sensitivity")

    def test_sample(self):
        SANSILLReduction(Run="010569", ProcessAs="Sample", OutputWorkspace="sample", Version=1)
        self._check_output(mtd["sample"], True, 1, 128 * 128 + 2)
        self._check_process_flag(mtd["sample"], "Sample")

    def test_absorber_tof(self):
        # D33 VTOF
        # actually this is a container run, not an absorber, but is fine for this test
        SANSILLReduction(Run="093409", ProcessAs="Absorber", OutputWorkspace="absorber", Version=1)
        self._check_output(mtd["absorber"], True, 30, 256 * 256 + 2)
        self._check_process_flag(mtd["absorber"], "Absorber")

    def test_beam_tof(self):
        # D33 VTOF
        SANSILLReduction(Run="093406", ProcessAs="Beam", OutputWorkspace="beam", FluxOutputWorkspace="flux", Version=1)
        self._check_output(mtd["beam"], True, 30, 256 * 256 + 2)
        self._check_process_flag(mtd["beam"], "Beam")
        run = mtd["beam"].getRun()
        self.assertAlmostEqual(run.getLogData("BeamCenterX").value, 0.0025, delta=1e-4)
        self.assertAlmostEqual(run.getLogData("BeamCenterY").value, 0.0009, delta=1e-4)
        self._check_output(mtd["flux"], False, 30, 256 * 256 + 2)
        self._check_process_flag(mtd["flux"], "Beam")

    def test_transmission_tof(self):
        # D33 VTOF
        SANSILLReduction(Run="093406", ProcessAs="Beam", OutputWorkspace="beam", Version=1)
        SANSILLReduction(Run="093407", ProcessAs="Transmission", BeamInputWorkspace="beam", OutputWorkspace="ctr", Version=1)
        self._check_output(mtd["ctr"], False, 97, 1)

    def test_reference_tof(self):
        # D33 VTOF
        # this is actually a sample run, not water, but is fine for this test
        SANSILLReduction(Run="093410", ProcessAs="Sample", OutputWorkspace="ref", Version=1)
        self._check_output(mtd["ref"], True, 30, 256 * 256 + 2)
        self._check_process_flag(mtd["ref"], "Sample")

    def test_sample_tof(self):
        # D33 VTOF, Pluronic F127
        SANSILLReduction(Run="093410", ProcessAs="Sample", OutputWorkspace="sample", Version=1)
        self._check_output(mtd["sample"], True, 30, 256 * 256 + 2)
        self._check_process_flag(mtd["sample"], "Sample")

    def test_sample_thickness(self):
        SANSILLReduction(Run="010569", ProcessAs="Sample", SampleThickness=-1, OutputWorkspace="sample", Version=1)
        a = mtd["sample"].getHistory().lastAlgorithm()
        thickness = a.getProperty("SampleThickness").value
        self.assertEqual(thickness, 0.1)

    def test_finite_sensitivity(self):
        SANSILLReduction(Runs="022846", ProcessAs="Water", OutputSensitivityWorkspace="sens", OutputWorkspace="_", MaxThreshold=5)
        self._check_process_flag(mtd["sens"], "Water")
        for spec_no in range(mtd["sens"].getNumberHistograms()):
            self.assertFalse(np.isnan(mtd["sens"].readY(spec_no)))

    def _check_process_flag(self, ws, value):
        self.assertTrue(ws.getRun().getLogData("ProcessedAs").value, value)

    def _check_output(self, ws, logs, blocksize, spectra):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertTrue(not ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(isinstance(ws.getInstrument(), Instrument))
        self.assertTrue(isinstance(ws.getRun(), Run))
        self.assertTrue(ws.getHistory())
        if logs:
            self.assertTrue(ws.getRun().hasProperty("qmin"))
            self.assertTrue(ws.getRun().hasProperty("qmax"))
            self.assertTrue(ws.getRun().hasProperty("l2"))
            self.assertTrue(ws.getRun().hasProperty("collimation.actual_position"))


if __name__ == "__main__":
    unittest.main()
