import unittest
from mantid.simpleapi import *
from mantid import mtd


class SofQWMomentsScanTest(unittest.TestCase):
    def test_sqw_moments_scan(self):
        reduced, sqw = SofQWMomentsScan(InputFiles='OSIRIS100320', Instrument='OSIRIS', Analyser='graphite', Reflection='002',
                                        SpectraRange='963,1004',
                                        QRange='0,0.1,2', EnergyRange='-0.4,0.01,0.4')

        self.assertEqual(sqw.getNumberHistograms(), 20)
        self.assertEqual(ws.blocksize(), 80)

if __name__ == "__main__":
    unittest.main()
