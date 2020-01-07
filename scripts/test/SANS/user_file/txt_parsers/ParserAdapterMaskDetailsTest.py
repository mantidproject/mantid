# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.FloatRange import FloatRange
from sans.common.enums import DetectorType
from sans.user_file.settings_tags import mask_block, MaskId, mask_block_cross, mask_line, range_entry_with_detector, \
    single_entry_with_detector
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterMaskDetailsTest(ParserAdapterTestCommon, unittest.TestCase):
    def test_block_masks(self):
        input_val = mask_block(horizontal1=1, horizontal2=2,
                               vertical1=3, vertical2=4,
                               detector_type=DetectorType.HAB)
        expected_val = [input_val]

        self.set_return_val({MaskId.BLOCK: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.block_masks)

    def test_block_cross(self):
        input_val = mask_block_cross(horizontal=None, vertical=None, detector_type=DetectorType.HAB)
        expected_val = [input_val]

        self.set_return_val({MaskId.BLOCK_CROSS: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.block_crosses)

    def test_line_mask(self):
        input_val = mask_line(width=1, angle=90, x=None, y=None)
        expected_val = [input_val]

        self.set_return_val({MaskId.LINE: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.line_masks)

    def test_time_masks(self):
        input_val = range_entry_with_detector(start=1, stop=2, detector_type=DetectorType.HAB)
        expected_val = [input_val]

        self.set_return_val({MaskId.TIME: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.time_masks)

    def test_time_detector_masks(self):
        input_val = range_entry_with_detector(start=2, stop=3, detector_type=DetectorType.LAB)
        expected_val = [input_val]

        self.set_return_val({MaskId.TIME_DETECTOR: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.time_masks_with_detector)

    def test_remove_time_masks(self):
        self.set_return_val({MaskId.CLEAR_TIME_MASK: True})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.remove_time_masks)
        self.assertFalse(returned.remove_detector_masks)

    def test_remove_detector_masks(self):
        self.set_return_val({MaskId.CLEAR_DETECTOR_MASK: True})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertTrue(returned.remove_detector_masks)
        self.assertFalse(returned.remove_time_masks)

    def test_remove_detector_defaults_false(self):
        self.set_return_val(None)

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertFalse(returned.remove_detector_masks)
        self.assertFalse(returned.remove_time_masks)

    @staticmethod
    def get_single_keys():
        return [MaskId.HORIZONTAL_SINGLE_STRIP_MASK, MaskId.VERTICAL_SINGLE_STRIP_MASK]

    @staticmethod
    def get_multiple_keys():
        return [MaskId.HORIZONTAL_RANGE_STRIP_MASK, MaskId.VERTICAL_RANGE_STRIP_MASK]

    def test_mask_strips_single(self):
        results = {}
        expected_val = [(DetectorType.HAB, 123)]

        for orientation in self.get_single_keys():
            self.set_return_val({orientation: single_entry_with_detector(123, DetectorType.HAB)})

            returned = self.instance.get_mask_details()

            self.assertIsNotNone(returned)
            results[orientation] = returned

        self.assertEqual(expected_val, results[MaskId.HORIZONTAL_SINGLE_STRIP_MASK].mask_horizontal_strips)
        self.assertEqual(expected_val, results[MaskId.VERTICAL_SINGLE_STRIP_MASK].mask_vertical_strips)

    def test_mask_strips_range(self):
        results = {}
        expected_val = [(DetectorType.LAB, 1), (DetectorType.LAB, 2), (DetectorType.LAB, 3)]

        for orientation in self.get_multiple_keys():
            self.set_return_val({orientation: range_entry_with_detector(start=1, stop=3,
                                                                        detector_type=DetectorType.LAB)})

            returned = self.instance.get_mask_details()

            self.assertIsNotNone(returned)
            results[orientation] = returned

        self.assertEqual(expected_val, results[MaskId.HORIZONTAL_RANGE_STRIP_MASK].mask_horizontal_strips)
        self.assertEqual(expected_val, results[MaskId.VERTICAL_RANGE_STRIP_MASK].mask_vertical_strips)

    def test_mask_horizontal_mixed(self):
        expected_val = [(DetectorType.HAB, 234),
                        (DetectorType.LAB, 1), (DetectorType.LAB, 2), (DetectorType.LAB, 3)]

        orientation_single = self.get_single_keys()
        orientation_range = self.get_multiple_keys()
        results = {}

        for index in range(len(self.get_multiple_keys())):
            self.set_return_val({orientation_single[index]: single_entry_with_detector(234, DetectorType.HAB),
                                 orientation_range[index]: range_entry_with_detector(start=1, stop=3,
                                                                                     detector_type=DetectorType.LAB)})

            returned = self.instance.get_mask_details()

            self.assertIsNotNone(returned)
            results[orientation_single[index]] = returned

        self.assertEqual(expected_val, results[MaskId.HORIZONTAL_SINGLE_STRIP_MASK].mask_horizontal_strips)
        self.assertEqual(expected_val, results[MaskId.VERTICAL_SINGLE_STRIP_MASK].mask_vertical_strips)

    def test_spectra_nums_single(self):
        input_val = 123
        expected_val = [input_val]

        self.set_return_val({MaskId.SINGLE_SPECTRUM_MASK: input_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.mask_spectra_nums)

    def test_spectra_nums_range(self):
        expected_val = [10, 11, 12]

        self.set_return_val({MaskId.SPECTRUM_RANGE_MASK: FloatRange(10, 12)})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.mask_spectra_nums)

    def test_spectra_nums_mixed(self):
        expected_val = [123, 10, 11, 12]

        self.set_return_val({MaskId.SINGLE_SPECTRUM_MASK: 123,
                             MaskId.SPECTRUM_RANGE_MASK: FloatRange(10, 12)})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.mask_spectra_nums)

    def test_mask_files_names(self):
        expected_val = "a.xml, b.xml"
        self.set_return_val({MaskId.FILE: expected_val})

        returned = self.instance.get_mask_details()

        self.assertIsNotNone(returned)
        self.assertEqual(expected_val, returned.mask_filenames)


if __name__ == '__main__':
    unittest.main()
