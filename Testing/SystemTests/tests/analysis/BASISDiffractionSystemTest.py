from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid import config
from mantid.simpleapi import BASISDiffraction


class OrientedSampleTest(stresstesting.MantidStressTest):
    """
    Run a reduction for a scan of runs probing different orientations
    of a crystal.
    """

    def __init__(self):
        super(OrientedSampleTest, self).__init__()
        self.config = None
        self.setUp()

    def setUp(self):
        self.config = {p: config[p] for p in ('default.facility',
                                              'default.instrument',
                                              'datasearch.directories')}
        config['default.facility'] = 'SNS'
        config['default.instrument'] = 'BASIS'
        config.appendDataSearchSubDir('BASIS/BASISDiffraction')

    def tearDown(self):
        config.update(self.config)

    def requiredFiles(self):
        return ['BASIS_Mask_default_diff.xml',
                'BSS_74799_event.nxs',
                'BSS_74800_event.nxs',
                'BSS_64642_event.nxs',
                'BSS_75527_event.nxs',
                'BASISOrientedSample.nxs']

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            BASISDiffraction(SingleCrystalDiffraction=True,
                             RunNumbers='74799-74800',
                             MaskFile='BASIS_Mask_default_diff.xml',
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
        finally:
            self.tearDown()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        return 'peaky', 'BASISOrientedSample.nxs'
