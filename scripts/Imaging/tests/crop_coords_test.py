from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt
from tests import test_helper as th


class CropCoordsTest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(CropCoordsTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from helper import Helper

        from filters import crop_coords
        self.alg = crop_coords

        self.h = Helper(r)

    def test_not_executed_volume(self):
        #Check that the filter is not executed when:
        #    - no Region of Interest is provided
        #    - Region of Interest is out of bounds anywhere
        # images that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()

        err_msg = "TEST NOT EXECUTED :: Running crop coords with Region of Interest {0} changed the data!"

        # left > right or top > bottom should not change the data
        roi = [61, 2, 5, 1]
        result = self.alg.execute_volume(images, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = [5, 15, 5, 5]
        result = self.alg.execute_volume(images, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = '[5, 5, 15, 5]'
        npt.assert_raises(ValueError, self.alg.execute_volume, images, roi,
                          self.h)

        roi = None
        result = self.alg.execute_volume(images, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

    def test_executed_volume(self):
        #Check that the filter is  executed when:
        #    - valid Region of Interest is provided

        # images that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()

        # control images
        roi = [5, 5, 5, 15]
        result = self.alg.execute_volume(images, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        # reset data
        images, control = th.gen_img_shared_array_and_copy()

        roi = [5, 5, 15, 15]
        result = self.alg.execute_volume(images, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        images, control = th.gen_img_shared_array_and_copy()

        roi = [1, 1, 5, 5]
        result = self.alg.execute_volume(images, roi, self.h)
        expected_shape = (10, 4, 4)
        npt.assert_equal(result.shape, expected_shape)

    def test_not_executed_image(self):
        # Check that the filter is not executed when:
        #     - no Region of Interest is provided
        #     - Region of Interest is out of bounds anywhere

        # images that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()
        image = images[0]
        control = control[0]
        err_msg = "TEST NOT EXECUTED :: Running crop coords with Region of Interest {0} changed the data!"

        # left > right or top > bottom should not change the data
        roi = [61, 2, 5, 1]
        result = self.alg.execute_image(image, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = [5, 15, 5, 5]
        result = self.alg.execute_image(image, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = '[5, 5, 15, 5]'
        npt.assert_raises(ValueError, self.alg.execute_image, image, roi,
                          self.h)

        roi = None
        result = self.alg.execute_image(image, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

    def test_executed_image(self):
        # image that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()
        image = images[0]
        control = control[0]

        err_msg = "TEST EXECUTED :: Running crop coords with Region of Interest {0} didn't change the data!"

        roi = [5, 5, 5, 15]
        result = self.alg.execute_image(image, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        # reset data
        image = th.gen_img_shared_array()[0]
        roi = [5, 5, 15, 15]
        result = self.alg.execute_image(image, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    def test_executed_image_no_helper(self):
        # image that will be put through testing
        images, control = th.gen_img_shared_array_and_copy()
        image = images[0]
        control = control[0]
        err_msg = "TEST EXECUTED :: Running crop coords with Region of Interest {0} didn't change the data!"

        roi = [5, 5, 5, 15]
        result = self.alg.execute_image(image, roi)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        # reset data
        images, control = th.gen_img_shared_array_and_copy()
        roi = [5, 5, 15, 15]
        result = self.alg.execute_image(image, roi)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)


if __name__ == '__main__':
    unittest.main()
