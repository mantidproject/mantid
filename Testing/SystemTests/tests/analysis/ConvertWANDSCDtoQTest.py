import stresstesting
import numpy as np
from mantid.simpleapi import *


class ConvertWANDSCDtoQTest(stresstesting.MantidStressTest):
    def requiredMemoryMB(self):
            return 8000

    def runTest(self):
        LoadMD('HB2C_WANDSCD_data.nxs', OutputWorkspace='ConvertWANDSCDtoQTest_data')
        LoadMD('HB2C_WANDSCD_norm.nxs', OutputWorkspace='ConvertWANDSCDtoQTest_norm')
        ConvertWANDSCDtoQTest_Q=ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQTest_data',
                                                  NormalisationWorkspace='ConvertWANDSCDtoQTest_norm')

        ConvertWANDSCDtoQTest_peaks=FindPeaksMD(InputWorkspace=ConvertWANDSCDtoQTest_Q, PeakDistanceThreshold=2,
                                                CalculateGoniometerForCW=True, Wavelength=1.488)
        FindUBUsingLatticeParameters(ConvertWANDSCDtoQTest_peaks, a=5.64, b=5.64, c=5.64, alpha=90, beta=90, gamma=90)
        UB = ConvertWANDSCDtoQTest_peaks.sample().getOrientedLattice().getUB()
        self.assertTrue(np.allclose(UB, [[-2.73152432e-17,  1.77061974e-01, -9.27942487e-03],
                                         [ 1.77304965e-01,  0.00000000e+00,  0.00000000e+00],
                                         [ 1.23032284e-17, -9.27942487e-03, -1.77061974e-01]]))

        ConvertWANDSCDtoQ(InputWorkspace='ConvertWANDSCDtoQTest_data',
                          NormalisationWorkspace='ConvertWANDSCDtoQTest_norm',
                          UBWorkspace=ConvertWANDSCDtoQTest_peaks,
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
