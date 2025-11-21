import systemtesting

import numpy as np

from mantid.simpleapi import AnalysisDataService, FindUBUsingIndexedPeaks, IndexPeaks, LoadIsawPeaks, mtd, ReorientUnitCell


class SingleCrystalPeaksIntHKLIntMNPSaveLoadTest(systemtesting.MantidSystemTest):
    def tearDown(self):
        AnalysisDataService.clear()

    def requiredFiles(self):
        return ["RFMBA2PbI4_Monoclinic_P_5sig.integrate"]

    def rotation_angle_of_U(self, workspace):
        oriented_lattice = mtd[str(workspace)].sample().getOrientedLattice()
        trace = oriented_lattice.getU().diagonal().sum()
        cos_theta = (trace - 1) / 2.0
        return round(np.degrees(np.arccos(cos_theta)))

    def runTest(self):
        # peaks workspace
        workspace_name = mtd.unique_hidden_name()
        LoadIsawPeaks(Filename="RFMBA2PbI4_Monoclinic_P_5sig.integrate", OutputWorkspace=workspace_name)
        FindUBUsingIndexedPeaks(PeaksWorkspace=workspace_name)
        IndexPeaks(PeaksWorkspace=workspace_name, CommonUBForAll=True)
        theta_before = self.rotation_angle_of_U(workspace_name)

        ReorientUnitCell(PeaksWorkspace=workspace_name, CrystalSystem="Monoclinic")
        IndexPeaks(PeaksWorkspace=workspace_name, CommonUBForAll=True)
        theta_after = self.rotation_angle_of_U(workspace_name)
        self.assertLessThan(theta_after, theta_before, msg="ReorientUnitCell didn't align the crystal")
