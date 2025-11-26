import systemtesting

import numpy as np

from mantid.simpleapi import AnalysisDataService, FindUBUsingIndexedPeaks, IndexPeaks, LoadIsawPeaks, mtd, ReorientUnitCell, SetUB


class SingleCrystalPeaksIntHKLIntMNPSaveLoadTest(systemtesting.MantidSystemTest):
    def tearDown(self):
        AnalysisDataService.clear()

    def requiredFiles(self):
        return ["RFMBA2PbI4_Monoclinic_P_5sig.integrate"]

    @staticmethod
    def rotation_angle_of_RU(workspace_name: str):
        R = np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
        oriented_lattice = mtd[workspace_name].sample().getOrientedLattice()
        trace = (R @ oriented_lattice.getU()).diagonal().sum()
        cos_theta = (trace - 1) / 2.0
        return round(np.degrees(np.arccos(cos_theta)))

    def runTest(self):
        # peaks workspace
        workspace_name = mtd.unique_hidden_name()
        LoadIsawPeaks(Filename="RFMBA2PbI4_Monoclinic_P_5sig.integrate", OutputWorkspace=workspace_name)
        FindUBUsingIndexedPeaks(PeaksWorkspace=workspace_name)
        ol = mtd[workspace_name].sample().getOrientedLattice()
        # U represents a 120 degree rotation around axis <1 -1 -1>
        U = np.array([[0, 0, -1], [-1, 0, 0], [0, 1, 0]], dtype=float)
        SetUB(Workspace=workspace_name, UB=(U @ ol.getB()).flatten().tolist())
        IndexPeaks(PeaksWorkspace=workspace_name, CommonUBForAll=True)
        self.assertEqual(self.rotation_angle_of_RU(workspace_name), 180)
        ReorientUnitCell(PeaksWorkspace=workspace_name, CrystalSystem="Monoclinic")
        self.assertEqual(self.rotation_angle_of_RU(workspace_name), 0)
