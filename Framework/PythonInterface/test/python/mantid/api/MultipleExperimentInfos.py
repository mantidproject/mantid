# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
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
        self.assertEqual(1, expt_ws.getNumExperimentInfo())

if __name__ == '__main__': unittest.main()
