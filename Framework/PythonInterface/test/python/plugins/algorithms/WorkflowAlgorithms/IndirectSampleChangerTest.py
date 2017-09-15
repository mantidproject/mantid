from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import IndirectSampleChanger
from mantid.api import mtd


class IndirectSampleChangerTest(unittest.TestCase):
    def test_sampleChanger(self):
        """
        Basic test for sample changer
        """
        IndirectSampleChanger(FirstRun=72462, LastRun=72465, NumberSamples=1, Instrument='IRIS', Analyser='graphite',
                              Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                              InelasticRange='0, 0.5', GroupingMethod='All')
        scan_ws = mtd['iris72462_to_72465_s0_scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 1.0519965)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.0452707)

    def test_multiple_samples(self):
        """
        Test for running with different material in the sample changer
        """
        IndirectSampleChanger(FirstRun=72462, LastRun=72465, NumberSamples=2, Instrument='IRIS', Analyser='graphite',
                              Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                              InelasticRange='0, 0.5', GroupingMethod='All')
        scan_ws = mtd['iris72462_to_72464_s0_scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 1.0519965)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.0487223)

        scan_ws = mtd['iris72463_to_72465_s1_scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 1.0452707)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.0537665)

    def test_msdFit(self):
        """
        Basic test for sample changer with MSDFit
        """
        IndirectSampleChanger(FirstRun=72462, LastRun=72465, NumberSamples=1, Instrument='IRIS', Analyser='graphite',
                              Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                              InelasticRange='0, 0.5', GroupingMethod='Individual', msdFit=True)
        scan_ws = mtd['iris72462_to_72465_s0_scan_eisf']
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 1.1014862)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 1.1648556)

        msd_ws = mtd['iris72462_to_72465_s0_scan_msd']
        self.assertEqual(round(scan_ws.readY(0)[1], 7), 1.1355038)
        self.assertEqual(round(scan_ws.readY(1)[2], 7), 1.1431749)


#----------------------------------------Failure cases-------------------------------------

    def test_run_numbers(self):
        """
        Test for run numbers in wrong order
        """
        self.assertRaises(RuntimeError, IndirectSampleChanger,
                          FirstRun = 72465, LastRun = 72462, NumberSamples=2, Instrument='IRIS',
                          Analyser='graphite',Reflection='002', SpectraRange='3, 50',
                          ElasticRange='-0.5, 0', InelasticRange='0, 0.5', GroupingMethod='All')

    def test_number_samples(self):
        """
        Test for more samples than runs
        """
        self.assertRaises(RuntimeError, IndirectSampleChanger,
                          FirstRun = 72462, LastRun = 72465, NumberSamples=5, Instrument='IRIS',
                          Analyser='graphite',Reflection='002', SpectraRange='3, 50',
                          ElasticRange='-0.5, 0', InelasticRange='0, 0.5', GroupingMethod='All')


if __name__ == '__main__':
    unittest.main()
