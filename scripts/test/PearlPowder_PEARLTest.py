
import os
import unittest

import mantid as mantid

import PearlPowder_common
import PearlPowder_PEARL


class PearlPowder_PEARLTest(unittest.TestCase):

    def test_inst_new2_exec_fmode_all(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        pearl_obj.focus(run_number=74795, focus_mode="all")

    def test_inst_new2_exec_fmode_groups(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        pearl_obj.focus(74795, focus_mode='groups')

    def test_inst_new2_exec_fmode_trans(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        pearl_obj.focus(74795, focus_mode="trans")

    def test_inst_new2_exec_fmode_mods(self):
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir="D:\\PEARL\\",
                                            raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        pearl_obj.focus(74795,focus_mode="mods")

    def test_instrument_ranges_calcs_correctly(self):
        # This test checks that the instrument ranges calculate correctly for given instruments
        # First the "new" instrument value
        new_alg_range, new_save_range = PearlPowder_PEARL._get_instrument_ranges("new")
        self.assertEquals(new_alg_range, 12, "'new' instrument algorithm range got " + str(new_alg_range))
        self.assertEquals(new_save_range, 3, "'new' instrument save range got " + str(new_save_range))

        new2_alg_range, new2_save_range = PearlPowder_PEARL._get_instrument_ranges("new2")
        self.assertEquals(new2_alg_range, 14, "'new2' instrument algorithm range got " + str(new2_alg_range))
        self.assertEquals(new2_save_range, 5, "'new2' instrument save range got " + str(new2_save_range))

    def test_create_van_absorb_corr_load(self):
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")
        vanFile35 = 'van_spline_TT35_cycle_15_3.nxs'
        pearl_obj.create_calibration_vanadium(vanadium_runs="91530_91531",
                                              empty_runs="91550_91551", output_file_name=vanFile35,
                                              num_of_splines=40, do_absorb_corrections=True, gen_absorb_correction=False)

    def xtest_create_van_absorb_corr_gen(self):
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")
        vanFile35 = 'van_spline_TT35_cycle_15_3.nxs'
        pearl_obj.create_calibration_vanadium(vanadium_runs="91530_91531",
                                              empty_runs="91550_91551", output_file_name=vanFile35,
                                              num_of_splines=40, do_absorb_corrections=True, gen_absorb_correction=True)

    def test_create_van_no_absorb_corr(self):
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")
        vanFile35 = 'van_spline_TT35_cycle_15_3.nxs'
        pearl_obj.create_calibration_vanadium(vanadium_runs="91530_91531",
                                              empty_runs="91550_91551", output_file_name=vanFile35,
                                              num_of_splines=40, do_absorb_corrections=False)

    def test_create_cal_by_name(self):
        # Checks testvan executes correctly
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')

        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")

        ngrpfile = 'test_cal_group_15_3.cal'

        pearl_obj.create_empty_calibration_by_names("91560_91561", output_file_name=ngrpfile)

    def test_create_cal(self):
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        pearl_obj = PearlPowder_PEARL.Pearl(user_name="Test", calibration_dir=CalibDir,
                                            raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")

        offsetfile = 'pearl_offset_15_3.cal'
        ngrpfile = 'test_cal_group_15_3.cal'
        PearlPowder_common.create_calibration(startup_object=pearl_obj, calibration_runs="91560_91561",
                                              offset_file_path=offsetfile, grouping_file_name=ngrpfile)

if __name__ == '__main__':
    DIRS = mantid.config['datasearch.directories'].split(';')
    CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
    unittest.main()