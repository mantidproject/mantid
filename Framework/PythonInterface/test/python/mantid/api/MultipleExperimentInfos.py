# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.api import AnalysisDataService, ExperimentInfo
from mantid.simpleapi import CreateSampleWorkspace, CreateMDWorkspace


class MultipleExperimentInfoTest(unittest.TestCase):

    def tearDown(self):
        AnalysisDataService.clear()

    def test_information_access(self):
        signal = 2.0
        ndims = 1
        expt_ws = WorkspaceCreationHelper.makeFakeMDHistoWorkspace(signal, ndims)
        expinfo = expt_ws.getExperimentInfo(0)
        self.assertTrue(isinstance(expinfo, ExperimentInfo))
        self.assertEqual(1, expt_ws.getNumExperimentInfo())

    def test_add_experiment_info(self):
        ws1 = CreateSampleWorkspace()
        md = CreateMDWorkspace(Dimensions=1, Extents='-1,1', Names='A', Units='U')
        md.addExperimentInfo(ws1)

if __name__ == '__main__': unittest.main()
