import unittest

import mantid as mantid

import PearlPowder_common as Common


class PearlRoutinesTest(unittest.TestCase):

    def test_generate_cycle(self):
        cycle_number = 123
        str_cycle_numer = str(cycle_number)
        Windows_style_path = "C:\\test\\"
        Linux_style_path = "~/test/"

        Win_result = Common._generate_cycle_dir(raw_data_dir=Windows_style_path, run_cycle=cycle_number)
        Linux_result = Common._generate_cycle_dir(raw_data_dir=Linux_style_path, run_cycle=cycle_number)

        self.assertEquals(Win_result, Windows_style_path + str_cycle_numer + '\\')
        self.assertEquals(Linux_result, Linux_style_path + str_cycle_numer + '/')

        # Next check it throws if the end of a path is unclosed
        bad_Windows_path = "C:\\test"
        bad_Linux_path = "~/test"

        self.assertRaises(ValueError, lambda: Common._generate_cycle_dir(raw_data_dir=bad_Windows_path,
                                                                         run_cycle=cycle_number))
        self.assertRaises(ValueError, lambda: Common._generate_cycle_dir(raw_data_dir=bad_Linux_path,
                                                                         run_cycle=cycle_number))




if __name__ == '__main__':
    unittest.main()