import unittest
import mantid
import isis_instrument as instruments


class SANSIsisInstrumentTest(unittest.TestCase):
    def test_add_TOFs_for_ROI_is_correct(self):
        # Arrange
        start = 125
        end = 10000
        inst = instruments.LOQ()
        inst.set_TOFs_for_ROI(start, end)

        # Act
        start_Tof, end_Tof = inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(start, start_Tof)
        self.assertEqual(end, end_Tof)

    def test_Tofs_for_ROI_are_reset_to_None(self):
        # Arrange
        start = 125
        end = 10000
        inst = instruments.LOQ()
        inst.set_TOFs_for_ROI(start, end)
        inst.reset_TOFs_for_ROI()

        # Act
        start_Tof, end_Tof = inst.get_TOFs_for_ROI()

        # Assert
        self.assertEqual(None, start_Tof)
        self.assertEqual(None, end_Tof)

if __name__ == "__main__":
    unittest.main()
