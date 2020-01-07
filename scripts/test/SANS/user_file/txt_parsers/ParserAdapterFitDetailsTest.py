# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.common.enums import FitType, DataType
from sans.user_file.settings_tags import fit_general, FitId
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterTestFitDetailsTest(ParserAdapterTestCommon, unittest.TestCase):

    def test_transmission_fit(self):
        expected_val = fit_general(start=1, stop=2, fit_type=FitType.LOGARITHMIC,
                                   data_type=DataType.CAN, polynomial_order=1)
        self.set_return_val({FitId.GENERAL: expected_val})

        returned = self.instance.get_fit_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.transmission_fit[0])
        self.assertEqual(expected_val, returned.transmission_fit[1])

    def test_transmission_fit_marked_off_when_no_fit(self):
        expected_val = fit_general(start=1, stop=2, fit_type=FitType.NO_FIT,
                                   data_type=DataType.SAMPLE, polynomial_order=1)
        self.set_return_val({FitId.GENERAL: expected_val})

        returned = self.instance.get_fit_details()

        self.assertIsNotNone(returned)
        self.assertFalse(returned.transmission_fit[0])
        self.assertEqual(expected_val, returned.transmission_fit[1])

    def test_monitor_times(self):
        expected_val = FloatRange(1.1, 2.3)
        self.set_return_val({FitId.MONITOR_TIMES: expected_val})

        returned = self.instance.get_fit_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.monitor_times[0])
        self.assertEqual(expected_val, returned.monitor_times[1])

    def test_set_off_by_default(self):
        self.set_return_val(None)

        returned = self.instance.get_fit_details()
        self.assertFalse(returned.monitor_times[0])
        self.assertFalse(returned.transmission_fit[0])


if __name__ == '__main__':
    unittest.main()
