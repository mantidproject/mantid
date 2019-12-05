# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import numpy as np
from mantid.simpleapi import *


class ConvertWANDSCDtoQTest(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
            return 8000

    def runTest(self):
        LoadMD('HB2C_WANDSCD_data.nxs', OutputWorkspace='ConvertWANDSCDtoQTest_data')
        LoadMD('HB2C_WANDSCD_norm.nxs', OutputWorkspace='ConvertWANDSCDtoQTest_norm')
        ConvertWANDSCDtoQTest_Q=ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQTest_data',
                                                  NormalisationWorkspace='ConvertWANDSCDtoQTest_norm')

        ConvertWANDSCDtoQTest_peaks=FindPeaksMD(InputWorkspace=ConvertWANDSCDtoQTest_Q, PeakDistanceThreshold=2,
                                                CalculateGoniometerForCW=True, Wavelength=1.488)

        self.assertEqual(ConvertWANDSCDtoQTest_peaks.getNumberPeaks(), 14)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(0)
        self.assertTrue(np.allclose(peak.getQSampleFrame(), [2.40072, 0.00357258, 4.32033]))
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(13)
        self.assertTrue(np.allclose(peak.getQSampleFrame(), [6.56011, 0.00357258, -2.52058]))
        self.assertDelta(peak.getWavelength(), 1.488, 1e-5)

        SetUB('ConvertWANDSCDtoQTest_data', UB="-2.7315243158024499e-17,1.7706197424726486e-01,-9.2794248657701375e-03,"
              "1.773049645390071e-01,0.,0.,1.2303228382369809e-17,-9.2794248657701254e-03,-1.7706197424726489e-01")

        ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQTest_data',
                          NormalisationWorkspace='ConvertWANDSCDtoQTest_norm',
                          Frame='HKL',
                          BinningDim0='-0.62,0.62,31',
                          BinningDim1='-2.02,7.02,226',
                          BinningDim2='-6.52,2.52,226',
                          OutputWorkspace='ConvertWANDSCDtoQTest_HKL')

    def validate(self):
        results = 'ConvertWANDSCDtoQTest_HKL'
        reference = 'ConvertWANDSCDtoQTest_HKL.nxs'

        Load(Filename=reference,OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1",results)
        checker.setPropertyValue("Workspace2",reference)
        checker.setPropertyValue("Tolerance", "1e-7")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ",checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True


class ConvertWANDSCDtoQ_HB3A_Test(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
            return 8000

    def runTest(self):
        LoadMD('HB3A_data.nxs', OutputWorkspace='ConvertWANDSCDtoQ_HB3ATest_data')

        SetGoniometer('ConvertWANDSCDtoQ_HB3ATest_data',
                      Axis0='omega,0,1,0,-1',
                      Axis1='chi,0,0,1,-1',
                      Axis2='phi,0,1,0,-1')

        ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQ_HB3ATest_data',
                          Wavelength=1.008,
                          BinningDim0='-5.0125,5.0125,401',
                          BinningDim1='-2.0125,3.0125,201',
                          BinningDim2='-0.0125,5.0125,201',
                          KeepTemporaryWorkspaces=True,
                          OutputWorkspace='ConvertWANDSCDtoQTest_Q')

        ConvertWANDSCDtoQTest_peaks=FindPeaksMD(InputWorkspace='ConvertWANDSCDtoQTest_Q_data',
                                                PeakDistanceThreshold=0.25,
                                                DensityThresholdFactor=20000,
                                                CalculateGoniometerForCW=True,
                                                Wavelength=1.008,
                                                FlipX=True,
                                                InnerGoniometer=False)

        IndexPeaks(ConvertWANDSCDtoQTest_peaks)

        self.assertEqual(ConvertWANDSCDtoQTest_peaks.getNumberPeaks(), 1)

        peak = ConvertWANDSCDtoQTest_peaks.getPeak(0)
        self.assertDelta(peak.getWavelength(), 1.008, 1e-7)
        np.testing.assert_allclose(peak.getQSampleFrame(), [-0.425693,1.6994,2.30206], rtol=1e-5)
        np.testing.assert_array_equal(peak.getHKL(), [0, 0, 6])

        ConvertWANDSCDtoQTest_HKL = ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQ_HB3ATest_data',
                                                      Wavelength=1.008,
                                                      Frame='HKL',
                                                      BinningDim0='-1.01,1.01,101',
                                                      BinningDim1='-1.01,1.01,101',
                                                      BinningDim2='4.99,7.01,101',
                                                      KeepTemporaryWorkspaces=True)

        signal = ConvertWANDSCDtoQTest_HKL.getSignalArray()
        # peak should be roughly the center of the volume
        np.testing.assert_array_equal(np.unravel_index(np.nanargmax(signal), signal.shape), (50, 51, 53))
