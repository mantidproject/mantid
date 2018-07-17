from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace
from mantid.simpleapi import ILLSANSReduction


class ILLSANSReductionTest(unittest.TestCase):

    _facility = None

    def setUp(self):
        self._facility = config['default.facility']
        config['default.facility'] = 'ILL'

    def tearDown(self):
        config['default.facility'] = self._facility
        mtd.clear()

    def test_absorber(self):

        ILLSANSReduction(Run='010462', ProcessAs='Absorber', OutputWorkspace='Cd')
        self._check_output(mtd['Cd'])

    def _check_output(self, ws):

        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertEqual(ws.blocksize(), 1)
        self.assertEqual(ws.getNumberHistograms(), 128 * 128 + 2)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getSampleDetails())
        self.assertTrue(ws.getHistory())
        self.assertTrue(ws.getRun().hasProperty('qmin'))
        self.assertTrue(ws.getRun().hasProperty('qmax'))
        self.assertTrue(ws.getRun().hasProperty('l2'))
        self.assertTrue(ws.getRun().hasProperty('pixel_height'))
        self.assertTrue(ws.getRun().hasProperty('pixel_width'))

if __name__ == '__main__':
    unittest.main()
