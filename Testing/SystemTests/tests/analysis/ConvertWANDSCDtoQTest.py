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
