from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class IndirectEnergyWindowScanTest(unittest.TestCase):

    def test_IRIS(self):
        IndirectEnergyWindowScan(InputFiles = "26176, 26173", Instrument = 'IRIS', Analyser = 'graphite',
                                 Reflection = '002', SpectraRange = '3, 50', ElasticRange = '-0.5, 0',
                                 InelasticRange = '0, 0.5', GroupingMethod = 'All')

        self.assertAlmostEqual(mtd['scan_eisf'].readY(0)[0], 0.973732648876)
        self.assertAlmostEqual(mtd['scan_eisf'].readY(1)[0], 1.05280906121)

    def test_OSIRIS(self):
        IndirectEnergyWindowScan(InputFiles = "OSI100319, OSI100326", Instrument = 'OSIRIS', Analyser = 'graphite',
                                 Reflection = '002', SpectraRange = '963, 1004', ElasticRange = '-1.0, 0.',
                                 InelasticRange = '0, 1.5', GroupingMethod = 'All')

        self.assertAlmostEqual(mtd['scan_eisf'].readY(0)[0], 0.855940981098)
        self.assertAlmostEqual(mtd['scan_eisf'].readY(1)[0], 0.822447283944)


    def test_MSDfit(self):
        IndirectEnergyWindowScan(InputFiles = "26176, 26173", Instrument = 'IRIS', Analyser = 'graphite',
                                 Reflection = '002', SpectraRange = '3, 50', ElasticRange = '-0.5, 0',
                                 InelasticRange = '0, 0.5', GroupingMethod = 'Individual', MSDFit = True)

        self.assertAlmostEqual(mtd['scan_msd_A0'].readY(0)[0], -2.45452608749)
        self.assertAlmostEqual(mtd['scan_msd_A1'].readY(0)[1], 0.166928976452)

if __name__ == '__main__':
    unittest.main()
