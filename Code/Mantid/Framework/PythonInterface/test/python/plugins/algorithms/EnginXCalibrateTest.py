import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnginXCalibrateTest(unittest.TestCase):

    def test_issues_with_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required
        ones.
        """

        # No Filename property (required)
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          File='foo', Bank=1)

        # Wrong filename
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          File='foo_is_not_there', Bank=2)

        # mispelled ExpectedPeaks
        tbl = CreateEmptyTableWorkspace(OutputWorkspace='test_table')
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          Filename='ENGINX00228061.nxs', DetectorPositions=tbl, Bank=2, Peaks='2')

        # mispelled DetectorPositions
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          Filename='ENGINX00228061.nxs', Detectors=tbl, Bank=2, Peaks='2')

        # There's no output workspace
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          File='foo', Bank=1, OutputWorkspace='nop')


    def test_fails_gracefully(self):
        """
        Checks a bad run.
        """

        # This should produce 'given peak center ... is outside of data range' warnings
        # and finally raise after a 'some peaks not found' error
        self.assertRaises(RuntimeError,
                          EnginXCalibrate,
                          Filename="ENGINX00228061.nxs", ExpectedPeaks=[0.2, 0.4], Bank=2)


    def test_runs_ok(self):
        """
        Checks normal operation.
        """

        difc, zero = EnginXCalibrate(Filename="ENGINX00228061.nxs",
                                     ExpectedPeaks=[1.6, 1.1, 1.8], Bank=2)
        self.assertAlmostEqual(difc, 369367.57492582797)
        self.assertAlmostEqual(zero, -223297.87349744083)



if __name__ == '__main__':
    unittest.main()
