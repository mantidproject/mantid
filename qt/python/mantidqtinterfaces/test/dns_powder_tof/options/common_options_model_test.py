# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""
import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns_powder_tof.options import common_options_model
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import \
    DNSCommonOptionsModel


class DNSCommonOptionsModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock
        cls.model = DNSCommonOptionsModel(parent=cls.parent)
        cls.fulldata = [{
            'det_rot': -5,
            'selector_speed': 1000,
            'wavelength': 47.4
        }, {
            'det_rot': -6,
            'selector_speed': 200,
            'wavelength': 31.4
        }]

    def test___init__(self):
        self.assertIsInstance(self.model, DNSObsModel)

    def test_get_det_rot_min_max(self):
        testv = self.model.get_det_rot_min_max(self.fulldata)
        self.assertEqual(testv, [5, 6])

    def test_selector_wavelength(self):
        testv = common_options_model._selector_wavelength(0)
        self.assertAlmostEqual(testv, 1000)
        testv = common_options_model._selector_wavelength(500)
        self.assertAlmostEqual(testv, 66.72)

    def test_get_selector_speeds(self):
        testv = common_options_model._get_selector_speeds(self.fulldata)
        self.assertEqual(testv, [1000, 200])

    def test_selector_speed_varies(self):
        testv = common_options_model._selector_speed_varies([1000, 200])
        self.assertTrue(testv)
        testv = common_options_model._selector_speed_varies([1000, 1001])
        self.assertFalse(testv)

    def test_get_wavelengths(self):
        testv = common_options_model._get_wavelengths(self.fulldata)
        self.assertAlmostEqual(testv[0], 4.74)
        self.assertAlmostEqual(testv[1], 3.14)

    def test_wavelength_varies(self):
        testv = common_options_model._wavelength_varies([2, 2, 2])
        self.assertFalse(testv)
        testv = common_options_model._wavelength_varies([2, 2, 3])
        self.assertTrue(testv)

    def test_selector_wavelength_missmatch(self):
        testv = common_options_model._selector_wavelength_missmatch(4, 10)
        self.assertTrue(testv)
        testv = common_options_model._selector_wavelength_missmatch(4, 4.1)
        self.assertFalse(testv)

    def test_determine_wavelength(self):
        testv = self.model.determine_wavelength(self.fulldata)
        self.assertAlmostEqual(testv[0], 4.74)
        self.assertTrue(testv[1]['wavelength_varies'])
        self.assertTrue(testv[1]['selector_wavelength_missmatch'])
        self.assertTrue(testv[1]['selector_speed_varies'])


if __name__ == '__main__':
    unittest.main()
