# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import EnergyWindowScan
from mantid.api import mtd


class EnergyWindowScanTest(unittest.TestCase):
    def test_IRIS(self):
        EnergyWindowScan(InputFiles="IRS26176.RAW, IRS26173.RAW", Instrument='IRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                         InelasticRange='0, 0.5', GroupingMethod='All')
        scan_ws = mtd['Scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.9737326)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.0528091)

    def test_OSIRIS(self):
        EnergyWindowScan(InputFiles="osi89757.raw", Instrument='OSIRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='963, 1004', ElasticRange='-1.0, 0',
                         InelasticRange='0, 1.5', GroupingMethod='All')

        self.assertEqual(round(mtd['Scan_el_eq1'].readY(0)[0], 7), 0)
        self.assertEqual(
            round(
                mtd['Scan_inel_eq2'].readY(0)[0], 7), -6.8454638)

    def test_MSDFit(self):
        EnergyWindowScan(InputFiles="IRS26176.RAW", Instrument='IRIS', Analyser='graphite',
                         Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                         InelasticRange='0, 0.5', GroupingMethod='Individual', MSDFit=True)
        scan_ws = mtd['Scan_msd_fit']
        self.assertEqual(round(scan_ws[0].readY(0)[0], 7), 0.0844171)
        self.assertEqual(round(scan_ws[0].readY(0)[1], 7), 0.0963181)


if __name__ == '__main__':
    unittest.main()
