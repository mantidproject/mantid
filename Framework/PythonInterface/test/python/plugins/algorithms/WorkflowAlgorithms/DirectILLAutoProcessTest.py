# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import config, mtd, DirectILLAutoProcess


class DirectILLAutoProcessTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]

        config.appendDataSearchSubDir("ILL/PANTHER/")
        config["default.facility"] = "ILL"
        config["default.instrument"] = "PANTHER"

    def tearDown(self):
        if self._facility:
            config["default.facility"] = self._facility
        if self._instrument:
            config["default.instrument"] = self._instrument
        mtd.clear()

    def test_empty(self):
        empty_runs = "9777"
        empty_name = "MTCell19meV"
        ei = 19
        elc = 104
        DirectILLAutoProcess(
            Runs=empty_runs,
            OutputWorkspace=empty_name,
            ProcessAs="Empty",
            ReductionType="Powder",
            IncidentEnergy=ei,
            IncidentEnergyCalibration="Energy Calibration ON",
            ElasticChannelIndex=elc,
            SaveOutput=False,
            ClearCache=True,
        )
        self._check_output(mtd[empty_name][0], 512, 73728, False, "Time-of-flight", "TOF", "Spectrum", "Label")

    def test_vanadium(self):
        vanadium_runs = "9406"
        vanadium_name = "V19meV"
        ei = 19
        DirectILLAutoProcess(
            Runs=vanadium_runs,
            OutputWorkspace=vanadium_name,
            ProcessAs="Vanadium",
            ReductionType="Powder",
            IncidentEnergy=ei,
            IncidentEnergyCalibration="Energy Calibration ON",
            SaveOutput=False,
            ClearCache=True,
        )
        self.assertTrue(isinstance(mtd[vanadium_name], WorkspaceGroup))
        self._check_output(mtd[vanadium_name][0], 664, 226, True, "q", "MomentumTransfer", "Energy transfer", "DeltaE")  # S(Q, w)
        self._check_output(
            mtd[vanadium_name][1], 226, 669, True, "Energy transfer", "DeltaE", "Scattering angle", "Degrees"
        )  # S(2theta, w)

    def test_sample_single_crystal(self):
        sample_runs = "13806"
        sample_name = "panther_sx"
        ei = 19.03
        elp = 104
        eBins = [-2.0, 0.19, 16]
        DirectILLAutoProcess(
            Runs=sample_runs,
            OutputWorkspace=sample_name,
            ProcessAs="Sample",
            ReductionType="SingleCrystal",
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=ei,
            ElasticChannelIndex=elp,
            EnergyRebinningParams=eBins,
            GroupDetBy=2,
            MaskThresholdMin=0,
            MaskThresholdMax=20,
            ClearCache=True,
            SaveOutput=False,
        )
        self._check_output(mtd[sample_name][0], 95, 36863, False, "Energy transfer", "DeltaE", "Spectrum", "Label")

    def test_sample_powder(self):
        sample_runs = "9738"
        sample_name = "panther_powder"
        ei = 19
        elp = 104
        DirectILLAutoProcess(
            Runs=sample_runs,
            OutputWorkspace=sample_name,
            ProcessAs="Sample",
            ReductionType="Powder",
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=ei,
            ElasticChannelIndex=elp,
            MaskThresholdMin=0,
            MaskThresholdMax=20,
            ClearCache=True,
            SaveOutput=False,
        )
        self._check_output(mtd[sample_name][0], 665, 226, True, "q", "MomentumTransfer", "Energy transfer", "DeltaE")  # S(Q, w)
        self._check_output(mtd[sample_name][1], 226, 669, True, "Energy transfer", "DeltaE", "Scattering angle", "Degrees")  # S(2theta, w)

    def _check_output(self, ws, blocksize, spectra, isDistribution, x_unit, x_unit_id, y_unit, y_unit_id):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.isHistogramData())
        self.assertEqual(ws.isDistribution(), isDistribution)
        self.assertEqual(ws.getAxis(0).getUnit().caption(), x_unit)
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), x_unit_id)
        self.assertEqual(ws.getAxis(1).getUnit().caption(), y_unit)
        self.assertEqual(ws.getAxis(1).getUnit().unitID(), y_unit_id)
        self.assertTrue(ws.getHistory())


if __name__ == "__main__":
    unittest.main()
