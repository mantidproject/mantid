from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th
from core.filters import crop_coords


class CropCoordsTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(CropCoordsTest, self).__init__(*args, **kwargs)

    def test_executed_with_flat_and_dark(self):
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.shared_deepcopy(images)[0]
        dark = th.shared_deepcopy(images)[0]

        roi = [1, 1, 5, 5]
        r_sample, r_flat, r_dark = crop_coords.execute(images, roi, flat, dark)
        expected_shape_sample = (10, 4, 4)
        expected_shape_flat_dark = (4, 4)
        npt.assert_equal(r_sample.shape, expected_shape_sample)
        npt.assert_equal(r_flat.shape, expected_shape_flat_dark)
        npt.assert_equal(r_dark.shape, expected_shape_flat_dark)

    def test_executed_with_only_flat_and_no_dark(self):
        """
        The filter will execute but BOTH flat and dark will be None!
        """
        images, control = th.gen_img_shared_array_and_copy()
        flat = th.shared_deepcopy(images)[0]
        dark = None

        roi = [1, 1, 5, 5]
        r_sample, r_flat, r_dark = crop_coords.execute(images, roi, flat, dark)
        expected_shape_sample = (10, 4, 4)
        npt.assert_equal(r_sample.shape, expected_shape_sample)
        npt.assert_equal(r_flat, None)
        npt.assert_equal(r_dark, None)

    def test_executed_with_no_flat_and_only_dark(self):
        """
        The filter will execute but BOTH flat and dark will be None!
        """
        images, control = th.gen_img_shared_array_and_copy()
        flat = None
        dark = th.shared_deepcopy(images)[0]

        roi = [1, 1, 5, 5]
        r_sample, r_flat, r_dark = crop_coords.execute(images, roi, flat, dark)
        expected_shape_sample = (10, 4, 4)
        npt.assert_equal(r_sample.shape, expected_shape_sample)
        npt.assert_equal(r_flat, None)
        npt.assert_equal(r_dark, None)

    def test_executed_only_volume(self):
        # Check that the filter is  executed when:
        #   - valid Region of Interest is provided
        #   - no flat or dark images are provided

        images, control = th.gen_img_shared_array_and_copy()
        roi = [1, 1, 5, 5]
        result = crop_coords.execute(images, roi)[0]
        expected_shape = (10, 4, 4)
        npt.assert_equal(result.shape, expected_shape)

    def test_not_executed_no_sample(self):
        # images that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()
        image = images[0]
        control = control[0]

        # not executed because no Sample is provided
        roi = [1, 2, 5, 1]
        npt.assert_raises(AssertionError, crop_coords.execute, None, roi)

    def test_not_executed_no_roi(self):
        # images that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()
        image = images[0]
        control = control[0]

        # not executed because no Region of interest is provided
        roi = None
        result = crop_coords.execute(image, roi)[0]
        npt.assert_equal(result, control)


if __name__ == '__main__':
    unittest.main()
