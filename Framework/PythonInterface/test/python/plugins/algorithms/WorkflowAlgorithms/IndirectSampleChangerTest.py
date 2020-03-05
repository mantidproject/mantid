# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import AnalysisDataService
from mantid.simpleapi import IndirectSampleChanger

import unittest


def exists_in_ads(workspace_name):
    return AnalysisDataService.doesExist(workspace_name)


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name) if exists_in_ads(workspace_name) else None


class IndirectSampleChangerTest(unittest.TestCase):

    def setUp(self):
        self._first_run = 72462
        self._last_run = 72465
        self._number_samples = 1
        self._instrument = 'IRIS'
        self._analyser = 'graphite'
        self._reflection = '002'
        self._spectra_range = '3,50'
        self._elastic_range = '-0.02,0.02'
        self._inelastic_range = '0.4,0.5'
        self._total_range = '-0.5,0.5'
        self._msd_fit = False
        self._width_fit = False

    def tearDown(self):
        AnalysisDataService.clear()

    def test_sampleChanger(self):
        """
        Basic test for sample changer
        """
        self._execute_IndirectSampleChanger()

        scan_ws = get_ads_workspace('iris72462_to_iris72465_scan_eisf')
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.795287)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 0.837118)

    def test_multiple_samples(self):
        """
        Test for running with different material in the sample changer
        """
        self._number_samples = 2

        self._execute_IndirectSampleChanger()

        scan_ws = get_ads_workspace('iris72462_to_iris72464_scan_eisf')
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.795287)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 0.8005723)

        scan_ws = get_ads_workspace('iris72463_to_iris72465_scan_eisf')
        self.assertEqual(round(scan_ws.readY(0)[0], 7), 0.837118)
        self.assertEqual(round(scan_ws.readY(1)[0], 7), 0.8447051)

    def test_msdFit(self):
        """
        Basic test for sample changer with MSDFit
        """
        self._msd_fit = True

        self._execute_IndirectSampleChanger()

        eisf_ws = get_ads_workspace('iris72462_to_iris72465_scan_eisf')
        self.assertEqual(round(eisf_ws.readY(0)[0], 7), 0.795287)
        self.assertEqual(round(eisf_ws.readY(1)[0], 7), 0.837118)

        msd_ws = get_ads_workspace('iris72462_to_iris72465_scan_msd')
        self.assertEqual(round(msd_ws.readY(0)[1], 7), 0.2456879)
        self.assertEqual(round(msd_ws.readY(1)[2], 7), 0.5845778)

    def test_that_IndirectSampleChanger_will_raise_an_error_when_given_run_numbers_in_the_wrong_order(self):
        self.assertRaises(RuntimeError, IndirectSampleChanger,
                          FirstRun=self._last_run, LastRun=self._first_run, NumberSamples=self._number_samples,
                          Instrument=self._instrument, Analyser=self._analyser, Reflection=self._reflection,
                          SpectraRange=self._spectra_range, ElasticRange=self._elastic_range,
                          InelasticRange=self._inelastic_range, TotalRange=self._total_range)

    def test_that_IndirectSampleChanger_will_raise_an_error_when_given_more_samples_tan_runs(self):
        self.assertRaises(RuntimeError, IndirectSampleChanger,
                          FirstRun=self._first_run, LastRun=self._last_run, NumberSamples=5, Instrument=self._instrument,
                          Analyser=self._analyser, Reflection=self._reflection, SpectraRange=self._spectra_range,
                          ElasticRange=self._elastic_range, InelasticRange=self._inelastic_range,
                          TotalRange=self._total_range)

    def _execute_IndirectSampleChanger(self):
        IndirectSampleChanger(FirstRun=self._first_run, LastRun=self._last_run, NumberSamples=self._number_samples,
                              Instrument=self._instrument, Analyser=self._analyser, Reflection=self._reflection,
                              SpectraRange=self._spectra_range, ElasticRange=self._elastic_range,
                              InelasticRange=self._inelastic_range, TotalRange=self._total_range, MSDFit=self._msd_fit,
                              WidthFit=self._width_fit)


if __name__ == '__main__':
    unittest.main()
