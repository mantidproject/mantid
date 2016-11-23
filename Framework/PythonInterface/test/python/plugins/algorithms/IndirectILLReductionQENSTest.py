from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid import config
from testhelpers import run_algorithm


class IndirectILLReductionQENSTest(unittest.TestCase):

    _run_one_wing = '090661'

    _run_two_wing = '136558'

    _runs_two_wing_multi = '136558,136559'

    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs

    def test_multifiles(self):

        args = {'Run': self._runs_two_wing_multi}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['red'], 2, 18, 1024)

        args['SumRuns'] = True

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['red'], 1, 18, 1024)

    def test_one_wing(self):

        args = {'Run': self._run_one_wing}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['red'], 1, 18, 1024)

    def _test_unmirror_0_1_2_3(self):

        args['Run'] = self._run_two_wing

        args['UnmirrorOption'] = 0

        both = IndirectILLReductionQENS(**self._args)

        args['UnmirrorOption'] = 1

        red = IndirectILLReductionQENS(**self._args)

        args['UnmirrorOption'] = 2

        left = IndirectILLReductionQENS(**self._args)

        args['UnmirrorOption'] = 3

        right = IndirectILLReductionQENS(**self._args)

        summed = Plus(left.getItem(0),right.getItem(0))

        result = CompareWorkspaces(summed,red.getItem(0))

        self._assertTrue(result[0],"Unmirror 1 should be the sum of 2 and 3")

        left_right = GroupWorkspaces([left.getItem(0).getName(), right.getItem(0).getName()])

        result = CompareWorkspaces(left_right,both)

        self._assertTrue(result[0],"Unmirror 0 should be the group of 2 and 3")

    def _test_unmirror_4_5(self):

        args['Run'] = '136558'

        args['UnmirrorOption'] = 4

        vana4 = IndirectILLReductionQENS(**self._args)

        args['AlignmentRun'] = '136558'

        args['UnmirrorOption'] = 5

        vana5 = IndirectILLReductionQENS(**self._args)

        result = CompareWorkspaces(vana4, vana5)

        self._assertTrue(result[0], "Unmirror 4 should be the same as 5 if "
                                    "the same run is also defined as alignment run")

    def _test_unmirror_6_7(self):

        args['Run'] = '136558'

        args['UnmirrorOption'] = 6

        vana6 = IndirectILLReductionQENS(**self._args)

        args['VanadiumRun'] = '136558'

        args['UnmirrorOption'] = 7

        vana7 = IndirectILLReductionQENS(**self._args)

        result = CompareWorkspaces(vana6,vana7)

        self._assertTrue(result[0], "Unmirror 6 should be the same as 7 if "
                                    "the same run is also defined as alignment run")

    def _check_workspace_group(self, wsgroup, nentries, nspectra, nbins):

        self.assertTrue(isinstance(wsgroup, WorkspaceGroup),
                        "{0} should be a group workspace".format(wsgroup.getName()))

        self.assertEquals(wsgroup.getNumberOfEntries(),nentries,
                          "{0} should contain {1} workspaces".format(wsgroup.getName(),nentries))

        item = wsgroup.getItem(0)

        name = item.getName()

        self.assertTrue(isinstance(item, MatrixWorkspace),
                        "{0} should be a matrix workspace".format(name))

        self.assertEqual(item.getAxis(0).getUnit().unitID(), "DeltaE",
                         "{0} should have DeltaE units in X-axis".format(name))

        self.assertEquals(item.getNumberHistograms(),nspectra,
                          "{0} should contain {1} spectra".format(name,nspectra))

        self.assertEquals(item.blocksize(), nbins,
                          "{0} should contain {1} bins".format(name, nbins))

        self.assertTrue(item.getSampleDetails(),
                        "{0} should have sample logs".format(name))

        self.assertTrue(item.getHistory().lastAlgorithm(),
                        "{0} should have history".format(name))

if __name__ == '__main__':
    unittest.main()
