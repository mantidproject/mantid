# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import ConvertMultipleRunsToSingleCrystalMD, Load, AlgorithmManager, SaveMD


class ConvertMultipleRunsToSingleCrystalMDQSampleTest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        return ["CORELLI_29782.nxs","CORELLI_29792.nxs"]

    def runTest(self):
        ConvertMultipleRunsToSingleCrystalMD(Filename='CORELLI_29782.nxs,CORELLI_29792.nxs',
                                             OutputWorkspace='CMRSCMD_output_QSample',
                                             SetGoniometer=True,
                                             Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                             MinValues=[-6,-0.05,0],
                                             MaxValues=[-2,0.05,4])

    def validate(self):
        results = 'CMRSCMD_output_QSample'
        reference = 'ConvertMultipleRunsToSingleCrystalMD_QSample.nxs'

        Load(Filename=reference,OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1",results)
        checker.setPropertyValue("Workspace2",reference)
        checker.setPropertyValue("Tolerance", "1e-5")
        checker.setPropertyValue("IgnoreBoxID", "1")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ",checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True


class ConvertMultipleRunsToSingleCrystalMDHKLTest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        return ["CORELLI_29782.nxs","CORELLI_29792.nxs",
                "SingleCrystalDiffuseReduction_UB.mat"]

    def runTest(self):
        ConvertMultipleRunsToSingleCrystalMD(Filename='CORELLI_29782.nxs,CORELLI_29792.nxs',
                                             UBMatrix="SingleCrystalDiffuseReduction_UB.mat",
                                             OutputWorkspace='CMRSCMD_output_HKL',
                                             SetGoniometer=True,
                                             Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                             MinValues=[-4,3,-0.05],
                                             MaxValues=[0,8,0.05],
                                             QFrame='HKL')

    def validate(self):
        results = 'CMRSCMD_output_HKL'
        reference = 'ConvertMultipleRunsToSingleCrystalMD_HKL.nxs'

        Load(Filename=reference,OutputWorkspace=reference)

        checker = AlgorithmManager.create("CompareMDWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1",results)
        checker.setPropertyValue("Workspace2",reference)
        checker.setPropertyValue("Tolerance", "1e-5")
        checker.setPropertyValue("IgnoreBoxID", "1")

        checker.execute()
        if checker.getPropertyValue("Equals") != "1":
            print(" Workspaces do not match, result: ",checker.getPropertyValue("Result"))
            print(self.__class__.__name__)
            SaveMD(InputWorkspace=results,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True
