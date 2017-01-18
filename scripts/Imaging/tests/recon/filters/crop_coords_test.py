from __future__ import (absolute_import, division, print_function)
import unittest
import numpy.testing as npt


class CropCoordsTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(CropCoordsTest, self).__init__(*args, **kwargs)

        # force silent outputs
        from recon.configs.recon_config import ReconstructionConfig
        r = ReconstructionConfig.empty_init()
        r.func.verbosity = 0
        from recon.helper import Helper

        self.h = Helper(r)

    def test_import(self):
        """
        Only check that the filter imports successfully
        """
        from recon.filters import crop_coords
        pass

    def test_not_executed_volume(self):
        """
        Check that the filter is not executed when:
            - no Region of Interest is provided
            - Region of Interest is out of bounds anywhere
        """
        # images that will be put through testing
        images = self.generate_images()

        # control images
        control = self.generate_images()
        err_msg = "TEST NOT EXECUTED :: Running crop coords with Region of Interest {0} changed the data!"
        from recon.filters import crop_coords

        # left > right or top > bottom should not change the data
        roi = [61, 2, 5, 1]
        result = crop_coords.execute_volume(images, roi, self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(roi))

        roi = [5, 15, 5, 5]
        result = crop_coords.execute_volume(images, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = '[5, 5, 15, 5]'
        npt.assert_raises(
            ValueError, crop_coords.execute_volume, images, roi, self.h)

        roi = None
        result = crop_coords.execute_volume(images, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

    def test_executed_volume(self):
        """
        Check that the filter is  executed when:
            - valid Region of Interest is provided
        """
        from recon.filters import crop_coords
        # images that will be put through testing
        images = self.generate_images()

        # control images
        control = self.generate_images()

        roi = [5, 5, 5, 15]
        result = crop_coords.execute_volume(images, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        # reset data
        images = self.generate_images()
        roi = [5, 5, 15, 15]
        result = crop_coords.execute_volume(images, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    def test_not_executed_image(self):
        """
        Check that the filter is not executed when:
            - no Region of Interest is provided
            - Region of Interest is out of bounds anywhere
        """
        # images that will be put through testing
        image = self.generate_images()[0]

        # control image
        control = self.generate_images()[0]
        err_msg = "TEST NOT EXECUTED :: Running crop coords with Region of Interest {0} changed the data!"
        from recon.filters import crop_coords

        # left > right or top > bottom should not change the data
        roi = [61, 2, 5, 1]
        result = crop_coords.execute_image(image, roi, self.h)
        npt.assert_equal(
            result, control, err_msg=err_msg.format(roi))

        roi = [5, 15, 5, 5]
        result = crop_coords.execute_image(image, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

        roi = '[5, 5, 15, 5]'
        npt.assert_raises(
            ValueError, crop_coords.execute_image, image, roi, self.h)

        roi = None
        result = crop_coords.execute_image(image, roi, self.h)
        npt.assert_equal(result, control, err_msg=err_msg.format(roi))

    def test_executed_image(self):
        from recon.filters import crop_coords
        # image that will be put through testing
        image = self.generate_images()[0]

        # control image
        control = self.generate_images()[0]
        err_msg = "TEST EXECUTED :: Running crop coords with Region of Interest {0} didn't change the data!"

        roi = [5, 5, 5, 15]
        result = crop_coords.execute_image(image, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

        # reset data
        image = self.generate_images()[0]
        roi = [5, 5, 15, 15]
        result = crop_coords.execute_image(image, roi, self.h)
        npt.assert_raises(AssertionError, npt.assert_equal, result, control)

    @staticmethod
    def generate_images():
        import numpy as np
        # generate 10 images with dimensions 10x10, all values 1. float32
        return np.full((10, 10, 10), 1., dtype=np.float32)

if __name__ == '__main__':
    unittest.main()
