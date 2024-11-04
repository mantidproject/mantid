# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import numpy as np
from mantid.api import AlgorithmManager
from mantid.simpleapi import ConvertWANDSCDtoQ, FindPeaksMD, IndexPeaks, Load, LoadMD, SaveMD, SetGoniometer, SetUB


class ConvertWANDSCDtoQTest(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 9500

    def runTest(self):
        LoadMD("HB2C_WANDSCD_data.nxs", OutputWorkspace="ConvertWANDSCDtoQTest_data")
        SetGoniometer("ConvertWANDSCDtoQTest_data", Axis0="s1,0,1,0,1", Average=False)
        LoadMD("HB2C_WANDSCD_norm.nxs", OutputWorkspace="ConvertWANDSCDtoQTest_norm")
        ConvertWANDSCDtoQTest_Q = ConvertWANDSCDtoQ(
            InputWorkspace="ConvertWANDSCDtoQTest_data", NormalisationWorkspace="ConvertWANDSCDtoQTest_norm"
        )

        ConvertWANDSCDtoQTest_peaks = FindPeaksMD(
            InputWorkspace=ConvertWANDSCDtoQTest_Q, PeakDistanceThreshold=2, CalculateGoniometerForCW=True, Wavelength=1.488
        )

        self.assertEqual(ConvertWANDSCDtoQTest_peaks.getNumberPeaks(), 14)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(0)
        np.testing.assert_allclose(peak.getQSampleFrame(), [2.400721, 0.001306865, 4.320331], rtol=1e-5)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(13)
        np.testing.assert_allclose(peak.getQSampleFrame(), [6.560115, 0.001306865, -2.520578], rtol=1e-5)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        SetUB(
            "ConvertWANDSCDtoQTest_data",
            UB="-2.7315243158024499e-17,1.7706197424726486e-01,-9.2794248657701375e-03,"
            "1.773049645390071e-01,0.,0.,1.2303228382369809e-17,-9.2794248657701254e-03,-1.7706197424726489e-01",
        )

        ConvertWANDSCDtoQ(
            InputWorkspace="ConvertWANDSCDtoQTest_data",
            NormalisationWorkspace="ConvertWANDSCDtoQTest_norm",
            Frame="HKL",
            BinningDim0="-0.62,0.62,31",
            BinningDim1="-2.02,7.02,226",
            BinningDim2="-6.52,2.52,226",
            OutputWorkspace="ConvertWANDSCDtoQTest_HKL",
        )

    def validate(self):
        results = "ConvertWANDSCDtoQTest_HKL"
        reference = "ConvertWANDSCDtoQTest_HKL.nxs"

        Load(Filename=reference, OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1", results)
        checker.setPropertyValue("Workspace2", reference)
        checker.setPropertyValue("Tolerance", "1e-7")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ", checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results, Filename=self.__class__.__name__ + "-mismatch.nxs")
            return False

        return True


class ConvertWANDSCDtoQ_HB3A_Test(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 9500

    def runTest(self):
        LoadMD("HB3A_data.nxs", OutputWorkspace="ConvertWANDSCDtoQ_HB3ATest_data")

        SetGoniometer("ConvertWANDSCDtoQ_HB3ATest_data", Axis0="omega,0,1,0,-1", Axis1="chi,0,0,1,-1", Axis2="phi,0,1,0,-1", Average=False)

        ConvertWANDSCDtoQ(
            InputWorkspace="ConvertWANDSCDtoQ_HB3ATest_data",
            Wavelength=1.008,
            BinningDim0="-5.0125,5.0125,401",
            BinningDim1="-2.0125,3.0125,201",
            BinningDim2="-0.0125,5.0125,201",
            KeepTemporaryWorkspaces=True,
            OutputWorkspace="ConvertWANDSCDtoQTest_Q",
        )

        ConvertWANDSCDtoQTest_peaks = FindPeaksMD(
            InputWorkspace="ConvertWANDSCDtoQTest_Q_data",
            PeakDistanceThreshold=0.25,
            DensityThresholdFactor=20000,
            CalculateGoniometerForCW=True,
            Wavelength=1.008,
            FlipX=True,
            InnerGoniometer=False,
        )

        IndexPeaks(ConvertWANDSCDtoQTest_peaks)

        self.assertEqual(ConvertWANDSCDtoQTest_peaks.getNumberPeaks(), 1)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(0)
        self.assertDelta(peak.getWavelength(), 1.008, 1e-7)
        np.testing.assert_allclose(peak.getQSampleFrame(), [-0.425693, 1.6994, 2.30206], rtol=1e-5)
        np.testing.assert_array_equal(peak.getHKL(), [0, 0, 6])

        ConvertWANDSCDtoQTest_HKL = ConvertWANDSCDtoQ(
            InputWorkspace="ConvertWANDSCDtoQ_HB3ATest_data",
            Wavelength=1.008,
            Frame="HKL",
            BinningDim0="-1.01,1.01,101",
            BinningDim1="-1.01,1.01,101",
            BinningDim2="4.99,7.01,101",
            KeepTemporaryWorkspaces=True,
        )

        signal = ConvertWANDSCDtoQTest_HKL.getSignalArray()
        # peak should be roughly the center of the volume
        np.testing.assert_array_equal(np.unravel_index(np.nanargmax(signal), signal.shape), (50, 51, 53))


class ConvertWANDSCDtoQ_Rotate_Test(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 9500

    def runTest(self):
        angleOffset = 45
        # ---
        LoadMD("HB2C_WANDSCD_data.nxs", OutputWorkspace="ConvertWANDSCDtoQTest_data")
        SetGoniometer("ConvertWANDSCDtoQTest_data", Axis0="s1,0,1,0,1", Average=False)
        LoadMD("HB2C_WANDSCD_norm.nxs", OutputWorkspace="ConvertWANDSCDtoQTest_norm")

        Q = ConvertWANDSCDtoQ(InputWorkspace="ConvertWANDSCDtoQTest_data", NormalisationWorkspace="ConvertWANDSCDtoQTest_norm")
        Qrot = ConvertWANDSCDtoQ(
            InputWorkspace="ConvertWANDSCDtoQTest_data", NormalisationWorkspace="ConvertWANDSCDtoQTest_norm", S1Offset=angleOffset
        )

        tableQ = FindPeaksMD(InputWorkspace=Q, PeakDistanceThreshold=2, CalculateGoniometerForCW=True, Wavelength=1.488, MaxPeaks=10)

        tableQrot = FindPeaksMD(InputWorkspace=Qrot, PeakDistanceThreshold=2, CalculateGoniometerForCW=True, Wavelength=1.488, MaxPeaks=10)

        peak = tableQ.getPeak(0)
        # determine angle from origin (x,y) plane only
        angle = np.degrees(np.arctan2(peak.getQSampleFrame().Z(), peak.getQSampleFrame().X()))
        hypotenuse = np.hypot(peak.getQSampleFrame().Z(), peak.getQSampleFrame().X())

        print("Angle {} :: Hypotenuse :: {}".format(angle, hypotenuse))
        # add offset degrees to determine new possible location, recalc coords
        newX = np.cos(np.radians(angle + angleOffset)) * hypotenuse
        newZ = np.sin(np.radians(angle + angleOffset)) * hypotenuse

        print("X: {} :: New X {}".format(peak.getQSampleFrame().X(), newX))
        print("Z: {} :: New Z {}".format(peak.getQSampleFrame().Z(), newZ))

        # look for coords in rotated table
        peakStillExists = False
        for i in range(tableQrot.getNumberPeaks()):
            rotatedPeak = tableQrot.getPeak(i)
            print("X: {} vs {}".format(rotatedPeak.getQSampleFrame().X(), newX))
            print("Z: {} vs {}".format(rotatedPeak.getQSampleFrame().Z(), newZ))
            if np.isclose(rotatedPeak.getQSampleFrame().X(), newX, rtol=1e-2) and np.isclose(
                rotatedPeak.getQSampleFrame().Z(), newZ, rtol=1e-2
            ):
                peakStillExists = True

        self.assertTrue(peakStillExists)
