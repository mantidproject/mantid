# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import mtd
from testhelpers import run_algorithm
from mantid.api import WorkspaceGroup, MatrixWorkspace
from mantid import config


class IndirectILLReductionFWS(unittest.TestCase):

    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    # EFWS+IFWS, two wing
    _run_two_wing_mixed = '170299:170304'

    # EFWS+IFWS, one wing
    _run_one_wing_mixed = '083072:083077'

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

    def test_two_wing(self):

        args = {'Run': self._run_two_wing_mixed,
                'OutputWorkspace': 'out'}

        alg_test = run_algorithm('IndirectILLReductionFWS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionFWS not executed")

        self._check_workspace_group(mtd['out_red'], 2, 18, 3)

        runs_log1 = mtd['out_red'].getItem(0).getRun().getLogData('ReducedRunsList').value

        runs_log2 = mtd['out_red'].getItem(1).getRun().getLogData('ReducedRunsList').value

        self.assertEquals(runs_log1,'170299,170301,170303',"Reduced runs list mismatch.")

        self.assertEquals(runs_log2,'170300,170302,170304',"Reduced runs list mismatch.")

    def test_one_wing(self):

        args = {'Run': self._run_one_wing_mixed,
                'OutputWorkspace': 'out'}

        alg_test = run_algorithm('IndirectILLReductionFWS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionFWS not executed")

        self._check_workspace_group(mtd['out_red'], 3, 18, 2)

    def _check_workspace_group(self, wsgroup, nentries, nspectra, nbins):

        self.assertTrue(isinstance(wsgroup, WorkspaceGroup),
                        "{0} should be a group workspace".format(wsgroup.getName()))

        self.assertEquals(wsgroup.getNumberOfEntries(), nentries,
                          "{0} should contain {1} workspaces".format(wsgroup.getName(), nentries))

        item = wsgroup.getItem(0)

        name = item.name()

        self.assertTrue(isinstance(item, MatrixWorkspace),
                        "{0} should be a matrix workspace".format(name))

        self.assertEquals(item.getNumberHistograms(), nspectra,
                          "{0} should contain {1} spectra".format(name, nspectra))

        self.assertEquals(item.blocksize(), nbins,
                          "{0} should contain {1} bins".format(name, nbins))

        self.assertTrue(item.getSampleDetails(),
                        "{0} should have sample logs".format(name))

        self.assertTrue(item.getHistory().lastAlgorithm(),
                        "{0} should have history".format(name))

if __name__ == "__main__":
    unittest.main()
