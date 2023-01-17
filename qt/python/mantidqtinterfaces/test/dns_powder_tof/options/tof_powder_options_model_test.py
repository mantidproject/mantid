# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI.
"""

import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.options import tof_powder_options_model
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import DNSCommonOptionsModel
from mantidqtinterfaces.dns_powder_tof.options.tof_powder_options_model import DNSTofPowderOptionsModel
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_dataset, get_fake_tof_binning


class DNSTofPowderOptionsModelTest(unittest.TestCase):
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.model = DNSTofPowderOptionsModel(parent=cls.parent)
        cls.fulldata = get_dataset()  # 2 datafiles, one is tof the other not

    def test___init__(self):
        self.assertIsInstance(self.model, DNSTofPowderOptionsModel)
        self.assertIsInstance(self.model, DNSCommonOptionsModel)
        self.assertIsInstance(self.model, DNSObsModel)

    def test_get_channel_widths(self):
        testv = tof_powder_options_model.get_channel_widths(self.fulldata)
        self.assertEqual(testv[0], [2.0, 1.6])
        self.assertEqual(testv[1], True)

    def test_channel_width_varies(self):
        self.assertTrue(tof_powder_options_model.channel_width_varies([0, 1]))
        self.assertFalse(tof_powder_options_model.channel_width_varies([1, 1]))
        self.assertFalse(tof_powder_options_model.channel_width_varies([0]))

    def test_get_tof_channels(self):
        testv = tof_powder_options_model.get_tof_channels(self.fulldata)
        self.assertEqual(testv[0], [1, 1000])
        self.assertEqual(testv[1], True)

    def test_number_of_tof_channels_varies(self):
        self.assertTrue(tof_powder_options_model.number_of_tof_channels_varies([0, 1]))
        self.assertFalse(tof_powder_options_model.number_of_tof_channels_varies([1, 1]))
        self.assertFalse(tof_powder_options_model.number_of_tof_channels_varies([0]))

    def test_estimate_q_and_binning(self):
        testv = self.model.estimate_q_and_binning(self.fulldata, 4.74)
        for key, value in testv[0].items():
            self.assertAlmostEqual(value, get_fake_tof_binning()[key])


if __name__ == "__main__":
    unittest.main()
