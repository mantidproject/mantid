from __future__ import (absolute_import, division, print_function)

from testhelpers import illhelpers, run_algorithm
from mantid.api import mtd
import numpy.testing
import unittest


class DirectILLSelfShieldingTest(unittest.TestCase):
    _TEST_WS = None
    _TEST_WS_NAME = '_testWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)

    def setUp(self):
        if DirectILLSelfShieldingTest._TEST_WS is None:
            bkgLevel = 0.0
            DirectILLSelfShieldingTest._TEST_WS = illhelpers.create_poor_mans_in5_workspace(bkgLevel,
                                                                                            illhelpers.default_test_detectors)
            tempMonitorWSName = 'monitors'
            kwargs = {
                'InputWorkspace': DirectILLSelfShieldingTest._TEST_WS,
                'DetectorWorkspace': DirectILLSelfShieldingTest._TEST_WS_NAME,
                'MonitorWorkspace': tempMonitorWSName
            }
            run_algorithm('ExtractMonitors', **kwargs)
            DirectILLSelfShieldingTest._TEST_WS = mtd[DirectILLSelfShieldingTest._TEST_WS_NAME]
            kwargs = {
                'Workspace': tempMonitorWSName
            }
            run_algorithm('DeleteWorkspace', **kwargs)
        mtd.addOrReplace(DirectILLSelfShieldingTest._TEST_WS_NAME, DirectILLSelfShieldingTest._TEST_WS)

    def tearDown(self):
        mtd.clear()

    def testOutputHasCommonBinningWithInput(self):
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
            'InputWorkspace': self._TEST_WS_NAME,
            'Geometry': geometry,
            'Material': material
        }
        run_algorithm('SetSample', **kwargs)
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
        self.assertEquals(outWS.getNumberHistograms(), inWS.getNumberHistograms())
        xs = outWS.extractX()
        originalXs = inWS.extractX()
        numpy.testing.assert_almost_equal(xs, originalXs[:, :])


if __name__ == '__main__':
    unittest.main()
