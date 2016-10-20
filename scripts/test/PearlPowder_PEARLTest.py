
import os
import unittest

import mantid as mantid

import PearlPowder_common
import PearlPowder_PEARL


class PearlRoutinesTest(unittest.TestCase):

    def xtest_inst_new2_exec_fmode_all(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        PearlPowder_common.focus(74795, fmode='all', ttmode='TT70', startup_object=pearl_obj)

    def test_inst_new2_exec_fmode_groups(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        PearlPowder_common.focus(74795, fmode='groups', ttmode='TT70', startup_object=pearl_obj)

    def test_instrument_ranges_calcs_correctly(self):
        # This test checks that the instrument ranges calculate correctly for given instruments
        # First the "new" instrument value
        new_alg_range, new_save_range = PearlPowder_PEARL._get_instrument_ranges("new")
        self.assertEquals(new_alg_range, 12, "'new' instrument algorithm range got " + str(new_alg_range))
        self.assertEquals(new_save_range, 3, "'new' instrument save range got " + str(new_save_range))

        new2_alg_range, new2_save_range = PearlPowder_PEARL._get_instrument_ranges("new2")
        self.assertEquals(new2_alg_range, 14, "'new2' instrument algorithm range got " + str(new2_alg_range))
        self.assertEquals(new2_save_range, 5, "'new2' instrument save range got " + str(new2_save_range))

    def xtest_create_van(self):
        # Checks testvan executes correctly
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')

        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")

        vanFile35 = os.path.join(CalibDir, 'van_spline_TT35_cycle_15_3.nxs')
        ngrpfile = os.path.join(CalibDir, 'test_cal_group_15_3.cal')

        PearlPowder_common.create_calibration_by_names("91560_91561", startup_objects=pearl_obj, ngroupfile=ngrpfile)

        offsetfile = os.path.join(CalibDir, 'pearl_offset_15_3.cal')

        PearlPowder_common.create_calibration(startup_object=pearl_obj, calibration_runs="91560_91561",
                                              offset_file_path=offsetfile, grouping_file_path=ngrpfile)

        PearlPowder_common.create_vanadium(startup_object=pearl_obj, vanadium_runs="91530_91531",
                                           empty_runs="91550_91551", tt_mode="TT35", output_file_name=vanFile35,
                                           num_of_spline_coefficients=40, do_absorp_corrections=True)


if __name__ == '__main__':
    DIRS = mantid.config['datasearch.directories'].split(';')
    CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
    unittest.main()