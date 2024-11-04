# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods
import os
import systemtesting
from mantid.kernel import config
from mantid.simpleapi import (
    CompareWorkspaces,
    CreateSimulationWorkspace,
    FilterPeaks,
    HB3AAdjustSampleNorm,
    HFIRCalculateGoniometer,
    IntegratePeaksMD,
    LoadNexus,
    PredictPeaks,
    SetGoniometer,
    SetUB,
)
from mantid.geometry import CrystalStructure, Goniometer


# The reference data for these tests were created with PredictPeaks in the state at Release 3.5,
# if PredictPeaks changes significantly, both reference data and test may need to be adjusted.


# The WISH test has a data mismatch which might be caused by the 'old' code having a bug (issue #14105).
# The difference is that peaks may have different d-values because they are assigned to a different detector.
# Instead of using the CompareWorkspaces, only H, K and L are compared.
class PredictPeaksTestWISH(systemtesting.MantidSystemTest):
    def runTest(self):
        simulationWorkspace = CreateSimulationWorkspace(Instrument="WISH", BinParams="0,1,2", UnitX="TOF")

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u="12,1,1", v="0,4,9")
        peaks = PredictPeaks(simulationWorkspace, WavelengthMin=0.5, WavelengthMax=6, MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus("predict_peaks_test_random_ub.nxs")

        hkls_predicted = self._get_hkls(peaks)
        hkls_reference = self._get_hkls(reference)

        lists_match, message = self._compare_hkl_lists(hkls_predicted, hkls_reference)

        self.assertEqual(lists_match, True, message)

    def _get_hkls(self, peaksWorkspace):
        h_list = peaksWorkspace.column("h")
        k_list = peaksWorkspace.column("k")
        l_list = peaksWorkspace.column("l")

        return [(x, y, z) for x, y, z in zip(h_list, k_list, l_list)]

    def _compare_hkl_lists(self, lhs, rhs):
        if len(lhs) != len(rhs):
            return False, "Lengths do not match: {} vs. {}".format(len(lhs), len(rhs))

        lhs_sorted = sorted(lhs)
        rhs_sorted = sorted(rhs)

        for i in range(len(lhs)):
            if lhs_sorted[i] != rhs_sorted[i]:
                return False, "Mismatch at position {}: {} vs. {}".format(i, lhs_sorted[i], rhs_sorted[i])

        return True, None


class PredictPeaksTestTOPAZ(systemtesting.MantidSystemTest):
    def runTest(self):
        direc = config["instrumentDefinition.directory"]
        xmlFile = os.path.join(direc, "TOPAZ_Definition_2015-01-01.xml")
        simulationWorkspace = CreateSimulationWorkspace(Instrument=xmlFile, BinParams="0,1,2", UnitX="TOF")

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u="12,1,1", v="0,4,9")
        peaks = PredictPeaks(simulationWorkspace, WavelengthMin=0.5, WavelengthMax=6, MinDSpacing=0.5, MaxDSpacing=10)

        reference = LoadNexus("predict_peaks_test_random_ub_topaz.nxs")

        simulationWorkspaceMatch = CompareWorkspaces(peaks, reference)

        self.assertTrue(simulationWorkspaceMatch[0])

        # compare using LeanElasticPeak
        leanelasticpeaks = PredictPeaks(
            simulationWorkspace, WavelengthMin=0.5, WavelengthMax=6, MinDSpacing=0.5, MaxDSpacing=10, OutputType="LeanElasticPeak"
        )
        self.assertEqual(peaks.getNumberPeaks(), leanelasticpeaks.getNumberPeaks())
        # sorting is different, just compare peak [1, 0, 2]
        peak102 = peaks.getPeak(117)
        leanelasticpeak102 = leanelasticpeaks.getPeak(7)
        self.assertDelta(peak102.getDSpacing(), leanelasticpeak102.getDSpacing(), 1e-9)
        self.assertDelta(peak102.getWavelength(), leanelasticpeak102.getWavelength(), 1e-9)
        self.assertDelta(peak102.getQSampleFrame()[0], leanelasticpeak102.getQSampleFrame()[0], 1e-9)
        self.assertDelta(peak102.getQSampleFrame()[1], leanelasticpeak102.getQSampleFrame()[1], 1e-9)
        self.assertDelta(peak102.getQSampleFrame()[2], leanelasticpeak102.getQSampleFrame()[2], 1e-9)


class PredictPeaksTestCORELLI(systemtesting.MantidSystemTest):
    def runTest(self):
        simulationWorkspace = CreateSimulationWorkspace(Instrument="CORELLI", BinParams="0,1,2", UnitX="TOF")

        SetUB(simulationWorkspace, a=10.9955, b=10.9955, c=14.31126, u="8.03319,7.49244,-0.625936", v="-0.23105,-0.457266,-14.2957")
        SetGoniometer(simulationWorkspace, Axis0="4.5,0,1,0,1")
        peaks = PredictPeaks(simulationWorkspace, WavelengthMin=0.9, WavelengthMax=1.1, MinDSpacing=13.5, MaxDSpacing=14.5)

        peak = peaks.getPeak(0)
        self.assertEqual(peak.getH(), 0)
        self.assertEqual(peak.getK(), 0)
        self.assertEqual(peak.getL(), 1)


