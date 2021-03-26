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
from abins.sdata import SDataByAngle


class AbinsSDataTest(unittest.TestCase):
    def setUp(self):
        self.default_threshold_value = abins.parameters.sampling['s_absolute_threshold']
        self.default_min_wavenumber = abins.parameters.sampling['min_wavenumber']
        self.default_max_wavenumber = abins.parameters.sampling['max_wavenumber']
        self.logger = logging.getLogger('abins-sdata-test')
        self.sample_data = {
            'atom_0': {
                's': {
                    'order_1': np.array([0., 0.001, 1., 1., 0.])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([0., 1.001, 2., 0., 3.])
                }
            }
        }

        self.frequencies = np.linspace(105, 145, 5)
        self.bin_width = 10

    def tearDown(self):
        abins.parameters.sampling['s_absolute_threshold'] = self.default_threshold_value
        abins.parameters.sampling['min_wavenumber'] = self.default_min_wavenumber
        abins.parameters.sampling['max_wavenumber'] = self.default_max_wavenumber

    def test_s_data(self):
        abins.parameters.sampling['min_wavenumber'] = 100
        abins.parameters.sampling['max_wavenumber'] = 150

        s_data = SData(temperature=10, sample_form='Powder', data=self.sample_data, frequencies=self.frequencies)

        self.assertTrue(np.allclose(s_data.extract()['frequencies'], self.frequencies))

        self.assertTrue(
            np.allclose(s_data.extract()['atom_0']['s']['order_1'], self.sample_data['atom_0']['s']['order_1']))
        self.assertTrue(
            np.allclose(s_data.extract()['atom_1']['s']['order_1'], self.sample_data['atom_1']['s']['order_1']))

        with self.assertRaises(AssertionError):
            with self.assertLogs(logger=self.logger, level='WARNING'):
                s_data.check_thresholds(logger=self.logger)

        abins.parameters.sampling['s_absolute_threshold'] = 0.5
        with self.assertLogs(logger=self.logger, level='WARNING'):
            s_data.check_thresholds(logger=self.logger)

    def test_s_data_indexing(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertTrue(np.allclose(s_data[1]['order_1'], self.sample_data['atom_1']['s']['order_1']))

        sliced_items = s_data[:]
        self.assertIsInstance(sliced_items, list)
        self.assertTrue(np.allclose(sliced_items[0]['order_1'], self.sample_data['atom_0']['s']['order_1']))
        self.assertTrue(np.allclose(sliced_items[1]['order_1'], self.sample_data['atom_1']['s']['order_1']))

    def test_sample_form(self):
        sample_form = 'Polycrystalline'

        s_data = SData(sample_form=sample_form, data=self.sample_data, frequencies=self.frequencies)
        self.assertEqual(sample_form, s_data.get_sample_form())

        with self.assertRaises(ValueError):
            s_data.check_known_sample_form()

        # Check should pass for 'Powder'
        powder_data = SData(sample_form='Powder', data=self.sample_data, frequencies=self.frequencies)
        powder_data.check_known_sample_form()

    def test_bin_width(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertAlmostEqual(self.bin_width, s_data.get_bin_width())

        # Nonlinear frequency sampling has no bin width
        irregular_frequencies = np.exp(self.frequencies)
        s_data_irregular_freq = SData(data=self.sample_data, frequencies=irregular_frequencies)
        self.assertIsNone(s_data_irregular_freq.get_bin_width())

        # Unordered frequencies are rejected at init
        shuffled_frequencies = np.concatenate([self.frequencies[3:], self.frequencies[:3]])
        with self.assertRaises(ValueError):
            SData(data=self.sample_data, frequencies=shuffled_frequencies)

    def test_s_data_temperature(self):
        # Good temperature should pass without issue
        good_temperature = 10.5
        s_data_good_temperature = SData(frequencies=self.frequencies,
                                        data=self.sample_data,
                                        temperature=good_temperature)
        s_data_good_temperature.check_finite_temperature()
        self.assertAlmostEqual(good_temperature, s_data_good_temperature.get_temperature())

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


class AbinsSDataByAngleTest(unittest.TestCase):
    def setUp(self):
        self.list_data = [{
            'atom_0': {
                's': {
                    'order_1': np.array([
                        0.,
                        0.001,
                        1.,
                        1.,
                        0.,
                        0.,
                    ])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([
                        0.,
                        1.001,
                        2.,
                        0.,
                        3.,
                        0.,
                    ])
                }
            }
        }, {
            'atom_0': {
                's': {
                    'order_1': np.array([
                        2.,
                        2.001,
                        3.,
                        2.,
                        0.,
                        0.,
                    ])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([
                        2.,
                        3.001,
                        2.,
                        0.,
                        2.,
                        0.,
                    ])
                }
            }
        }]

        self.init_data = {
            'atom_0': {
                's': {
                    'order_1': np.array([[0., 0.001, 1., 1., 0., 0.], [2., 2.001, 3., 2., 0., 0.]])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([[
                        0.,
                        1.001,
                        2.,
                        0.,
                        3.,
                        0.,
                    ], [
                        2.,
                        3.001,
                        2.,
                        0.,
                        2.,
                        0.,
                    ]])
                }
            }
        }

        self.angles = [27., 45.]
        self.frequencies = np.linspace(105, 145, 6)
        self.sum_data = {
            'atom_0': {
                's': {
                    'order_1': np.array([2., 2.002, 4., 3., 0, 0.])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([2., 4.002, 4., 0., 5., 0.])
                }
            }
        }
        self.avg_data = {
            'atom_0': {
                's': {
                    'order_1': np.array([1., 1.001, 2., 1.5, 0, 0.])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([1., 2.001, 2., 0., 2.5, 0.])
                }
            }
        }
        self.weighted_data = {
            'atom_0': {
                's': {
                    'order_1': np.array([2., 2.003, 5., 4., 0, 0.])
                }
            },
            'atom_1': {
                's': {
                    'order_1': np.array([2., 5.003, 6., 0., 8., 0.])
                }
            }
        }

    def test_s_data_by_angle_set_angle_data(self):
        sdba = self.get_s_data_by_angle()
        sdba.set_angle_data(0, SData(data=self.list_data[1], frequencies=self.frequencies))
        self.assertTrue(
            np.allclose(sdba[0].extract()['atom_1']['s']['order_1'], self.list_data[1]['atom_1']['s']['order_1']))

        sdba.set_angle_data(0, SData(data=self.list_data[0], frequencies=self.frequencies), add_to_existing=True)
        self.assertTrue(
            np.allclose(sdba[0].extract()['atom_1']['s']['order_1'], self.sum_data['atom_1']['s']['order_1']))

    def get_s_data_by_angle(self, **kwargs):
        return SDataByAngle(data=self.init_data, angles=self.angles, frequencies=self.frequencies, **kwargs)

    def test_s_data_by_angle_indexing(self):
        temperature, sample_form = (100., 'powder')

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)

        for i in range(2):
            self.assertIsInstance(sdba[i], SData)
            self.assertTrue(np.allclose(sdba[i].get_frequencies(), self.frequencies))
            self.assertTrue(
                np.allclose(sdba[i].extract()['atom_1']['s']['order_1'], self.list_data[i]['atom_1']['s']['order_1']))

        with self.assertRaises(IndexError):
            sdba[2]

    def test_s_data_by_angle_slicing(self):
        temperature, sample_form = (101., 'plasma')

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)
        sliced_data = sdba[-1:]

        self.assertIsInstance(sliced_data, SDataByAngle)
        self.assertTrue(np.allclose(self.frequencies, sliced_data.frequencies))
        self.assertEqual(sample_form, sliced_data.sample_form)
        self.assertEqual(temperature, sliced_data.temperature)

        self.assertTrue(
            np.allclose(self.list_data[1]['atom_0']['s']['order_1'],
                        sliced_data[0].extract()['atom_0']['s']['order_1']))

        self.assertEqual(len(sdba), len(self.list_data))
        self.assertEqual(len(sdba[-1:]), 1)

    def test_sdba_from_sdata_series(self):
        temperature, sample_form = (102., 'quasicrystal')
        angle_0_sdata = SData(data=self.list_data[0],
                              frequencies=self.frequencies,
                              temperature=temperature,
                              sample_form=sample_form)
        angle_1_sdata = SData(data=self.list_data[1],
                              frequencies=self.frequencies,
                              temperature=temperature,
                              sample_form=sample_form)

        sdba = SDataByAngle.from_sdata_series([angle_0_sdata, angle_1_sdata], angles=self.angles)

        for input_data, output_data in zip([angle_0_sdata, angle_1_sdata], sdba):
            self.assertEqual(input_data.get_sample_form(), output_data.get_sample_form())
            self.assertEqual(input_data.get_temperature(), output_data.get_temperature())
        self.assertTrue(np.allclose(input_data.get_frequencies(), output_data.get_frequencies()))
        self.assertTrue(
            np.allclose(input_data.extract()['atom_0']['s']['order_1'],
                        output_data.extract()['atom_0']['s']['order_1']))

    def test_sdba_from_sdata_series_bad_inputs(self):
        bad_data = [  # Inconsistent frequencies
            {
                'data': [
                    SData(data=self.list_data[0], frequencies=[1, 2, 3, 4, 5, 6]),
                    SData(data=self.list_data[1], frequencies=[2, 4, 6, 8, 10, 12])
                ],
                'angles': self.angles,
                'error': ValueError
            },
            # Inconsistent sample_form
            {
                'data': [
                    SData(data=self.list_data[0], frequencies=self.frequencies),
                    SData(data=self.list_data[1], frequencies=self.frequencies, sample_form='crystal')
                ],
                'angles': self.angles,
                'error': ValueError
            },
            # Inconsistent temperature
            {
                'data': [
                    SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.),
                    SData(data=self.list_data[1], frequencies=self.frequencies, temperature=200.)
                ],
                'angles': self.angles,
                'error': ValueError
            },
            # Inconsistent number of angles
            {
                'data': [SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.)],
                'angles': self.angles,
                'error': IndexError
            },
            # Bad typing
            {
                'data': [
                    SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.),
                    'SData (actually just a string)'
                ],
                'angles': self.angles,
                'error': TypeError
            },
        ]

        for dataset in bad_data:
            with self.assertRaises(dataset['error']):
                SDataByAngle.from_sdata_series(dataset['data'], angles=dataset['angles'])

    def test_s_data_sum_over_angles(self):
        temperature, sample_form = (100., 'crunchy powder')

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)

        for kwargs, ref_data in [({}, self.sum_data), ({
                'average': True
        }, self.avg_data), ({
                'weights': [2, 1]
        }, self.weighted_data)]:

            summed_data = sdba.sum_over_angles(**kwargs)
            for atom_key in ref_data:
                self.assertTrue(
                    np.allclose(summed_data.extract()[atom_key]['s']['order_1'], ref_data[atom_key]['s']['order_1']))
