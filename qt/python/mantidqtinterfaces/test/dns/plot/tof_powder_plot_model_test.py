# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns.plot.tof_powder_plot_model import \
    DNSTofPowderPlotModel

from mantid.api import MatrixWorkspace  # pylint: disable=no-name-in-module
from mantid.simpleapi import (CreateSampleWorkspace, DeleteWorkspace,
                              GroupWorkspaces)


class DNSTofPowderPlotModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.model = DNSTofPowderPlotModel(None)
        # Create two workspaces

    def test___init__(self):
        self.assertIsInstance(self.model, DNSTofPowderPlotModel)
        self.assertIsInstance(self.model, DNSObsModel)

    def test_get_plot_workspace(self):
        data1_sqw = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        testv = self.model.get_plot_workspace()
        self.assertIsInstance(testv, MatrixWorkspace)
        testv = self.model.get_plot_workspace()
        self.assertIsInstance(testv, MatrixWorkspace)
        # Create two workspaces
        GroupWorkspaces([data1_sqw, ws2], OutputWorkspace='testv')
        testv = self.model.get_plot_workspace()
        self.assertIsInstance(testv, MatrixWorkspace)
        DeleteWorkspace(data1_sqw)
        testv = self.model.get_plot_workspace()
        self.assertFalse(testv)


if __name__ == '__main__':
    unittest.main()
