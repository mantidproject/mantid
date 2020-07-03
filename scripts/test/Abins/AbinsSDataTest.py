# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
import logging
import abins


class AbinsSDataTest(unittest.TestCase):
    def setUp(self):
        self.default_threshold_value = abins.parameters.sampling['s_absolute_threshold']
        self.default_min_wavenumber = abins.parameters.sampling['min_wavenumber']
        self.default_max_wavenumber = abins.parameters.sampling['max_wavenumber']
        self.logger = logging.getLogger('abins-sdata-test')

    def tearDown(self):
        abins.parameters.sampling['s_absolute_threshold'] = self.default_threshold_value
        abins.parameters.sampling['min_wavenumber'] = self.default_min_wavenumber
        abins.parameters.sampling['max_wavenumber'] = self.default_max_wavenumber

    def test_s_data(self):
        abins.parameters.sampling['min_wavenumber'] = 100
        abins.parameters.sampling['max_wavenumber'] = 150

        frequencies = np.linspace(105, 145, 5)
        sample_data = {'atom_1': {'s':
                                  {'order_1': np.array([0., 0.001, 1., 1., 0., 0.,])}}}

        s_data = abins.SData(temperature=10, sample_form='Powder', bin_width=10,
                             data=sample_data, frequencies=frequencies)

        self.assertTrue(np.allclose(s_data.extract()['frequencies'],
                                    frequencies))

        self.assertTrue(np.allclose(s_data.extract()['atom_1']['s']['order_1'],
                                    sample_data['atom_1']['s']['order_1']))

        with self.assertRaises(AssertionError):
            with self.assertLogs(logger=self.logger, level='WARNING'):
                s_data.check_thresholds(logger=self.logger)

        abins.parameters.sampling['s_absolute_threshold'] = 0.5
        with self.assertLogs(logger=self.logger, level='WARNING'):
            s_data.check_thresholds(logger=self.logger)
