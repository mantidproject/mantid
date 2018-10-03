# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid import config
from testhelpers import run_algorithm


class IndirectILLReductionQENSTest(unittest.TestCase):

    _run_one_wing = '090661'

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

    def test_two_wing_multi(self):

        args = {'Run': self._runs_two_wing_multi,
                'OutputWorkspace': 'out'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_red'], 2, 18, 1024)

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_red'], 2, 18, 1024)

        args['SumRuns'] = True

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_red'], 1, 18, 1024)

    def test_one_wing(self):

        args = {'Run': self._run_one_wing,
                'OutputWorkspace': 'out'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed")

        self._check_workspace_group(mtd['out_red'], 1, 18, 1024)

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
