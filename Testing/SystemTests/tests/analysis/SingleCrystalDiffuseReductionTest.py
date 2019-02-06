# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import SingleCrystalDiffuseReduction, Load, AlgorithmManager, SaveMD


class SingleCrystalDiffuseTest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        return ["CORELLI_29782.nxs","CORELLI_29792.nxs",
                "SingleCrystalDiffuseReduction_SA.nxs",
                "SingleCrystalDiffuseReduction_Flux.nxs",
                "SingleCrystalDiffuseReduction_UB.mat"]

    def runTest(self):
        SingleCrystalDiffuseReduction(Filename='CORELLI_29782.nxs,CORELLI_29792.nxs',
                                      SolidAngle='SingleCrystalDiffuseReduction_SA.nxs',
                                      Flux='SingleCrystalDiffuseReduction_Flux.nxs',
                                      UBMatrix="SingleCrystalDiffuseReduction_UB.mat",
                                      OutputWorkspace='SCDR_output',
                                      SetGoniometer=True,
                                      Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                      QDimension0='1,1,0',
                                      QDimension1='1,-1,0',
                                      Dimension0Binning='-7.5375,0.075,7.5375',
                                      Dimension1Binning='-13.165625,0.13100125,13.165625',
                                      Dimension2Binning='-0.1,0.1',
                                      SymmetryOperations="P 31 2 1")

    def validate(self):
        results = 'SCDR_output'
        reference = 'SingleCrystalDiffuseReduction.nxs'

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