class PredictPeaksCalculateStructureFactorsTest(systemtesting.MantidSystemTest):
    expected_num_peaks = 546

    def runTest(self):
        simulationWorkspace = CreateSimulationWorkspace(Instrument="WISH", BinParams="0,1,2", UnitX="TOF")

        SetUB(simulationWorkspace, a=5.5, b=6.5, c=8.1, u="12,1,1", v="0,4,9")

        # Setting some random crystal structure. Correctness of structure factor calculations is ensured in the
        # test suite of StructureFactorCalculator and does not need to be tested here.
        simulationWorkspace.sample().setCrystalStructure(CrystalStructure("5.5 6.5 8.1", "P m m m", "Fe 0.121 0.234 0.899 1.0 0.01"))

        peaks = PredictPeaks(
            simulationWorkspace, WavelengthMin=0.5, WavelengthMax=6, MinDSpacing=0.5, MaxDSpacing=10, CalculateStructureFactors=True
        )

        self.assertEqual(peaks.getNumberPeaks(), self.expected_num_peaks)

        for i in range(self.expected_num_peaks):
            peak = peaks.getPeak(i)
            self.assertLessThan(0.0, peak.getIntensity())

        peaks_no_sf = PredictPeaks(
            simulationWorkspace, WavelengthMin=0.5, WavelengthMax=6, MinDSpacing=0.5, MaxDSpacing=10, CalculateStructureFactors=False
        )

        for i in range(self.expected_num_peaks):
            peak = peaks_no_sf.getPeak(i)
            self.assertEqual(0.0, peak.getIntensity())


class PredictPeaksTestDEMAND(systemtesting.MantidSystemTest):
    def runTest(self):
        HB3AAdjustSampleNorm(Filename="HB3A_exp0724_scan0183.nxs", OutputWorkspace="data")

        peaks = PredictPeaks(
            "data",
            ReflectionCondition="B-face centred",
            CalculateGoniometerForCW=True,
            Wavelength=1.008,
            InnerGoniometer=True,
            FlipX=True,
            MinAngle=-2,
            MaxAngle=90,
        )

        self.assertEqual(peaks.getNumberPeaks(), 57)
        peak0 = peaks.getPeak(0)
        self.assertDelta(peak0.getWavelength(), 1.008, 1e-5)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), -14)
        q_sample = peak0.getQSampleFrame()
        self.assertDelta(q_sample[0], 4.45402, 1e-5)
        self.assertDelta(q_sample[1], -0.419157, 1e-5)
        self.assertDelta(q_sample[2], 0.0906594, 1e-5)

        # test predicting with LeanElasticPeak, filter any peak with 0 intensity
        PredictPeaks(
            "data",
            ReflectionCondition="B-face centred",
            OutputType="LeanElasticPeak",
            CalculateWavelength=False,
            OutputWorkspace="leanelasticpeaks",
        )

        IntegratePeaksMD("data", "leanelasticpeaks", PeakRadius=0.1, OutputWorkspace="integrated_peaks")
        filtered_peaks = FilterPeaks("integrated_peaks", FilterVariable="Intensity", FilterValue=0, Operator=">")

        self.assertEqual(filtered_peaks.getNumberPeaks(), 66)
        peak0 = filtered_peaks.getPeak(0)
        self.assertDelta(peak0.getWavelength(), 0, 1e-5)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), -14)
        q_sample = peak0.getQSampleFrame()
        self.assertDelta(q_sample[0], 4.45541, 1e-5)
        self.assertDelta(q_sample[1], -0.420383, 1e-5)
        self.assertDelta(q_sample[2], 0.0913072, 1e-5)

        g = Goniometer()
        g.setR(peak0.getGoniometerMatrix())
        YZY = g.getEulerAngles("YZY")
        self.assertDelta(YZY[0], 0, 1e-7)
        self.assertDelta(YZY[1], 0, 1e-7)
        self.assertDelta(YZY[2], 0, 1e-7)
        self.assertDelta(peak0.getWavelength(), 0, 1e-9)

        HFIRCalculateGoniometer(filtered_peaks)

        peak0 = filtered_peaks.getPeak(0)
        g = Goniometer()
        g.setR(peak0.getGoniometerMatrix())
        YZY = g.getEulerAngles("YZY")
        self.assertDelta(YZY[0], -20.4997, 1e-7)
        self.assertDelta(YZY[1], 0.0003, 1e-7)
        self.assertDelta(YZY[2], 0.5340761720854811, 1e-7)
        self.assertDelta(peak0.getWavelength(), 1.008, 1e-9)
