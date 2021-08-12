# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.utils.nomad.diagnostics import (determine_tubes_threshold, CollimationLevel, InstrumentComponentLevel,
                                            _NOMADMedianDetectorTest)

# third-party imports
import numpy as np
from numpy.testing import assert_allclose

# standard imports
from collections import namedtuple
import os
import tempfile
import unittest


class NOMADMedianDetectorTestTest(unittest.TestCase):

    @staticmethod
    def _generate_test_data():
        """

        Returns
        -------
        ~tuple
            namedtuple as instrument, dict as mask configuration, numpy.ndarray

        """
        # 16 pixels per tube
        # 2  tubes  per n-pack
        tube_size = 12
        pack_size = 2

        # Test data: 3 2-packs for m, m1 and m2
        test_data = np.zeros(shape=(3 * pack_size, tube_size), dtype='float')

        full_collimated = np.zeros((tube_size,), dtype='float')
        full_collimated[0::2] = np.arange(1, tube_size//2 + 1)
        full_collimated[1::2] = np.arange(tube_size, tube_size//2, -1)

        # not collimated (0, 1) and fully collimated (2, 3)
        test_data[0] = full_collimated
        test_data[1] = np.flip(full_collimated, axis=0)
        test_data[2] = full_collimated
        test_data[3] = np.flip(full_collimated, axis=0)
        # half collimated
        test_data[4] = np.arange(1, tube_size + 1)
        test_data[5] = np.arange(tube_size, 0, -1)

        # Intentionally set the maximum value to a very large number to make
        # difference between average and median
        test_data[0, 1] *= 10
        test_data[1, tube_size - 2] *= 10
        test_data[2, 1] *= 10
        test_data[3, tube_size - 2] *= 12
        test_data[4, tube_size - 1] *= 10
        test_data[5, 0] *= 12

        print(f'[DEBUG] Test 3 x 2 pack:\n{test_data}')

        # Mock instrument
        info_dict = dict()

        info_dict['num_banks'] = 1
        info_dict['num_8packs_per_bank'] = [6]  # [i, i+1) is the range of 8 packs for bank i
        info_dict['num_8packs'] = 3
        info_dict['num_pixels_per_tube'] = tube_size
        info_dict['num_tubes_per_8pack'] = 2
        info_dict['num_tubes'] = info_dict['num_8packs'] * info_dict['num_tubes_per_8pack']
        info_dict['num_pixels'] = info_dict['num_tubes'] * info_dict['num_pixels_per_tube']

        # convert to namedtuple and return
        instrument_class = namedtuple("nomad", info_dict)
        instrument = instrument_class(**info_dict)

        # Mask configuration
        mask_config = {'collimation': {'full_col': [2],
                                       'half_col': [3]},
                       'threshold': {'low_pixel': 0.9, 'high_pixel': 1.2,
                                     'low_tube': 0.7, 'high_tube': 1.3}}

        return instrument, mask_config, test_data.flatten()

    def _generate_test_ymal(self, file_name):
        """

        Returns
        -------

        """
        ymal_str = """

        #
        # Configuration file for generating mask file for NOMAD.
        #
        # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        # This file needs to be maintained by IS/CIS to update
        # for each cycle if necessary.
        # +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        #
        detector_ranges:
          #
          # This section contains information concerning the calculation
          # of solid angle corresponding to each pixel.
          #
          # The firste two numbers following each entry specify the range
          # of eight-packs corresponding each specific region. The third
          # number specifies the pixel width and the forth number is for
          # the exponential intended to be used for seach specific region.
          #
          # Non- back or forward scattering regions.
          range_1: [0, 13, 0.02, 2.5]
          range_2: [14, 36, 0.02, 3.0]
          range_3: [27, 50, 0.02, 3.0]
          range_4: [51, 62, 0.02, 3.0]
          # Back and forward scattering regions.
          range_back: [63, 80, 0.01, 2.0]
          range_forward: [81, 98, 0.01, 2.0]

        threshold:
          #
          # This block specifies the threshold (relative to median integrated
          # intensity of either pixel or tube) for masking out pixels.
          #
          low_pixel: 0.9
          high_pixel: 1.2
          low_tube: 0.7
          high_tube: 1.3

        collimation:
          #
          # This block specifies the full and half collimated eight-packs.
          #
          full_col: [1, 8, 16, 25, 27, 28, 29]
          half_col: [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46]

        # Indeces of eight-packs in use.
        eight_packs: [3,7,8,9,10,11,19,20,26,28,30,34,38,39,40,41,44,45,46,47,48,49,50,54,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,89,90,93,94,95]

        bank:
          #
          # Definition of range for each bank.
          #
          bank_1: [0, 5]
          bank_2: [6, 14]
          bank_3: [15, 22]
          bank_4: [23, 29]
          bank_5: [30, 44]
          bank_6: [45, 49]
        """
        yml_file = open(file_name, 'w')
        yml_file.write(ymal_str)
        yml_file.close()

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

    def test_ymal_input(self):
        """

        Returns
        -------

        """
        # Generate test file
        temp_dir = tempfile.mkdtemp()
        temp_ymal = os.path.join(temp_dir, 'nomad_mask.yml')
        self._generate_test_ymal(temp_ymal)
        assert os.path.exists(temp_ymal)

        mask_config = _NOMADMedianDetectorTest.parse_yaml(temp_ymal)
        assert isinstance(mask_config, dict)

        # AssertionError: ['detector_ranges', 'threshold', 'collimation', 'eight_packs', 'bank']
        print(f'{mask_config["collimation"]}')

        full_col_8packs = mask_config['collimation']['full_col']
        assert isinstance(full_col_8packs[0], int), f'{type(full_col_8packs[0])}'
        # {'full_col': [1, 8, 16, 25, 27, 28, 29], 'half_col': [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46]}
        assert 8 in full_col_8packs
        half_col_8packs = mask_config['collimation']['half_col']
        assert 36 in half_col_8packs

        # Get nomad instrument configuration
        nomad = _NOMADMedianDetectorTest.set_nomad_constants()

        # Test ymal and numpy conversion
        eight_pack_collimation_states = \
            _NOMADMedianDetectorTest.get_collimation_states(mask_config['collimation'],
                                                            nomad, InstrumentComponentLevel.EightPack)
        assert eight_pack_collimation_states.shape == (49,), f'{eight_pack_collimation_states.shape}'
        # check full collimated
        for pack_index in [1, 8, 16, 25, 27, 28, 29]:
            assert eight_pack_collimation_states[pack_index - 1] == 2,\
                f'Pack index {pack_index - 1}: {eight_pack_collimation_states[pack_index - 1]}'
        # check half collimated
        for pack_index in [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46]:
            assert eight_pack_collimation_states[pack_index - 1] == 1,\
                f'Pack index {pack_index - 1}: {eight_pack_collimation_states[pack_index - 1]}'
        # not collimated
        for pack_index in [2, 3, 4, 5, 6, 7, 9, 10, 11, 43, 44, 47, 48, 49]:
            assert eight_pack_collimation_states[pack_index - 1] == 0,\
                f'Pack index {pack_index - 1}: {eight_pack_collimation_states[pack_index - 1]}'

        # pixel collimation
        pixel_collimation_states = \
            _NOMADMedianDetectorTest.get_collimation_states(mask_config['collimation'],
                                                            nomad, InstrumentComponentLevel.Pixel)
        assert pixel_collimation_states.shape == (256 * 8 * 49, ), f'{pixel_collimation_states.shape}'
        # check full collimated
        for pack_index in [1, 8, 16, 25, 27, 28, 29]:
            pack_index -= 1
            np.testing.assert_allclose(pixel_collimation_states[pack_index * 8 * 256:(pack_index + 1) * 8 * 256],
                                       np.zeros(8 * 256) + 2,
                                       err_msg=f'Pack index {pack_index * 8 * 256}:... : '
                                               f'{pixel_collimation_states[pack_index]}')
        # check half collimated
        for pack_index in [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46]:
            pack_index -= 1
            np.testing.assert_allclose(pixel_collimation_states[pack_index * 8 * 256:(pack_index + 1) * 8 * 256],
                                       np.zeros(8 * 256) + 1,
                                       err_msg=f'Pack index {pack_index * 8 * 256}:... : '
                                               f'{pixel_collimation_states[pack_index]}')
        # not collimated
        for pack_index in [2, 3, 4, 5, 6, 7, 9, 10, 11, 43, 44, 47, 48, 49]:
            pack_index -= 1
            np.testing.assert_allclose(pixel_collimation_states[pack_index * 8 * 256:(pack_index + 1) * 8 * 256],
                                       np.zeros(8 * 256),
                                       err_msg=f'Pack index {pack_index * 8 * 256}:... : '
                                               f'{pixel_collimation_states[pack_index]}')

    def test_determine_tubes_thresholds(self):
        """

        Returns
        -------

        """
        # Set up test data
        mock_nomad, test_config, test_data = self._generate_test_data()

        # Determine threshold
        some_returns = determine_tubes_threshold(test_data, test_config, mock_nomad)

        # Gold file
        expected_mask_states = np.array([[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0],
                                         [0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
                                         [1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0],
                                         [0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1],
                                         [1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1],
                                         [1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1],])

        np.testing.assert_allclose(some_returns.reshape(6, 12).astype(int), expected_mask_states)

    def test_export_mantid_mask(self):
        """Test to export Mask to XML
        """
        # output file name
        test_xml_name = '/tmp/test_mask.xml'

        # mask states
        test_mask_states = numpy.ndarray(shape=(128*8*99, ), dtype=bool)
        test_mask_states[:] = False
        test_mask_states[0:123] = True
        test_mask_states[3000:4321] = True

        # export masks
        export_masks(test_mask_states, test_xml_name, 'NOMAD')

        # check
        assert os.path.exists(test_xml_name)
        with open(test_xml_name, 'r') as mask_file:
            lines = mask_file.readlines()
            assert lines[3].strip() == '<detids>0-122,3000-4320</detids>'
        os.remove(test_xml_name)

    def test_ymal_input(self):
        """

        Returns
        -------

        """
        # Generate test file
        temp_dir = tempfile.mkdtemp()
        temp_ymal = os.path.join(temp_dir, 'nomad_mask.yml')
        self._generate_test_ymal(temp_ymal)
        assert os.path.exists(temp_ymal)

        mask_config = _NOMADMedianDetectorTest.parse_yaml(temp_ymal)
        assert isinstance(mask_config, dict)

        # AssertionError: ['detector_ranges', 'threshold', 'collimation', 'eight_packs', 'bank']
        print(f'{mask_config["collimation"]}')

if __name__ == "__main__":
    unittest.main()
