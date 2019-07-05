# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from testhelpers import illhelpers, run_algorithm
from mantid.api import mtd
import numpy.testing
import unittest


class DirectILLSelfShieldingTest(unittest.TestCase):
    _TEST_WS = None

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._TEST_WS_NAME = '_testWS_'

    def setUp(self):
        if DirectILLSelfShieldingTest._TEST_WS is None:
            bkgLevel = 0.0
            DirectILLSelfShieldingTest._TEST_WS = illhelpers.create_poor_mans_in5_workspace(bkgLevel,
                                                                                            illhelpers.default_test_detectors)
        tempMonitorWSName = 'monitors'
        kwargs = {
            'InputWorkspace': DirectILLSelfShieldingTest._TEST_WS,
            'DetectorWorkspace': self._TEST_WS_NAME,
            'MonitorWorkspace': tempMonitorWSName
        }
        run_algorithm('ExtractMonitors', **kwargs)
        kwargs = {
            'Workspace': tempMonitorWSName
        }
        run_algorithm('DeleteWorkspace', **kwargs)

    def tearDown(self):
        mtd.clear()

    def testExecSparseInstrument(self):
        self._setDefaultSample(self._TEST_WS_NAME)
        outWSName = 'correctionWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'SimulationInstrument': 'Sparse Instrument',
            'SparseInstrumentRows': 3,
            'SparseInstrumentColumns': 2,
            'NumberOfSimulatedWavelengths': 3,
            'rethrow': True
        }
        run_algorithm('DirectILLSelfShielding', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        inWS = mtd[self._TEST_WS_NAME]
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), inWS.getNumberHistograms())
        xs = outWS.extractX()
        originalXs = inWS.extractX()
        numpy.testing.assert_almost_equal(xs, originalXs[:, :])

    def testOutputHasCommonBinningWithInput(self):
        self._setDefaultSample(self._TEST_WS_NAME)
        outWSName = 'correctionWS'
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLSelfShielding', **kwargs)
        self.assertTrue(mtd.doesExist(outWSName))
        inWS = mtd[self._TEST_WS_NAME]
        outWS = mtd[outWSName]
        self.assertEqual(outWS.getNumberHistograms(), inWS.getNumberHistograms())
        xs = outWS.extractX()
        originalXs = inWS.extractX()
        numpy.testing.assert_almost_equal(xs, originalXs[:, :])

    def _setDefaultSample(self, wsName):
        geometry = {
            'Shape': 'Cylinder',
            'Height': 8.0,
            'Radius': 2.0,
            'Center': [0.0, 0.0, 0.0]
        }
        material = {
            'ChemicalFormula': 'V',
            'SampleNumberDensity': 0.1
        }
        kwargs = {
            'InputWorkspace': wsName,
            'Geometry': geometry,
            'Material': material
        }
        run_algorithm('SetSample', **kwargs)


if __name__ == '__main__':
    unittest.main()
