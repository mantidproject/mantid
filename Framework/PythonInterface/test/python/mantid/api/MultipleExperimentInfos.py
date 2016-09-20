from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.api import ExperimentInfo

class MultipleExperimentInfoTest(unittest.TestCase):

    def test_information_access(self):
        signal = 2.0
        ndims = 1
        expt_ws = WorkspaceCreationHelper.makeFakeMDHistoWorkspace(signal, ndims)
        expinfo = expt_ws.getExperimentInfo(0)
        self.assertTrue(isinstance(expinfo, ExperimentInfo))
        self.assertEquals(1, expt_ws.getNumExperimentInfo())

if __name__ == '__main__': unittest.main()
