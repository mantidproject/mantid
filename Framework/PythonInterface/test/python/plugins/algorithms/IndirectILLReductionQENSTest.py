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

        args = {'Run': self._runs_two_wing_multi,
                'OutputWorkspace': 'out_ws'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_ws'], 2, 18, 1024)

        args['SumRuns'] = True

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_ws'], 1, 18, 1024)

    def test_one_wing(self):

        args = {'Run': self._run_one_wing}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['red'], 1, 18, 1024)

    def test_unmirror_0_1_2_3(self):

        args = {'Run' : self._run_two_wing,
                'UnmirrorOption' : 0,
                'OutputWorkspace' : 'both'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 0")

        args['UnmirrorOption'] = 1

        args['OutputWorkspace'] = 'red'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 1")

        args['UnmirrorOption'] = 2

        args['OutputWorkspace'] = 'left'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 2")

        args['UnmirrorOption'] = 3

        args['OutputWorkspace'] = 'right'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 3")

        summed = Plus(mtd['left'].getItem(0),mtd['right'].getItem(0))

        Scale(InputWorkspace=summed,Factor=0.5,OutputWorkspace=summed)

        result = CompareWorkspaces(summed,mtd['red'].getItem(0))

        self.assertTrue(result[0],"Unmirror 1 should be the sum of 2 and 3")

        left_right = GroupWorkspaces([mtd['left'].getItem(0).getName(), mtd['right'].getItem(0).getName()])

        result = CompareWorkspaces(left_right,'both')

        self.assertTrue(result[0],"Unmirror 0 should be the group of 2 and 3")

    def test_unmirror_4_5(self):

        args = {'Run': self._run_two_wing,
                'UnmirrorOption': 4,
                'OutputWorkspace': 'vana4'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 4")

        args['AlignmentRun'] = self._run_two_wing

        args['UnmirrorOption'] = 5

        args['OutputWorkspace'] = 'vana5'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 5")

        result = CompareWorkspaces('vana4', 'vana5')

        self.assertTrue(result[0], "Unmirror 4 should be the same as 5 if "
                                    "the same run is also defined as alignment run")

    def test_unmirror_6_7(self):

        args = {'Run': self._run_two_wing,
                'UnmirrorOption': 6,
                'OutputWorkspace': 'vana6'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 6")

        args['AlignmentRun'] = self._run_two_wing

        args['UnmirrorOption'] = 7

        args['OutputWorkspace'] = 'vana7'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 7")

        result = CompareWorkspaces('vana6','vana7')

        self.assertTrue(result[0], "Unmirror 6 should be the same as 7 if "
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
