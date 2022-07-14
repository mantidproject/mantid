# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which interpolates DNS standard data to bank positions
"""
import unittest
from unittest.mock import call
from unittest.mock import patch
import numpy as np
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import \
    get_elastic_standard_datadic

from mantidqtinterfaces.dns_powder_elastic.data_structures. \
    dns_standard_interpolator import (
        read_standard_file, create_array, average_array, interp,
        closest_file_number, write_inp_file, interpolate_standard
        )


class DNSStandardInterpolatorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.arr1 = np.array(
            [-5., 10., 3., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12.,
             13.,
             14., 15., 16., 17., 18., 19., 20., 21., 22., 23., 24.]
        )
        cls.arr2 = np.vstack((cls.arr1 * 2, cls.arr1))
        cls.standarddata = get_elastic_standard_datadic()

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_standard_interpolator.DNSFile')
    def test_read_standard_file(self, mock_dnsfile):
        mock_dnsfile.return_value.det_rot = -5
        mock_dnsfile.return_value.timer = 10
        mock_dnsfile.return_value.monitor = 3
        mock_dnsfile.return_value.counts = np.zeros((2, 1))
        testv = read_standard_file('C:/123', 5)
        self.assertTrue((testv == np.array([-5., 10., 3., 0., 0.])).all())

    @patch(
        'mantidqtinterfaces.dns_powder_elastic.data_structures.'
        'dns_standard_interpolator.'
        'read_standard_file')
    def test_create_array(self, mock_read):
        mock_read.return_value = self.arr1
        testv = create_array('C:/123', [1, 2])
        self.assertIsInstance(testv[0], np.ndarray)
        self.assertEqual(testv[1], {-5.0: 2})
        self.assertEqual(testv[0].shape, (2, 27))
        self.assertTrue((testv[0][0] == mock_read.return_value).all())

    def test_average_array(self):
        tesarr = np.array([[1, 2, 3], [4, 5, 6], [1.01, 7, 8]])
        testv = average_array(tesarr)
        self.assertTrue(
            (testv == np.array([[1., 9., 11.], [4., 5., 6.]])).all())

    def test_interp(self):
        testv = interp(self.arr2, [-5, -6])
        self.assertEqual(testv.shape, (2, 27))
        self.assertTrue((testv[1] == np.array(
            [-6., 12., 3.6, 1.2, 2.4, 3.6, 4.8, 6., 7.2, 8.4, 9.6, 10.8, 12.,
             13.2, 14.4, 15.6, 16.8, 18., 19.2, 20.4, 21.6, 22.8, 24., 25.2,
             26.4, 27.6, 28.8])).all())
        self.assertTrue((testv[0] == self.arr1).all())

    def test_closest_file_number(self):
        testv = closest_file_number(5.2, {5: 1, 6: 2, 5.1: 2})
        self.assertEqual(testv, 2)
        testv = closest_file_number(5, {5: 1, 6: 2, 5.1: 2})
        self.assertEqual(testv, 1)

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_standard_interpolator.DNSFile')
    def test_write_inp_file(self, mock_dnsfile):
        testv = write_inp_file(self.arr2, 'C:', {5: 56, 6: 2, 5.1: 2}, 56,
                               '123')
        self.assertEqual(mock_dnsfile.call_count, 2)
        mock_dnsfile.has_calls(
            [call('C:/', '_000056.d_dat'), call('C:/', '_000056.d_dat')])
        self.assertEqual(mock_dnsfile.return_value.det_rot, -5)
        self.assertEqual(mock_dnsfile.return_value.timer, 10)
        self.assertEqual(mock_dnsfile.return_value.monitor, 3)
        self.assertTrue((mock_dnsfile.return_value.counts == np.reshape(
            self.arr2[1, 3:], (24, 1))).all())
        self.assertEqual(mock_dnsfile.return_value.write.call_count, 2)
        mock_dnsfile.return_value.write.has_calls(
            [call('C:/interp/', '123_000056.d_dat'),
             call('C:/interp/', '123_000057.d_dat')])
        self.assertEqual(testv, [56, 57])

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_standard_interpolator.'
           'create_dir')
    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_standard_interpolator.'
           'create_array')
    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_standard_interpolator.'
           'write_inp_file')
    def test_interpolate_standard(self, mock_write, mock_create_array,
                                  mock_cdir):
        mock_create_array.return_value = [self.arr2, {-5: 2, -6: 3}]
        mock_write.return_value = [2, 3]

        testv = interpolate_standard(self.standarddata, [-6], 'a.py')
        mock_cdir.has_calls(call('C:/interp/'))
        self.assertEqual(testv, [{'vana': {
            'path': 'C:/interp/a_ip_vana',
            'z_nsf': [2, 3], 'z_sf': [2, 3]}, 'nicr': {
            'path': 'C:/interp/a_ip_nicr',
            'x_nsf': [2, 3], 'x_sf': [2, 3], 'y_nsf': [2, 3], 'y_sf': [2, 3],
            'z_nsf': [2, 3], 'z_sf': [2, 3]}, 'empty': {
            'path': 'C:/interp/a_ip_empty',
            'x_nsf': [2, 3], 'x_sf': [2, 3], 'y_nsf': [2, 3], 'y_sf': [2, 3],
            'z_nsf': [2, 3], 'z_sf': [2, 3]}}, False])
        self.assertEqual(mock_write.call_count, 14)
        self.assertEqual(len(mock_write.call_args[0]), 5)
        self.assertTrue((mock_write.call_args[0][0] == np.array(
            [[-6., 12., 3.6, 1.2, 2.4, 3.6, 4.8, 6., 7.2, 8.4, 9.6,
              10.8, 12., 13.2, 14.4, 15.6, 16.8, 18., 19.2, 20.4, 21.6, 22.8,
              24., 25.2, 26.4, 27.6, 28.8]])).all())
        self.assertEqual(mock_write.call_args[0][1],
                         'C:/_knso_554573_to_554632_ip_empty')
        self.assertEqual(mock_write.call_args[0][2], {-5: 2, -6: 3})
        self.assertEqual(mock_write.call_args[0][3], 4)
        self.assertEqual(mock_write.call_args[0][4], 'a_ip_empty')
        if __name__ == '__main__':
            unittest.main()
