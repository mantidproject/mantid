# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import numpy as np
from mantid.simpleapi import *


class ConvertHFIRSCDtoMDETest(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
            return 4000

    def runTest(self):
        LoadMD('HB2C_WANDSCD_data.nxs', OutputWorkspace='ConvertHFIRSCDtoMDETest_data')

        ConvertHFIRSCDtoMDETest_Q=ConvertHFIRSCDtoMDE(InputWorkspace='ConvertHFIRSCDtoMDETest_data', Wavelength=1.488)

        self.assertEqual(ConvertHFIRSCDtoMDETest_Q.getNEvents(), 18022177)

        ConvertHFIRSCDtoMDETest_peaks=FindPeaksMD(InputWorkspace=ConvertHFIRSCDtoMDETest_Q, PeakDistanceThreshold=2.2,
                                                  CalculateGoniometerForCW=True, Wavelength=1.488)

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 14)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(0)
        np.testing.assert_allclose(peak.getQSampleFrame(), [0.09446778,0.001306865,2.180508], rtol=1e-05)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(13)
        np.testing.assert_allclose(peak.getQSampleFrame(), [6.754011,0.001306865,1.918834], rtol=1e-05)
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)


class ConvertHFIRSCDtoMDE_HB3A_Test(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
            return 1000

    def runTest(self):
        LoadMD('HB3A_data.nxs', OutputWorkspace='ConvertHFIRSCDtoMDE_HB3ATest_data')

        SetGoniometer('ConvertHFIRSCDtoMDE_HB3ATest_data',
                      Axis0='omega,0,1,0,-1',
                      Axis1='chi,0,0,1,-1',
                      Axis2='phi,0,1,0,-1')

        ConvertHFIRSCDtoMDETest_Q=ConvertHFIRSCDtoMDE(InputWorkspace='ConvertHFIRSCDtoMDE_HB3ATest_data', Wavelength=1.008)

        self.assertEqual(ConvertHFIRSCDtoMDETest_Q.getNEvents(), 9038)

        ConvertHFIRSCDtoMDETest_peaks=FindPeaksMD(InputWorkspace='ConvertHFIRSCDtoMDETest_Q',
                                                  PeakDistanceThreshold=0.25,
                                                  DensityThresholdFactor=20000,
                                                  CalculateGoniometerForCW=True,
                                                  Wavelength=1.008,
                                                  FlipX=True,
                                                  InnerGoniometer=False)

        IndexPeaks(ConvertHFIRSCDtoMDETest_peaks)

        self.assertEqual(ConvertHFIRSCDtoMDETest_peaks.getNumberPeaks(), 1)

        peak = ConvertHFIRSCDtoMDETest_peaks.getPeak(0)
        self.assertDelta(peak.getWavelength(), 1.008, 1e-7)
        np.testing.assert_allclose(peak.getQSampleFrame(), [-0.417683,  1.792265,  2.238072], rtol=1e-5)
        np.testing.assert_array_equal(peak.getHKL(), [0, 0, 6])

    def validate(self):
        results = 'ConvertHFIRSCDtoMDETest_Q'
        reference = 'ConvertHFIRSCDtoMDE_HB3A_Test.nxs'

        Load(Filename=reference,OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1",results)
        checker.setPropertyValue("Workspace2",reference)
        checker.setPropertyValue("Tolerance", "1e-5")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ",checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True
