# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import mtd
from mantid.simpleapi import config, LagrangeILLReduction
import unittest


class LagrangeILLReductionTest(unittest.TestCase):
    _facility = None
    _data_search_dirs = None

    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config.appendDataSearchSubDir("ILL/LAGRANGE/")
        config.setFacility("ILL")

    @classmethod
    def tearDownClass(cls):
        config.setFacility(cls._facility)
        config.setDataSearchDirs(cls._data_search_dirs)
        mtd.clear()

    def test_only_one_run(self):
        result = LagrangeILLReduction(SampleRuns="012869", NexusInput=False)
        self.check_result(result, "Energy", 150, 21.4992, 96.019)

        self.assertEqual(result.getRun().getLogData("run_title").value, "empty-cell-12mm-inner cu220")
        self.assertAlmostEqual(result.readY(0)[10], 0.014433, 4)
        self.assertAlmostEqual(result.readY(0)[80], 0.010947, 4)

    def test_multiple_runs(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", NexusInput=False)

        self.check_result(result, "Energy", 276, 21.4992, 446.5527)

        self.assertAlmostEqual(result.readY(0)[10], 0.014433, 4)
        self.assertAlmostEqual(result.readY(0)[80], 0.010947, 4)

    def test_water_correction(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", ContainerRuns="012882:012884", NexusInput=False)
        self.check_result(result, "Energy", 276, 21.4992, 446.5527)

        self.assertAlmostEqual(result.readY(0)[10], -0.017195, 4)
        self.assertAlmostEqual(result.readY(0)[80], -0.015001, 4)

    def test_calibration_correction(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", CorrectionFile="correction-water-cu220-2020.txt", NexusInput=False)
        self.check_result(result, "Energy", 276, 21.4992, 446.5527)

        self.assertAlmostEqual(result.readY(0)[10], 0.01478, 4)
        self.assertAlmostEqual(result.readY(0)[80], 0.01176, 4)

    def test_all_corrections(self):
        result = LagrangeILLReduction(
            SampleRuns="012869:012871", ContainerRuns="012882:012884", CorrectionFile="correction-water-cu220-2020.txt", NexusInput=False
        )
        self.check_result(result, "Energy", 276, 21.4992, 446.5527)

        self.assertAlmostEqual(result.readY(0)[10], -0.01761, 4)
        self.assertAlmostEqual(result.readY(0)[80], -0.01612, 4)

    def test_incident_energy(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", UseIncidentEnergy=True, NexusInput=False)
        self.check_result(result, "Energy", 276, 25.9992, 451.0527)

        self.assertAlmostEqual(result.readY(0)[10], 0.014433, 4)
        self.assertAlmostEqual(result.readY(0)[80], 0.010947, 4)

    def test_convert_to_wavenumber(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", ConvertToWaveNumber=True, NexusInput=False)
        self.check_result(result, "Energy_inWavenumber", 276, 173.4028, 3601.6907)

        self.assertAlmostEqual(result.readY(0)[10], 0.014433, 4)
        self.assertAlmostEqual(result.readY(0)[80], 0.010947, 4)

    def test_no_normalisation(self):
        result = LagrangeILLReduction(SampleRuns="012869:012871", NormaliseBy="None", NexusInput=False)
        self.check_result(result, "Energy", 276, 21.4992, 446.5527)
        self.assertAlmostEqual(result.readY(0)[10], 2165, 4)
        self.assertAlmostEqual(result.readY(0)[80], 1642, 4)

    def test_merging_close_initial_energies(self):
        result = LagrangeILLReduction(SampleRuns="012869_close_scans", NormaliseBy="None", NexusInput=False)
        self.check_result(result, "Energy", 148, 21.4992, 96.019)
        self.assertAlmostEqual(result.readY(0)[10], 1972, 4)
        self.assertAlmostEqual(result.readY(0)[80], 1737, 4)

    def test_one_scan_point(self):
        result = LagrangeILLReduction(SampleRuns="014220", NormaliseBy="Monitor", UseIncidentEnergy=True, NexusInput=False)
        self.check_result(result, "Energy", 1, 4.4998, 4.4998)
        self.assertAlmostEqual(result.readY(0)[0], 2.096, 3)
        self.assertEqual(result.getNumberHistograms(), 1)

    def test_nexus_sample_reduction(self):
        result = LagrangeILLReduction(SampleRuns="014412.nxs", NormaliseBy="None", UseIncidentEnergy=True)
        self.check_result(result, "Energy", 31, 35, 50)
        self.assertAlmostEqual(result.readY(0)[10], 3.0, 3)
        self.assertAlmostEqual(result.readY(0)[30], 3.0, 3)

    def test_nexus_sample_reduction_monitor_normalisation(self):
        result = LagrangeILLReduction(SampleRuns="014412.nxs", NormaliseBy="Monitor", UseIncidentEnergy=True)
        self.check_result(result, "Energy", 31, 35, 50)
        self.assertAlmostEqual(result.readY(0)[10], 3.0, 3)
        self.assertAlmostEqual(result.readY(0)[30], 3.0, 3)

    def test_merging_close_scans_nexus(self):
        result = LagrangeILLReduction(SampleRuns="014412_close_scans_sample.nxs", NormaliseBy="None", UseIncidentEnergy=True)
        self.check_result(result, "Energy", 30, 35, 50)
        self.assertAlmostEqual(result.readY(0)[10], 120.0, 3)
        self.assertAlmostEqual(result.readY(0)[29], 310.0, 3)

    def test_merging_close_scans_nexus_with_container(self):
        result = LagrangeILLReduction(
            SampleRuns="014412_close_scans_sample.nxs", ContainerRuns="014412_close_scans.nxs", NormaliseBy="None", UseIncidentEnergy=True
        )
        self.check_result(result, "Energy", 30, 35, 50)
        self.assertAlmostEqual(result.readY(0)[10], (120 - 12), 3)
        self.assertAlmostEqual(result.readY(0)[29], (310 - 31), 3)

    def test_nexus_with_offset(self):
        result = LagrangeILLReduction(SampleRuns="014412.nxs", NormaliseBy="None", UseIncidentEnergy=False)
        self.check_result(result, "Energy", 31, (35 - 4.5), (50 - 4.5))  # the offset of 4.5 is applied to data
        self.assertAlmostEqual(result.readY(0)[10], 3.0, 3)
        self.assertAlmostEqual(result.readY(0)[29], 3.0, 3)

    def test_nexus_convert_to_wavenumber(self):
        result = LagrangeILLReduction(SampleRuns="014412.nxs", ConvertToWaveNumber=True)
        self.check_result(result, "Energy_inWavenumber", 31, 245.9991, 366.9823)

        self.assertAlmostEqual(result.readY(0)[10], 3.0, 4)
        self.assertAlmostEqual(result.readY(0)[30], 3.0, 4)

    def test_nexus_multiple_files(self):
        result = LagrangeILLReduction(SampleRuns="014412.nxs, 014412_scan_continues.nxs")
        self.check_result(result, "Energy", 62, 30.5, 61.5)

        self.assertAlmostEqual(result.readY(0)[0], 3.0, 4)
        self.assertAlmostEqual(result.readY(0)[30], 3.0, 4)
        self.assertAlmostEqual(result.readY(0)[32], 2.0, 4)
        self.assertAlmostEqual(result.readY(0)[60], 30.0, 4)

    def check_result(self, ws, expected_unit, expected_bins, first_bin, last_bin):
        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.blocksize(), expected_bins)
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), expected_unit)
        self.assertAlmostEqual(ws.getAxis(0).extractValues()[0], first_bin, 4)
        self.assertAlmostEqual(ws.getAxis(0).extractValues()[-1], last_bin, 4)


if __name__ == "__main__":
    unittest.main()
