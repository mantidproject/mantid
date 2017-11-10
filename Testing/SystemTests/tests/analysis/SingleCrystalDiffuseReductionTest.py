import stresstesting
from mantid.simpleapi import SingleCrystalDiffuseReduction, Load, AlgorithmManager, SaveMD


class SingleCrystalDiffuseTest(stresstesting.MantidStressTest):
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
                                      Uproj='1,1,0',
                                      Vproj='1,-1,0',
                                      Wproj='0,0,1',
                                      BinningDim0='-7.5375,7.5375,201',
                                      BinningDim1='-13.165625,13.165625,201',
                                      BinningDim2='-0.1,0.1,1',
                                      SymmetryOps="P 31 2 1")

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
