# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.utils.nomad.diagnostics import CollimationLevel, _NOMADMedianDetectorTest

# third-party imports
import numpy as np
from numpy.testing import assert_allclose

# standard imports
import unittest


class NOMADMedianDetectorTestTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        r"""Instantiate an object of type _NOMADMedianDetectorTest"""
        #
        # tester 1
        #
        # Some tubes in the last two panels have some collimation. Their medians cannot be calculated
        config = {'collimation': {'half_col': list(range(77, 78)),  # last valid eightpack in next to last panel
                                  'full_col': list(range(95, 96))},  # last valid eightpack in last panel
                  'eight_packs': [3, 7, 8, 9, 10, 11, 19, 20, 26, 28, 30, 34, 38, 39, 40, 41, 44, 45, 46, 47, 48, 49,
                                  50, 54, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
                                  76, 77, 89, 90, 93, 94, 95],
                  }
        tester = _NOMADMedianDetectorTest()
        tester.config = config
        unique_intensities = np.arange(tester.PANEL_COUNT)

        # each panel will have same intensity for all its pixels
        intensities = list()
        for intensity, (tube_begin, tube_end) in enumerate(tester.tube_range):
            intensities.extend([intensity] * (tube_end - tube_begin) * tester.PIXELS_IN_TUBE)
        # divide each pixel intensity by tester.PIXELS_IN_TUBE so that the summed tube intensity becomes the panel index
        intensities = np.array(intensities, dtype=float) / tester.PIXELS_IN_TUBE
        # mask unused eightpacks
        mask = np.full(tester.EIGHTPACK_COUNT, True)
        mask[config['eight_packs']] = False  # don't mask eightpacks in use
        mask = np.repeat(mask, tester.TUBES_IN_EIGHTPACK * tester.PIXELS_IN_TUBE)
        tester.intensities = np.ma.masked_array(intensities, mask=mask)
        cls.tester1 = tester
        #
        # tester 2
        # the intensity of each pixel corresponds to the index of the tube to which the pixel belongs
        #
        config = {'eight_packs': [3, 7, 8, 9, 10, 11, 19, 20, 26, 28, 30, 34, 38, 39, 40, 41, 44, 45, 46, 47, 48, 49,
                                  50, 54, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
                                  76, 77, 89, 90, 93, 94, 95]
                  }
        tester = _NOMADMedianDetectorTest()
        tester.config = config
        unique_intensities = np.arange(tester.TUBE_COUNT)
        intensities = np.repeat(unique_intensities, tester.PIXELS_IN_TUBE)
        # mask unused eightpacks
        mask = np.full(tester.EIGHTPACK_COUNT, True)
        mask[config['eight_packs']] = False  # don't mask eightpacks in use
        mask = np.repeat(mask, tester.TUBES_IN_EIGHTPACK * tester.PIXELS_IN_TUBE)
        tester.intensities = np.ma.masked_array(intensities, mask=mask)
        cls.tester2 = tester

    def test_tube_range(self):
        ranges = _NOMADMedianDetectorTest().tube_range
        self.assertEquals(ranges[0], [0, 14 * 8])
        self.assertEqual(ranges[-1], [81 * 8, 99 * 8])

    def test_tube_intensity(self):
        # No masked eightpacks, the intensity of each pixel is the index of the tube to which the pixel belongs
        tester = _NOMADMedianDetectorTest()
        unique_intensities = np.arange(tester.TUBE_COUNT)
        tester.intensities = np.repeat(unique_intensities, tester.PIXELS_IN_TUBE)
        assert_allclose(tester.tube_intensity, tester.PIXELS_IN_TUBE * unique_intensities, atol=0.1)
        # Same as before, but some eightpacks are masked
        tester = self.tester2
        assert_allclose(tester.tube_intensity, tester.PIXELS_IN_TUBE * unique_intensities, atol=0.1)

    def test_tube_collevel(self):
        config = dict(collimation={'half_col': list(range(63, 81)),  # next to last panel
                                   'full_col': list(range(81, 99))})  # last panel
        # No masked eightpacks
        tester = _NOMADMedianDetectorTest()
        tester.config = config
        levels = tester.tube_collevel  # shape = (TUBE_COUNT,)
        c = tester.TUBES_IN_EIGHTPACK
        assert np.all(levels[0: 63 * c] == CollimationLevel.Empty)
        assert np.all(levels[63 * c: 81 * c] == CollimationLevel.Half)
        assert np.all(levels[81 * c:] == CollimationLevel.Full)
        # Masked eightpacks
        tester = self.tester1
        levels = tester.tube_collevel  # shape = (TUBE_COUNT,)
        assert len(levels) == tester.TUBE_COUNT
        first1, last1 = tester.TUBES_IN_EIGHTPACK * 77, tester.TUBES_IN_EIGHTPACK * 78
        assert np.all(levels[: first1]) == CollimationLevel.Empty
        assert np.all(levels[first1: last1]) == CollimationLevel.Half
        first2, last2 = tester.TUBES_IN_EIGHTPACK * 95, tester.TUBES_IN_EIGHTPACK * 96
        assert np.all(levels[last1: first2]) == CollimationLevel.Empty
        assert np.all(levels[first2: last2]) == CollimationLevel.Full
        assert np.all(levels[last2:]) == CollimationLevel.Empty

    def test_panel_median(self):
        tester = self.tester1
        medians = tester.panel_median
        assert medians.shape == (tester.TUBE_COUNT,)
        # The median of each tube in the panel should be the pixel intensity of the panel
        for intensity, (tube_begin, tube_end) in enumerate(tester.tube_range):
            # assertions are evaluated only on non-masked indexes
            assert_allclose(medians[tube_begin: tube_end], intensity, atol=0.1)

    def test_eightpack_median(self):
        tester = self.tester1
        medians = tester.eightpack_median
        assert medians.shape == (tester.TUBE_COUNT,)
        # The median of each tube in the panel should be the pixel intensity of the panel
        for intensity, (tube_begin, tube_end) in enumerate(tester.tube_range):
            # assertions are evaluated only on non-masked indexes
            assert_allclose(medians[tube_begin: tube_end], intensity, atol=0.1)

    def test_mask_by_tube_intensity(self):
        tester = self.tester1
        tester.intensities += 1e-06  # so that no intensity is zero
        # All pixels will be masked
        tester.config['threshold'] = {'low_tube': 1.5, 'high_tube': 0.5}
        mask = tester.mask_by_tube_intensity
        assert np.all(mask)
        # Only unused pixels will be masked
        tester.config['threshold'] = {'low_tube': 0.5, 'high_tube': 1.5}
        pixel_mask = tester.mask_by_tube_intensity
        pixel_unused, pixel_used = tester.intensities.mask, ~tester.intensities.mask
        assert ~np.all(pixel_mask[pixel_used])  # the mask of unused pixels is False
        assert np.all(pixel_mask[pixel_unused])  # the mask of unused pixels is True


if __name__ == "__main__":
    unittest.main()
