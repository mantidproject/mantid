import systemtesting

import numpy as np
from numpy.testing import assert_array_almost_equal

from mantid.simpleapi import AnalysisDataService, FindUBUsingIndexedPeaks, IndexPeaks, LoadIsawPeaks, mtd, ReorientUnitCell, SetUB


class ReorientUnitCellTest(systemtesting.MantidSystemTest):
    def tearDown(self):
        AnalysisDataService.clear()

    def requiredFiles(self):
        return ["TOPAZ_Monoclinic_P_5sig.integrate"]

    def runTest(self):
        # peaks workspace
        workspace_name = mtd.unique_hidden_name()
        LoadIsawPeaks(Filename="TOPAZ_Monoclinic_P_5sig.integrate", OutputWorkspace=workspace_name)
        FindUBUsingIndexedPeaks(PeaksWorkspace=workspace_name)
        ol = mtd[workspace_name].sample().getOrientedLattice()
        ## Initially, Z axis points along `-a*`, X-axis along along `b*`
        # U represents a 120 degree rotation around axis <1 1 -1>
        U = np.array([[0, 1, 0], [0, 0, -1], [-1, 0, 0]], dtype=float)
        SetUB(Workspace=workspace_name, UB=(U @ ol.getB()).flatten().tolist())
        u = np.array(mtd[workspace_name].sample().getOrientedLattice().getuVector())
        assert_array_almost_equal(u, np.array([-9.2, 0, 0]), decimal=1)
        IndexPeaks(PeaksWorkspace=workspace_name, CommonUBForAll=True)
        ReorientUnitCell(PeaksWorkspace=workspace_name, CrystalSystem="Monoclinic")
        # Now, Z axis points along `+a*`
        u = np.array(mtd[workspace_name].sample().getOrientedLattice().getuVector())
        assert_array_almost_equal(u, np.array([9.2, 0, 0]), decimal=1)
