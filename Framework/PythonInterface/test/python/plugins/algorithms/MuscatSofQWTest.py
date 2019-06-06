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

class MuscatSofQWTest(unittest.TestCase):

    def setUp(self):
        self._sample_ws = Load(Filename='irs26176_graphite002_red.nxs',
                               OutputWorkspace='__MuscatSofQWTest_sample')
        self._resolution_ws = Load(Filename='irs26173_graphite002_res.nxs',
                                   OutputWorkspace='__MuscatSofQWTest_resolution')
        self._param_ws = Load(Filename='irs26176_graphite002_conv_1LFixF_s0_to_9_Result.nxs',
                              OutputWorkspace='__MuscatSofQWTest_param')


    def tearDown(self):
        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(self._resolution_ws)
        DeleteWorkspace(self._param_ws)


    def test_happy_case(self):
        """
        A basic test to see that the algorithm executes correctly.
        """
        sqw_ws = MuscatSofQW(SampleWorkspace=self._sample_ws,
                             ResolutionWorkspace=self._resolution_ws,
                             ParameterWorkspace=self._param_ws,
                             OutputWorkspace='__MuscatSofQWTest_result')

        self.assertEqual(sqw_ws.getNumberHistograms(), self._sample_ws.getNumberHistograms())
        self.assertEqual(sqw_ws.getAxis(0).getUnit().unitID(), 'Energy')
        self.assertEqual(sqw_ws.getAxis(1).getUnit().unitID(), 'MomentumTransfer')

        x_data = sqw_ws.dataX(0)
        self.assertAlmostEqual(x_data[0], -0.5)
        self.assertAlmostEqual(x_data[-1], 0.5)
        self.assertAlmostEqual(x_data[len(x_data)//2], 0.0)

        self.assertEqual(sqw_ws.blocksize(), 200)


    def test_energy_max(self):
        """
        A basic test to see that the algorithm executes correctly.
        """
        sqw_ws = MuscatSofQW(SampleWorkspace=self._sample_ws,
                             ResolutionWorkspace=self._resolution_ws,
                             ParameterWorkspace=self._param_ws,
                             OutputWorkspace='__MuscatSofQWTest_result',
                             EnergyMax=1.0)

        self.assertEqual(sqw_ws.getNumberHistograms(), self._sample_ws.getNumberHistograms())
        self.assertEqual(sqw_ws.getAxis(0).getUnit().unitID(), 'Energy')
        self.assertEqual(sqw_ws.getAxis(1).getUnit().unitID(), 'MomentumTransfer')

        x_data = sqw_ws.dataX(0)
        self.assertAlmostEqual(x_data[0], -1.0)
        self.assertAlmostEqual(x_data[-1], 1.0)
        self.assertAlmostEqual(x_data[len(x_data)//2], 0.0)

        self.assertEqual(sqw_ws.blocksize(), 400)


    def test_energy_increment(self):
        """
        A basic test to see that the algorithm executes correctly.
        """
        sqw_ws = MuscatSofQW(SampleWorkspace=self._sample_ws,
                             ResolutionWorkspace=self._resolution_ws,
                             ParameterWorkspace=self._param_ws,
                             OutputWorkspace='__MuscatSofQWTest_result',
                             EnergyInc=0.1)

        self.assertEqual(sqw_ws.getNumberHistograms(), self._sample_ws.getNumberHistograms())
        self.assertEqual(sqw_ws.getAxis(0).getUnit().unitID(), 'Energy')
        self.assertEqual(sqw_ws.getAxis(1).getUnit().unitID(), 'MomentumTransfer')

        x_data = sqw_ws.dataX(0)
        self.assertAlmostEqual(x_data[0], -0.5)
        self.assertAlmostEqual(x_data[-1], 0.5)
        self.assertAlmostEqual(x_data[len(x_data)//2], 0.0)

        self.assertEqual(sqw_ws.blocksize(), 10)


if __name__ == '__main__':
    unittest.main()
