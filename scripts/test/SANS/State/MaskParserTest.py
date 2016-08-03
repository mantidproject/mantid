import unittest
import mantid

from SANS2.State.StateBuilder.MaskParser import (parse_masking_string, SpectrumBlock)
from SANS2.State.StateDirector.TestDirector import TestDirector
from SANS2.Common.SANSEnumerations import DetectorType


class BlockParsingTest(unittest.TestCase):
    @staticmethod
    def _get_sample_state():
        # Get the sample state
        test_director = TestDirector()
        return test_director.construct()

    def test_that_block_is_parsed_correctly_with_h_before_v(self):
        # Arrange
        mask_command = "H12>H13+V14>V18"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(h*192 + 9 + v) for h in range(12, 14) for v in range(14, 19)])
        self.assertTrue(expected == mask_string)

    def test_that_block_is_parsed_correctly_with_v_before_h(self):
        # Arrange
        mask_command = "V12>V13+H14>H18"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(h*192 + 9 + v) for h in range(14, 19) for v in range(12, 14)])
        self.assertTrue(mask_string == expected)

    def test_that_block_cross_is_parsed_correctly_with_h_before_v(self):
        # Arrange
        mask_command = "H18+V12"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = str(18 * 192 + 9 + 12)
        self.assertTrue(mask_string == expected)

    def test_that_block_cross_is_parsed_correctly_with_h_before_v_v2(self):
        # Arrange
        mask_command = "V18+H12"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = str(12 * 192 + 9 + 18)
        self.assertTrue(mask_string == expected)


class RangeParsingTest(unittest.TestCase):
    @staticmethod
    def _get_sample_state():
        test_director = TestDirector()
        return test_director.construct()

    def test_that_block_is_parsed_correctly_for_h(self):
        # Arrange
        mask_command = "H12>H13"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(h*192 + 9 + v) for h in range(12, 14) for v in range(0, 192)])
        self.assertTrue(expected == mask_string)

    def test_that_block_is_parsed_correctly_for_v(self):
        # Arrange
        mask_command = "V12>V13"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(h * 192 + 9 + v) for h in range(0, 192) for v in range(12, 14)])
        self.assertTrue(expected == mask_string)

    def test_that_block_is_parsed_correctly_for_s(self):
        # Arrange
        mask_command = "S9>S16"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(s) for s in range(9, 17)])
        self.assertTrue(expected == mask_string)


class StripAndSingleSpectrumParsingTest(unittest.TestCase):
    @staticmethod
    def _get_sample_state():
        test_director = TestDirector()
        return test_director.construct()

    def test_horizontal_strip_is_extracted(self):
        # Arrange
        mask_command = "H12"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(12*192 + 9 + v) for v in range(0, 192)])
        self.assertTrue(expected == mask_string)

    def test_vertical_strip_is_extracted(self):
        # Arrange
        mask_command = "V12"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ",".join([str(h*192 + 9 + 12) for h in range(0, 192)])
        self.assertTrue(expected == mask_string)

    def test_single_spectrum_is_extracted(self):
        # Arrange
        mask_command = "S12"
        state = BlockParsingTest._get_sample_state()
        spectrum_block = SpectrumBlock(state.data, DetectorType.Lab)

        # Act
        mask_string = parse_masking_string(mask_command, spectrum_block)

        # Assert
        expected = ["12"]
        self.assertTrue(expected == mask_string)


if __name__ == '__main__':
    unittest.main()
