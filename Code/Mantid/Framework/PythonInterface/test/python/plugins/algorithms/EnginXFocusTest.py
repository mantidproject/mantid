import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnginXFocusTest(unittest.TestCase):

    def test_wrong_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing
        required ones.
        """

        # No Filename property
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          File='foo', Bank=1, OutputWorkspace='nop')

        # Wrong filename
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          File='foo_is_not_there', Bank=1, OutputWorkspace='nop')

        # mispelled bank
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          Filename='ENGINX00228061.nxs', bnk='2', OutputWorkspace='nop')

        # mispelled DetectorsPosition
        tbl = CreateEmptyTableWorkspace()
        self.assertRaises(RuntimeError,
                          EnginXFocus,
                          Filename='ENGINX00228061.nxs', Detectors=tbl, OutputWorkspace='nop')


    def _check_output_ok(self, ws, ws_name='', y_dim_max=1, yvalues=None):
        """
        Checks expected types, values, etc. of an output workspace from EnginXFocus.
        """

        self.assertTrue(isinstance(ws, MatrixWorkspace),
                        'Output workspace should be a matrix workspace.')
        self.assertEqual(ws.name(), ws_name)
        self.assertEqual(ws.getTitle(), 'yscan;y=250.210')
        self.assertEqual(ws.isHistogramData(), True)
        self.assertEqual(ws.isDistribution(), True)
        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.blocksize(), 98)
        self.assertEqual(ws.getNEvents(), 98)
        self.assertEqual(ws.getNumDims(), 2)
        self.assertEqual(ws.YUnit(), 'Counts')
        dimX = ws.getXDimension()
        self.assertAlmostEqual( dimX.getMaximum(), 36938.078125)
        self.assertEqual(dimX.getName(), 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = ws.getYDimension()
        self.assertEqual(dimY.getMaximum(), y_dim_max)
        self.assertEqual(dimY.getName(), 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')

        if None == yvalues:
            raise ValueError("No y-vals provided for test")
        xvals = [10861.958645540433, 12192.372902418168, 13522.787159295902,
                 14853.201416173637, 24166.101214317776, 34809.415269339654]
        for i, bin in enumerate([0, 5, 10, 15, 50, 90]):
            self.assertAlmostEqual( ws.readX(0)[bin], xvals[i])
            self.assertAlmostEqual( ws.readY(0)[bin], yvalues[i])


    def test_runs_ok(self):
        """
        Checks that output looks fine for normal operation conditions, (default) Bank=1
        """

        out_name = 'out'
        out = EnginXFocus(Filename='ENGINX00228061.nxs', Bank=1, OutputWorkspace=out_name)

        yvals = [0.0037582279159681957, 0.00751645583194, 0.0231963801368, 0.0720786940576,
                 0.0615909620868, 0.00987979301753]
        self._check_output_ok(ws=out, ws_name=out_name, y_dim_max=1, yvalues=yvals)


    def test_runs_ok_bank2(self):
        """
        Checks that output looks fine for normal operation conditions, Bank=2
        """

        out_name = 'out_bank2'
        out_bank2 = EnginXFocus(Filename="ENGINX00228061.nxs", Bank=2,
                                OutputWorkspace=out_name)

        yvals = [0, 0.0112746837479, 0.0394536605073, 0.0362013481777,
                 0.0728500403862, 0.000870882282987]
        self._check_output_ok(ws=out_bank2, ws_name=out_name, y_dim_max=1201,
                              yvalues=yvals)



if __name__ == '__main__':
    unittest.main()
