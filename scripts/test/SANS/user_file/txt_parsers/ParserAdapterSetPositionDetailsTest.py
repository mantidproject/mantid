# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.Position import XYPosition
from sans.common.enums import DetectorType
from sans.user_file.settings_tags import SetId, position_entry, set_scales_entry
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterSetPositionDetailsTest(ParserAdapterTestCommon, unittest.TestCase):
    def test_centre(self):
        for det_type in [DetectorType.HAB, DetectorType.LAB]:
            input_vals = position_entry(pos1=1, pos2=2, detector_type=det_type)
            expected_pos = XYPosition(X=1, Y=2)

            self.set_return_val({SetId.CENTRE: input_vals,
                                 SetId.CENTRE_HAB: input_vals})

            returned = self.instance.get_set_position_details()

            self.assertIsNotNone(returned)
            self.assertEqual({det_type: expected_pos}, returned.centre)

    def test_scales(self):
        expected_value = set_scales_entry(s=1, a=2, b=3, c=4, d=5)
        self.set_return_val({SetId.SCALES : expected_value})

        returned = self.instance.get_set_position_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_value, returned.scales)


if __name__ == '__main__':
    unittest.main()
