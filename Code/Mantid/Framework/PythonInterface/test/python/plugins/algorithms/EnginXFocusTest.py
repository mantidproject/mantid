import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnginXFocusTest(unittest.TestCase):

    def test_wrong_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required 
        ones
        """

        # No Filename
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          File='foo',
                          Bank=1)

        # no bank
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          File='foo')


    def _check_output_ok(self, ws, ws_name='', y_dim_max=1):
        """
        Checks expected types, values, etc.
        """

        self.assertTrue(isinstance(ws, MatrixWorkspace),
                        'Output workspace should be a matrix workspace.')
        self.assertEqual(ws.name(), ws_name)
        self.assertEqual(ws.getTitle(), 'EFURS02 Creep')
        self.assertEqual(ws.isHistogramData(), True)
        self.assertEqual(ws.isDistribution(), True)
        self.assertEqual(ws.blocksize(), 10186)
        self.assertEqual(ws.getNEvents(), 10186)
        self.assertEqual(ws.getNumDims(), 2)
        self.assertEqual(ws.YUnit(), 'Counts')
        dimX = ws.getXDimension()
        self.assertTrue( (dimX.getMaximum() - 52907.19921875) < 1e-5)
        self.assertEqual(dimX.getName(), 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = ws.getYDimension()
        self.assertEqual(dimY.getMaximum(), y_dim_max)
        self.assertEqual(dimY.getName(), 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')


    def test_runs_ok(self):
        """
        Checks that output looks fine for normal operation conditions, (default) Bank=1
        """

        out = EnginXFocus(Filename="ENGINX00213855.nxs", Bank=1)
        self._check_output_ok(ws=out, ws_name='out', y_dim_max=1)


    def test_runs_ok_bank2(self):
        """
        Checks that output looks fine for normal operation conditions, Bank=2
        """

        out_bank2 = EnginXFocus(Filename="ENGINX00213855.nxs", Bank=2)
        self._check_output_ok(ws=out_bank2, ws_name='out_bank2', y_dim_max=1201)


if __name__ == '__main__':
    unittest.main()
