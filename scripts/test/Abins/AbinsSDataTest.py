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
from abins import SData


class AbinsSDataTest(unittest.TestCase):
    def setUp(self):
        self.default_threshold_value = abins.parameters.sampling['s_absolute_threshold']
        self.default_min_wavenumber = abins.parameters.sampling['min_wavenumber']
        self.default_max_wavenumber = abins.parameters.sampling['max_wavenumber']
        self.logger = logging.getLogger('abins-sdata-test')
        self.sample_data = {'atom_1': {'s':
                                       {'order_1': np.array([0., 0.001, 1., 1., 0., 0.,])}},
                            'atom_2': {'s':
                                       {'order_1': np.array([0., 1.001, 2., 0., 3., 0.,])}}}

        self.frequencies = np.linspace(105, 145, 5)
        self.bin_width = 10

    def tearDown(self):
        abins.parameters.sampling['s_absolute_threshold'] = self.default_threshold_value
        abins.parameters.sampling['min_wavenumber'] = self.default_min_wavenumber
        abins.parameters.sampling['max_wavenumber'] = self.default_max_wavenumber

    def test_s_data(self):
        abins.parameters.sampling['min_wavenumber'] = 100
        abins.parameters.sampling['max_wavenumber'] = 150

        s_data = SData(temperature=10, sample_form='Powder',
                       data=self.sample_data, frequencies=self.frequencies)

        self.assertTrue(np.allclose(s_data.extract()['frequencies'],
                                    self.frequencies))

        self.assertTrue(np.allclose(s_data.extract()['atom_1']['s']['order_1'],
                                    self.sample_data['atom_1']['s']['order_1']))
        self.assertTrue(np.allclose(s_data.extract()['atom_2']['s']['order_1'],
                                    self.sample_data['atom_2']['s']['order_1']))

        with self.assertRaises(AssertionError):
            with self.assertLogs(logger=self.logger, level='WARNING'):
                s_data.check_thresholds(logger=self.logger)

        abins.parameters.sampling['s_absolute_threshold'] = 0.5
        with self.assertLogs(logger=self.logger, level='WARNING'):
            s_data.check_thresholds(logger=self.logger)

    def test_s_data_indexing(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertTrue(np.allclose(s_data[1]['order_1'],
                                    self.sample_data['atom_2']['s']['order_1']))

        sliced_items = s_data[:]
        self.assertIsInstance(sliced_items, list)
        self.assertTrue(np.allclose(sliced_items[0]['order_1'],
                                    self.sample_data['atom_1']['s']['order_1']))
        self.assertTrue(np.allclose(sliced_items[1]['order_1'],
                                    self.sample_data['atom_2']['s']['order_1']))

    def test_sample_form(self):
        sample_form = 'Polycrystalline'

        s_data = SData(sample_form=sample_form,
                       data=self.sample_data, frequencies=self.frequencies)
        self.assertEqual(sample_form, s_data.get_sample_form())

        with self.assertRaises(ValueError):
            s_data.check_known_sample_form()

        # Check should pass for 'Powder'
        powder_data = SData(sample_form='Powder', data=self.sample_data,
                            frequencies=self.frequencies)
        powder_data.check_known_sample_form()

    def test_bin_width(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertAlmostEqual(self.bin_width, s_data.get_bin_width())

        # Nonlinear frequency sampling has no bin width
        irregular_frequencies = np.exp(self.frequencies)
        s_data_irregular_freq = SData(data=self.sample_data,
                                      frequencies=irregular_frequencies)
        self.assertIsNone(s_data_irregular_freq.get_bin_width())

        # Unordered frequencies are rejected at init
        shuffled_frequencies = np.concatenate([self.frequencies[4:],
                                               self.frequencies[:4]])
        with self.assertRaises(ValueError):
            SData(data=self.sample_data, frequencies=shuffled_frequencies)

    def test_s_data_temperature(self):
        # Good temperature should pass without issue
        good_temperature = 10.5
        s_data_good_temperature = SData(frequencies=self.frequencies,
                                        data=self.sample_data,
                                        temperature=good_temperature)
        s_data_good_temperature.check_finite_temperature()
        self.assertAlmostEqual(good_temperature,
                               s_data_good_temperature.get_temperature())

        # Wrong type should get a TypeError at init
        with self.assertRaises(TypeError):
            SData(frequencies=self.frequencies, data=self.sample_data, temperature="10")

        # Non-finite values should get a ValueError when explicitly checked
        for bad_temperature in (-20., 0):
            s_data_bad_temperature = SData(frequencies=self.frequencies,
                                           data=self.sample_data,
                                           temperature=bad_temperature)
            with self.assertRaises(ValueError):
                s_data_bad_temperature.check_finite_temperature()
