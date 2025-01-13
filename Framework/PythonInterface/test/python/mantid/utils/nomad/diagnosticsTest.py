# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.utils.nomad.diagnostics import CollimationLevel, InstrumentComponentLevel, _NOMADMedianDetectorTest

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
        test_data = np.zeros(shape=(3 * pack_size, tube_size), dtype="float")

        full_collimated = np.zeros((tube_size,), dtype="float")
        full_collimated[0::2] = np.arange(1, tube_size // 2 + 1)
        full_collimated[1::2] = np.arange(tube_size, tube_size // 2, -1)

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

        # Mock instrument
        info_dict = dict()

        info_dict["num_Panels"] = 1
        info_dict["num_8packs"] = 3
        info_dict["num_pixels_per_tube"] = tube_size
        info_dict["num_tubes_per_8pack"] = 2
        info_dict["num_tubes"] = info_dict["num_8packs"] * info_dict["num_tubes_per_8pack"]
        info_dict["num_pixels"] = info_dict["num_tubes"] * info_dict["num_pixels_per_tube"]

        # convert to namedtuple and return
        instrument_class = namedtuple("nomad", info_dict)
        instrument = instrument_class(**info_dict)

        # Mask configuration
        mask_config = {
            "collimation": {"full_col": [1], "half_col": [2]},
            "threshold": {"low_pixel": 0.9, "high_pixel": 1.2, "low_tube": 0.7, "high_tube": 1.3},
        }

        return instrument, mask_config, test_data.flatten()

    @classmethod
    def setUpClass(cls):
        r"""Instantiate an object of type _NOMADMedianDetectorTest"""
        #
        # tester 1
        # each panel will have same intensity for all its pixels
        #
        # Some tubes in the last two panels have some collimation. Their medians cannot be calculated
        config = {
            "collimation": {
                "half_col": list(range(77, 78)),  # last valid eightpack in next to last panel
                "full_col": list(range(95, 96)),
            },  # last valid eightpack in last panel
            "eight_packs": [
                3,
                7,
                8,
                9,
                10,
                11,
                19,
                20,
                26,
                28,
                30,
                34,
                38,
                39,
                40,
                41,
                44,
                45,
                46,
                47,
                48,
                49,
                50,
                54,
                57,
                58,
                59,
                60,
                61,
                62,
                63,
                64,
                65,
                66,
                67,
                68,
                69,
                70,
                71,
                72,
                73,
                74,
                75,
                76,
                77,
                89,
                90,
                93,
                94,
                95,
            ],
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
        mask[config["eight_packs"]] = False  # don't mask eightpacks in use
        mask = np.repeat(mask, tester.TUBES_IN_EIGHTPACK * tester.PIXELS_IN_TUBE)
        tester.intensities = np.ma.masked_array(intensities, mask=mask)
        cls.tester1 = tester
        #
        # tester 2
        # the intensity of each pixel corresponds to the index of the tube to which the pixel belongs
        #
        config = {
            "eight_packs": [
                3,
                7,
                8,
                9,
                10,
                11,
                19,
                20,
                26,
                28,
                30,
                34,
                38,
                39,
                40,
                41,
                44,
                45,
                46,
                47,
                48,
                49,
                50,
                54,
                57,
                58,
                59,
                60,
                61,
                62,
                63,
                64,
                65,
                66,
                67,
                68,
                69,
                70,
                71,
                72,
                73,
                74,
                75,
                76,
                77,
                89,
                90,
                93,
                94,
                95,
            ]
        }
        tester = _NOMADMedianDetectorTest()
        tester.config = config
        unique_intensities = np.arange(tester.TUBE_COUNT)
        intensities = np.repeat(unique_intensities, tester.PIXELS_IN_TUBE)
        # mask unused eightpacks
        mask = np.full(tester.EIGHTPACK_COUNT, True)
        mask[config["eight_packs"]] = False  # don't mask eightpacks in use
        mask = np.repeat(mask, tester.TUBES_IN_EIGHTPACK * tester.PIXELS_IN_TUBE)
        tester.intensities = np.ma.masked_array(intensities, mask=mask)
        cls.tester2 = tester

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

        # Indices of eight-packs in use.
        eight_packs: [3,7,8,9,10,11,19,20,26,28,30,34,38,39,40,41,44,45,46,47,48,49,50,54,57,58,59,60,
                      61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,89,90,93,94,95]

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
        yml_file = open(file_name, "w")
        yml_file.write(ymal_str)
        yml_file.close()

    def test_ymal_input(self):
        """Test processing YMAL"""
        # Generate test file
        temp_dir = tempfile.mkdtemp()
        temp_ymal = os.path.join(temp_dir, "nomad_mask.yml")
        self._generate_test_ymal(temp_ymal)
        assert os.path.exists(temp_ymal)

        # Parse yaml
        mask_config = _NOMADMedianDetectorTest.parse_yaml(temp_ymal)
        assert isinstance(mask_config, dict)

        # Check parsing
        full_col_8packs = mask_config["collimation"]["full_col"]
        # eight_packs: [3,7,8,9,10,11,19,20,26,28,30,34, ...]
        # full_col: from [1, 8, 16, 25, 27, 28, 29] -> [7, 26, ...]
        assert 7 in full_col_8packs and 26 in full_col_8packs, f"Mapped full column 8 packs: {full_col_8packs}"
        # half_col': [30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46] -> [...]
        half_col_8packs = mask_config["collimation"]["half_col"]
        assert 63 in half_col_8packs and 90 in half_col_8packs, f"Mapped half column 8 packs: {half_col_8packs}"

        # Get nomad instrument configuration
        nomad = _NOMADMedianDetectorTest.set_nomad_constants()

        # Test ymal and numpy conversion
        eight_pack_collimation_states = _NOMADMedianDetectorTest.get_collimation_states(
            mask_config["collimation"], nomad, InstrumentComponentLevel.EightPack
        )
        assert eight_pack_collimation_states.shape == (99,), f"{eight_pack_collimation_states.shape}"
        # check full collimated
        for pack_index in [7, 26]:
            assert eight_pack_collimation_states[pack_index] == 2, (
                f"Pack index {pack_index}: {eight_pack_collimation_states[pack_index]} in {eight_pack_collimation_states}"
            )
        # check half collimated
        for pack_index in [63, 90]:
            assert eight_pack_collimation_states[pack_index] == 1, f"Pack index {pack_index}: {eight_pack_collimation_states[pack_index]}"
        # not collimated
        for pack_index in [2, 3, 4, 5, 6, 91, 92]:
            assert eight_pack_collimation_states[pack_index] == 0, f"Pack index {pack_index}: {eight_pack_collimation_states[pack_index]}"

        # pixel collimation
        pixel_collimation_states = _NOMADMedianDetectorTest.get_collimation_states(
            mask_config["collimation"], nomad, InstrumentComponentLevel.Pixel
        )
        assert pixel_collimation_states.shape == (128 * 8 * 99,), f"{pixel_collimation_states.shape}"
        # check full collimated
        for pack_index in [7, 26]:
            np.testing.assert_allclose(
                pixel_collimation_states[pack_index * 8 * 128 : (pack_index + 1) * 8 * 128],
                np.zeros(8 * 128) + 2,
                err_msg=f"Pack index {pack_index * 8 * 128}:... : {pixel_collimation_states[pack_index]}",
            )
        # check half collimated
        for pack_index in [63, 90]:
            np.testing.assert_allclose(
                pixel_collimation_states[pack_index * 8 * 128 : (pack_index + 1) * 8 * 128],
                np.zeros(8 * 128) + 1,
                err_msg=f"Pack index {pack_index * 8 * 128}:... : {pixel_collimation_states[pack_index]}",
            )
        # not collimated
        for pack_index in [2, 3, 4, 5, 6, 91, 92]:
            np.testing.assert_allclose(
                pixel_collimation_states[pack_index * 8 * 128 : (pack_index + 1) * 8 * 128],
                np.zeros(8 * 128),
                err_msg=f"Pack index {pack_index * 8 * 128}:... : {pixel_collimation_states[pack_index]}",
            )

    def test_determine_tubes_thresholds(self):
        # Set up test data
        mock_nomad, test_config, test_data = self._generate_test_data()

        # Determine threshold
        some_returns = _NOMADMedianDetectorTest.determine_tubes_threshold(test_data, test_config, mock_nomad)

        # Gold file
        expected_mask_states = np.array(
            [
                [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0],
                [0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
                [1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0],
                [0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1],
                [1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1],
                [1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1],
            ]
        )

        np.testing.assert_allclose(some_returns.reshape(6, 12).astype(int), expected_mask_states)

    def test_tube_range(self):
        ranges = _NOMADMedianDetectorTest().tube_range
        self.assertEqual(ranges[0], [0, 14 * 8])
        self.assertEqual(ranges[-1], [81 * 8, 99 * 8])

    def test_tube_in_flat_panel(self):
        states = _NOMADMedianDetectorTest().tube_in_flat_panel
        self.assertTrue(np.all(~states[0:504]))
        self.assertTrue(np.all(states[504:]))

    def test_pixel_in_flat_panel(self):
        states = _NOMADMedianDetectorTest().pixel_in_flat_panel
        self.assertTrue(np.all(~states[0 : 504 * 128]))
        self.assertTrue(np.all(states[504 * 128 :]))

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
        config = dict(collimation={"half_col": list(range(63, 81)), "full_col": list(range(81, 99))})  # next to last panel  # last panel
        # No masked eightpacks
        tester = _NOMADMedianDetectorTest()
        tester.config = config
        levels = tester.tube_collevel  # shape = (TUBE_COUNT,)
        c = tester.TUBES_IN_EIGHTPACK
        assert np.all(levels[0 : 63 * c] == CollimationLevel.Empty)
        assert np.all(levels[63 * c : 81 * c] == CollimationLevel.Half)
        assert np.all(levels[81 * c :] == CollimationLevel.Full)
        # Masked eightpacks
        tester = self.tester1
        levels = tester.tube_collevel  # shape = (TUBE_COUNT,)
        assert len(levels) == tester.TUBE_COUNT
        first1, last1 = tester.TUBES_IN_EIGHTPACK * 77, tester.TUBES_IN_EIGHTPACK * 78
        assert np.all(levels[:first1] == CollimationLevel.Empty)
        assert np.all(levels[first1:last1] == CollimationLevel.Half)
        first2, last2 = tester.TUBES_IN_EIGHTPACK * 95, tester.TUBES_IN_EIGHTPACK * 96
        assert np.all(levels[last1:first2] == CollimationLevel.Empty)
        assert np.all(levels[first2:last2] == CollimationLevel.Full)
        assert np.all(levels[last2:] == CollimationLevel.Empty)

    def test_panel_median(self):
        tester = self.tester1
        medians = tester.panel_median
        assert medians.shape == (tester.TUBE_COUNT,)
        # The median of each tube in the panel should be the pixel intensity of the panel
        for intensity, (tube_begin, tube_end) in enumerate(tester.tube_range):
            # assertions are evaluated only on non-masked indexes
            assert_allclose(medians[tube_begin:tube_end], intensity, atol=0.1)

    def test_eightpack_median(self):
        tester = self.tester1
        medians = tester.eightpack_median
        assert medians.shape == (tester.TUBE_COUNT,)
        # The median of each tube in the panel should be the pixel intensity of the panel
        for intensity, (tube_begin, tube_end) in enumerate(tester.tube_range):
            # assertions are evaluated only on non-masked indexes
            assert_allclose(medians[tube_begin:tube_end], intensity, atol=0.1)

    def test_mask_by_tube_intensity(self):
        tester = self.tester1
        tester.intensities += 1e-06  # so that no intensity is zero
        tester.config["threshold"] = {"low_tube": 1.5, "high_tube": 0.5}
        mask = tester.mask_by_tube_intensity
        # Only pixels in use and in flat panels will remain unmasked
        self.assertTrue(~np.all(mask[tester.pixel_in_flat_panel & tester.pixel_in_use]))
        self.assertTrue(np.all(~mask[tester.pixel_in_flat_panel & tester.pixel_in_use]))  # all other pixels are masked
        # Only unused pixels will be masked
        tester.config["threshold"] = {"low_tube": 0.5, "high_tube": 1.5}
        pixel_mask = tester.mask_by_tube_intensity
        pixel_unused, pixel_used = tester.intensities.mask, ~tester.intensities.mask
        assert ~np.all(pixel_mask[pixel_used])  # the mask of unused pixels is False
        assert np.all(pixel_mask[pixel_unused])  # the mask of unused pixels is True

    def test_mask_by_pixel_intensity(self):
        tester = self.tester1
        tester.intensities += 1e-06  # so that no intensity is zero
        # All pixels will be masked
        tester.config["threshold"] = {"low_pixel": 1.5, "high_pixel": 0.5}
        mask = tester.mask_by_pixel_intensity
        assert np.all(mask)
        # Only unused pixels will be masked
        tester.config["threshold"] = {"low_tube": 0.5, "high_tube": 1.5}
        pixel_mask = tester.mask_by_tube_intensity
        pixel_unused, pixel_used = tester.intensities.mask, ~tester.intensities.mask
        assert ~np.all(pixel_mask[pixel_used])  # the mask of unused pixels is False
        assert np.all(pixel_mask[pixel_unused])  # the mask of unused pixels is True


if __name__ == "__main__":
    unittest.main()
