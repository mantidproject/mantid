from __future__ import (absolute_import, division, print_function)
from mantid import *
import os
import unittest
import pearl_routines



class PearlRoutinesTest(unittest.TestCase):

    def test_inst_new2_exec(self):
        pearl_obj = pearl_routines.PearlStart(user_name="Test", calibration_dir="D:\\PEARL\\",
                                              raw_data_dir="D:\\PEARL\\", output_dir="D:\\PEARL\\output\\")
        pearl_routines.focus(74795, fmode='trans', ttmode='TT70', startup_object=pearl_obj)

    def xtest_inst_new2_old_api(self):
        # Tests the instrument with a "new2" data file using the old API call
        pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
        pearl_routines.PEARL_focus(74795, fmode='trans', ttmode='TT70')

    def xtest_inst_new_old_api(self):
        # Tests the instrument with a "new" data file using the old API call
        pearl_routines.PEARL_startup('Mantid_Developer', '15_4')
        pearl_routines.PEARL_focus(73987, fmode='trans', ttmode='TT70')

    def xtest_create_group_old_api(self):
        # Checks testvan executes correctly
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        pearl_routines.pearl_set_currentdatadir(DataDir)
        pearl_routines.PEARL_setdatadir(DataDir)

        # create calibration folder to process calibration files too
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        # setting calibration files directory
        pearl_routines.pearl_initial_dir(CalibDir)
        vanFile35 = os.path.join(CalibDir, 'van_spline_TT35_cycle_15_3.nxs')
        ngrpfile = os.path.join(CalibDir, 'test_cal_group_15_3.cal')
        # TODO fix the old API
        pearl_routines.PEARL_creategroup("91560_91561", ngroupfile=ngrpfile)

        offsetfile = os.path.join(CalibDir, 'pearl_offset_15_3.cal')

        pearl_routines.PEARL_createcal("91560_91561", noffsetfile=offsetfile, groupfile=ngrpfile)
        pearl_routines.PEARL_createvan("91530_91531", "91550_91551", ttmode="TT35",
                                       nvanfile=vanFile35, nspline=40, absorb=True, debug=True)

    def test_create_van(self):
        # Checks testvan executes correctly
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')

        pearl_obj = pearl_routines.PearlStart(user_name="Test", calibration_dir=CalibDir,
                                              raw_data_dir=DataDir, output_dir="D:\\PEARL\\output\\")

        vanFile35 = os.path.join(CalibDir, 'van_spline_TT35_cycle_15_3.nxs')
        ngrpfile = os.path.join(CalibDir, 'test_cal_group_15_3.cal')

        pearl_routines.create_group("91560_91561", startup_objects=pearl_obj, ngroupfile=ngrpfile)

        offsetfile = os.path.join(CalibDir, 'pearl_offset_15_3.cal')

        pearl_routines.create_calibration(startup_object=pearl_obj, calibration_runs="91560_91561",
                                          offset_file_path=offsetfile, grouping_file_path=ngrpfile)
        pearl_routines.create_vanadium(startup_object=pearl_obj, vanadium_runs="91530_91531", empty_runs="91550_91551",
                                       tt_mode="TT35", output_file_name=vanFile35, num_of_spline_coefficients=40,
                                       do_absorp_corrections=True)

    def test_instrument_ranges_calcs_correctly(self):
        # This test checks that the instrument ranges calculate correctly for given instruments
        # First the "new" instrument value
        new_alg_range, new_save_range = pearl_routines._setup_focus_for_inst("new")
        self.assertEquals(new_alg_range, 12, "'new' instrument algorithm range got " + str(new_alg_range))
        self.assertEquals(new_save_range, 3, "'new' instrument save range got " + str(new_save_range))

        new2_alg_range, new2_save_range = pearl_routines._setup_focus_for_inst("new2")
        self.assertEquals(new2_alg_range, 14, "'new2' instrument algorithm range got " + str(new2_alg_range))
        self.assertEquals(new2_save_range, 5, "'new2' instrument save range got " + str(new2_save_range))


    def xtest_numeric_file_names_gen_correct(self):
        # This test checks the generated names for files are correct
        # when the run number is purely numeric
        pearl_routines._generate_out_file_names(int(12345))
        # TODO when userdata processed is moved out of global


if __name__ == '__main__':
    DIRS = config['datasearch.directories'].split(';')
    CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
    unittest.main()