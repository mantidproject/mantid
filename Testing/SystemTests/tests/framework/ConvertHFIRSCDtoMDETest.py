# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import platform
import systemtesting
import numpy as np
from mantid.api import AlgorithmManager
from mantid.simpleapi import ConvertHFIRSCDtoMDE, FindPeaksMD, HFIRCalculateGoniometer, IndexPeaks, Load, LoadMD, SaveMD, SetGoniometer


def _skip_test():
    """Helper function to determine if we run the test"""
    return "Linux" not in platform.platform()


class ConvertHFIRSCDtoMDETest(systemtesting.MantidSystemTest):
    def skipTests(self):
        return _skip_test()

    def requiredMemoryMB(self):
        return 4000

    def runTest(self):
        LoadMD("HB2C_WANDSCD_data.nxs", OutputWorkspace="ConvertHFIRSCDtoMDETest_data")
        SetGoniometer("ConvertHFIRSCDtoMDETest_data", Axis0="s1,0,1,0,1", Average=False)

        ConvertHFIRSCDtoMDETest_Q = ConvertHFIRSCDtoMDE(InputWorkspace="ConvertHFIRSCDtoMDETest_data", Wavelength=1.488)

        self.assertEqual(ConvertHFIRSCDtoMDETest_Q.getNEvents(), 18022177)

        ConvertHFIRSCDtoMDETest_peaks = FindPeaksMD(
            InputWorkspace=ConvertHFIRSCDtoMDETest_Q, PeakDistanceThreshold=2.2, CalculateGoniometerForCW=True, Wavelength=1.488
        )

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 14)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(0)
        np.testing.assert_allclose(peak.getQSampleFrame(), [0.09446778, 0.001306865, 2.180508], rtol=1e-05)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(13)
        np.testing.assert_allclose(peak.getQSampleFrame(), [6.754011, 0.003572579, 1.918834], rtol=1e-05)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        # new method using multiple goniometers, compare peak q_sample to old method
        ConvertHFIRSCDtoMDETest_peaks2 = FindPeaksMD(InputWorkspace=ConvertHFIRSCDtoMDETest_Q, PeakDistanceThreshold=2.2)

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks2.getNumberPeaks(), 14)

        for p in range(14):
            np.testing.assert_allclose(
                ConvertHFIRSCDtoMDETest_peaks2.getPeak(p).getQSampleFrame(),
                ConvertHFIRSCDtoMDETest_peaks.getPeak(p).getQSampleFrame(),
                atol=0.005,
                err_msg=f"mismatch for peak {p}",
            )

        # now try using LeanElasticPeak
        ConvertHFIRSCDtoMDETest_peaks3 = FindPeaksMD(
            InputWorkspace=ConvertHFIRSCDtoMDETest_Q, PeakDistanceThreshold=2.2, OutputType="LeanElasticPeak"
        )

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks3.getNumberPeaks(), 14)

        for p in range(14):
            np.testing.assert_allclose(
                ConvertHFIRSCDtoMDETest_peaks3.getPeak(p).getQSampleFrame(),
                ConvertHFIRSCDtoMDETest_peaks.getPeak(p).getQSampleFrame(),
                atol=0.005,
                err_msg=f"mismatch for peak {p}",
            )

        HFIRCalculateGoniometer(ConvertHFIRSCDtoMDETest_peaks3)
        peak0 = ConvertHFIRSCDtoMDETest_peaks3.getPeak(0)
        self.assertDelta(peak0.getWavelength(), 1.488, 1e-8)
        g = peak0.getGoniometerMatrix()
        self.assertDelta(np.rad2deg(np.arccos(g[0][0])), 77.5, 1e-2)

        ConvertHFIRSCDtoMDETest_Qlorentz = ConvertHFIRSCDtoMDE(
            InputWorkspace="ConvertHFIRSCDtoMDETest_data", LorentzCorrection=True, Wavelength=1.488
        )

        ConvertHFIRSCDtoMDETest_peaks = FindPeaksMD(
            InputWorkspace=ConvertHFIRSCDtoMDETest_Qlorentz, PeakDistanceThreshold=2.2, CalculateGoniometerForCW=True, Wavelength=1.488
        )

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 14)


class ConvertHFIRSCDtoMDE_HB3A_Test(systemtesting.MantidSystemTest):
    def skipTests(self):
        return _skip_test()

    def requiredMemoryMB(self):
        return 1000

    def runTest(self):
        LoadMD("HB3A_data.nxs", OutputWorkspace="ConvertHFIRSCDtoMDE_HB3ATest_data")

        SetGoniometer(
            "ConvertHFIRSCDtoMDE_HB3ATest_data", Axis0="omega,0,1,0,-1", Axis1="chi,0,0,1,-1", Axis2="phi,0,1,0,-1", Average=False
        )

        ConvertHFIRSCDtoMDETest_Q = ConvertHFIRSCDtoMDE(InputWorkspace="ConvertHFIRSCDtoMDE_HB3ATest_data", Wavelength=1.008)

        self.assertEqual(ConvertHFIRSCDtoMDETest_Q.getNEvents(), 9038)

        ConvertHFIRSCDtoMDETest_peaks = FindPeaksMD(
            InputWorkspace="ConvertHFIRSCDtoMDETest_Q",
            PeakDistanceThreshold=0.25,
            DensityThresholdFactor=20000,
            CalculateGoniometerForCW=True,
            Wavelength=1.008,
            FlipX=True,
            InnerGoniometer=False,
        )

        IndexPeaks(ConvertHFIRSCDtoMDETest_peaks)

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 1)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(0)
        self.assertDelta(peak.getWavelength(), 1.008, 1e-7)
        np.testing.assert_allclose(peak.getQSampleFrame(), [-0.417683, 1.792265, 2.238072], rtol=1e-5)
        np.testing.assert_array_equal(peak.getHKL(), [0, 0, 6])

        # new method using multiple goniometers, q_sample should be the same as above method
        ConvertHFIRSCDtoMDETest_peaks2 = FindPeaksMD(
            InputWorkspace="ConvertHFIRSCDtoMDETest_Q", PeakDistanceThreshold=0.25, DensityThresholdFactor=20000
        )

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks2.getNumberPeaks(), 1)
        np.testing.assert_allclose(ConvertHFIRSCDtoMDETest_peaks2.getPeak(0).getQSampleFrame(), [-0.417683, 1.792265, 2.238072], rtol=1e-3)

        ConvertHFIRSCDtoMDETest_Qlorentz = ConvertHFIRSCDtoMDE(
            InputWorkspace="ConvertHFIRSCDtoMDE_HB3ATest_data", LorentzCorrection=True, Wavelength=1.008
        )

        ConvertHFIRSCDtoMDETest_peaks = FindPeaksMD(
            InputWorkspace=ConvertHFIRSCDtoMDETest_Qlorentz,
            PeakDistanceThreshold=0.25,
            DensityThresholdFactor=20000,
            CalculateGoniometerForCW=True,
            Wavelength=1.008,
            FlipX=True,
            InnerGoniometer=False,
        )

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 1)

    def validate(self):
        results = "ConvertHFIRSCDtoMDETest_Q"
        reference = "ConvertHFIRSCDtoMDE_HB3A_Test.nxs"

        Load(Filename=reference, OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1", results)
        checker.setPropertyValue("Workspace2", reference)
        checker.setPropertyValue("Tolerance", "1e-5")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ", checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results, Filename=self.__class__.__name__ + "-mismatch.nxs")
            return False

        return True
