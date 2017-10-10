from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import BASISDiffraction

class OrientedSampleTest(stresstesting.MantidStressTest):
    """
    Run a reduction for a scan of runs probing different orientations
    of a crystal.
    """

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        BASISDiffraction(SampleOrientation=True,
                         RunNumbers='74799-74804',
                         VanadiumRuns='64642',
                         BackgroundRun='75527',
                         PsiAngleLog='SE50Rot',
                         PsiOffset=-27.0,
                         LatticeSizes=[10.71, 10.71, 10.71],
                         LatticeAngles=[90.0, 90.0, 90.0],
                         VectorU=[1, 1, 0],
                         VectorV=[0, 0, 1],
                         Uproj=[1, 1, 0],
                         Vproj=[0, 0, 1],
                         Wproj=[1, -1, 0],
                         Nbins=300,
                         OutputWorkspace='peaky')

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """

        self.tolerance = 0.1
        return 'peaky', 'BASISOrientedSample.nxs'