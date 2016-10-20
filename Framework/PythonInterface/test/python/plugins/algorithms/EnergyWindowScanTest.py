from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import EnergyWindowScan
from mantid.api import mtd


class IndirectEnergyWindowScanTest(unittest.TestCase):
    def test_IRIS(self):
        EnergyWindowScan(InputFiles="26176, 26173", Instrument='IRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                         InelasticRange='0, 0.5', GroupingMethod='All')
        scan_ws = mtd['Scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.9737326)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.0528091)

    def test_OSIRIS(self):
        EnergyWindowScan(InputFiles="OSI100319, OSI100326", Instrument='OSIRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='963, 1004', ElasticRange='-1.0, 0.',
                         InelasticRange='0, 1.5', GroupingMethod='All')
        scan_ws = mtd['Scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.8559410)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 0.8224473)

    def test_MSDfit(self):
        EnergyWindowScan(InputFiles="26176", Instrument='IRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                         InelasticRange='0, 0.5', GroupingMethod='Individual', MSDFit=True)
        scan_ws = mtd['Scan_el_eq2_0_Workspace']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), -2.4719849)
        self.assertEqual(round(scan_ws.readY(0)[1], 7), -2.3400994)


if __name__ == '__main__':
    unittest.main()
