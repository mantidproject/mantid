# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class TransformToIqtTest(unittest.TestCase):


    def setUp(self):
        """
        Generate reference result param table.
        """

        CreateEmptyTableWorkspace(OutputWorkspace='__TransformToIqtTest_param')
        self._param_table = mtd['__TransformToIqtTest_param']

        self._param_table.addColumn('int', 'SampleInputBins')
        self._param_table.addColumn('float', 'BinReductionFactor')
        self._param_table.addColumn('int', 'SampleOutputBins')
        self._param_table.addColumn('float', 'EnergyMin')
        self._param_table.addColumn('float', 'EnergyMax')
        self._param_table.addColumn('float', 'EnergyWidth')
        self._param_table.addColumn('float', 'Resolution')
        self._param_table.addColumn('int', 'ResolutionBins')

        self._param_table.addRow([1725, 10.0, 172, -0.5, 0.5, 0.00581395, 0.01845, 6])


    def test_with_can_reduction(self):
        """
        Tests running using the container reduction as a resolution.
        """

        sample = Load('irs26176_graphite002_red')
        can = Load('irs26173_graphite002_red')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=can,
                                     BinReductionFactor=10)

        self.assertTrue(CompareWorkspaces(params, self._param_table, 1e-8)[0])


    def test_with_resolution_reduction(self):
        """
        Tests running using the instrument resolution workspace.
        """

        sample = Load('irs26176_graphite002_red')
        resolution = Load('irs26173_graphite002_res')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=resolution,
                                     BinReductionFactor=10)

        self.assertTrue(CompareWorkspaces(params, self._param_table, 1e-8)[0])


if __name__ == '__main__':
    unittest.main()
