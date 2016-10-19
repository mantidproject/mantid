import unittest

import mantid as mantid

import PearlPowder_common



# This file should ONLY test common logic and not the flow of the program

class PearlRoutinesTest(unittest.TestCase):
    def test_instrument_ranges_calcs_correctly(self):
        # This test checks that the instrument ranges calculate correctly for given instruments
        # First the "new" instrument value
        new_alg_range, new_save_range = PearlPowder_common._get_instrument_ranges("new")
        self.assertEquals(new_alg_range, 12, "'new' instrument algorithm range got " + str(new_alg_range))
        self.assertEquals(new_save_range, 3, "'new' instrument save range got " + str(new_save_range))

        new2_alg_range, new2_save_range = PearlPowder_common._get_instrument_ranges("new2")
        self.assertEquals(new2_alg_range, 14, "'new2' instrument algorithm range got " + str(new2_alg_range))
        self.assertEquals(new2_save_range, 5, "'new2' instrument save range got " + str(new2_save_range))

if __name__ == '__main__':
    unittest.main()