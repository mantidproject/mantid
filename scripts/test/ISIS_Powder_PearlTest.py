
import os
import unittest

import mantid as mantid
from isis_powder import pearl


class isis_powder_PearlTest(unittest.TestCase):

    # Static method tests

    def test_cycle_information_generates_correctly(self):
        # This checks that the cycle information generates using the correct keys for the dict
        output = self._get_pearl_inst_defaults()._get_cycle_information(85500)
        expected_cycle = "14_1"
        expected_inst_vers = "new2"
        self.assertEquals(output["cycle"], expected_cycle)
        self.assertEquals(output["instrument_version"], expected_inst_vers)

    def test_get_instrument_ranges(self):
        # This test checks that the instrument ranges calculate correctly for given instruments
        # First the "new" instrument value
        new_alg_range, new_save_range = pearl._get_instrument_ranges("new")
        self.assertEquals(new_alg_range, 12, "'new' instrument algorithm range got " + str(new_alg_range))
        self.assertEquals(new_save_range, 3, "'new' instrument save range got " + str(new_save_range))

        new2_alg_range, new2_save_range = pearl._get_instrument_ranges("new2")
        self.assertEquals(new2_alg_range, 14, "'new2' instrument algorithm range got " + str(new2_alg_range))
        self.assertEquals(new2_save_range, 5, "'new2' instrument save range got " + str(new2_save_range))

    def test_generate_inst_file_name(self):
        # Tests that the generated names conform to the format expected
        old_name_input = 71008  # This is the last run to use the old format
        new_name_input = 71009  # Everything after this run should use new format

        expected_old_name = "PRL" + str(old_name_input)
        expected_new_name = "PEARL" + "000" + str(new_name_input)  # New names should have 8 numerical digits

        old_output = pearl._gen_file_name(old_name_input)
        self.assertEquals(expected_old_name, old_output)

        new_output = pearl._gen_file_name(new_name_input)
        self.assertEquals(expected_new_name, new_output)

    # Non static methods
    def test_get_calibration_full_paths(self):
        input_cycle = "15_4"
        input_tt_mode = self.default_tt_mode

        expected_calfile = "pearl_offset_15_3.cal"
        expected_van_aborb_file = "pearl_absorp_sphere_10mm_newinst2_long.nxs"
        expected_grouping_file = "pearl_group_12_1_TT88.cal"
        expected_vanadium_file = "van_spline_TT88_cycle_15_4.nxs"

        expected_calibration_dir = self.calibration_dir

        pearl_obj = self._get_pearl_inst_defaults()
        output = pearl_obj._get_calibration_full_paths(input_cycle)

        self.assertEquals(output["calibration"], expected_calibration_dir + expected_calfile)
        self.assertEquals(output["grouping"], expected_calibration_dir + expected_grouping_file)
        self.assertEquals(output["vanadium_absorption"], expected_calibration_dir + expected_van_aborb_file)
        self.assertEquals(output["vanadium"], expected_calibration_dir + expected_vanadium_file)

    def _get_pearl_inst_defaults(self):
        return pearl.Pearl(user_name="unitTest-PEARL", calibration_dir=self.calibration_dir,
                                       raw_data_dir=self.raw_data_dir, output_dir=self.output_dir)

    def _get_pearl_inst_all_specified(self):
        return pearl.Pearl(calibration_dir=self.calibration_dir, raw_data_dir=self.raw_data_dir,
                           output_dir=self.output_dir, input_file_ext=self.default_ext, tt_mode=self.tt_mode)

    # Test params
    calibration_dir = "calDir"
    raw_data_dir = "rawDir"
    output_dir = "outDir"

    default_tt_mode = "TT88"

    # Optional Params
    default_ext = ".ext"
    tt_mode = "tt_test"


if __name__ == '__main__':
    DIRS = mantid.config['datasearch.directories'].split(';')
    CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
    unittest.main()