from __future__ import (absolute_import, division, print_function)
import os.path
import shutil

import mantid.simpleapi as mantid
from mantid import config
import stresstesting
import pearl_routines

DIFF_PLACES = 8
# initial directory to the system test data files
DIRS = config['datasearch.directories'].split(';')


class PearlPowderOldApiFmodeAll(stresstesting.MantidStressTest):
    fmode = "all"

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        _run_focus(self.fmode)

    def validate(self):
        return _validate_wrapper(self, self.fmode)

    def cleanup(self):
        _clean_up_files(DIRS)


class PearlPowderOldApiFmodeGroups(stresstesting.MantidStressTest):
    fmode = "groups"

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        _run_focus(self.fmode)

    def validate(self):
        return _validate_wrapper(self, self.fmode)

    def cleanup(self):
        _clean_up_files(DIRS)


class PearlPowderOldApiFmodeMods(stresstesting.MantidStressTest):
    fmode = "mods"

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        _run_focus(self.fmode)

    def validate(self):
        return _validate_wrapper(self, self.fmode)

    def cleanup(self):
        _clean_up_files(DIRS)


class PearlPowderOldApiFmodeTrans(stresstesting.MantidStressTest):
        fmode = "trans"

        def requiredFiles(self):
            return _gen_required_files()

        def runTest(self):
            _run_focus(self.fmode)

        def validate(self):
            return _validate_wrapper(self, self.fmode)

        def cleanup(self):
            _clean_up_files(DIRS, tof_xye_file_saved=True)


# Class common implementation
def _clean_up_files(directories, tof_xye_file_saved=False):
        filenames = ["PEARL/Focus_Test/DataOut/PEARL92476_92479.nxs",
                     "PEARL/Focus_Test/DataOut/PEARL92476_92479-0.gss"]

        if tof_xye_file_saved:
            filenames.extend(["PEARL/Focus_Test/DataOut/PEARL92476_92479_tof_xye-0.dat",
                             "PEARL/Focus_Test/DataOut/PEARL92476_92479_d_xye-0.dat"])

        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
            cali_path = os.path.join(directories[0], "PEARL/Focus_Test/DataOut")
            shutil.rmtree(cali_path)
        except OSError as ose:
            print ('could not delete the generated file: ', ose.filename)


def _gen_required_files():
    filenames = []

    # existing calibration files
    filenames.extend(('PEARL/Focus_Test/Calibration/pearl_group_12_1_TT70.cal',
                      'PEARL/Focus_Test/Calibration/pearl_offset_15_3.cal',
                      'PEARL/Focus_Test/Calibration/van_spline_TT70_cycle_15_4.nxs',
                      'PEARL/Focus_Test/Attentuation/PRL112_DC25_10MM_FF.OUT',
                      # Input files
                      'PEARL/Focus_Test/RawFiles/PEARL00092476.raw',
                      'PEARL/Focus_Test/RawFiles/PEARL00092477.raw',
                      'PEARL/Focus_Test/RawFiles/PEARL00092478.raw',
                      'PEARL/Focus_Test/RawFiles/PEARL00092479.raw'
                      ))
    # raw files / run numbers 92476-92479
    for i in range(6, 10):
        filenames.append('PEARL/Focus_Test/RawFiles/PEARL0009247' + str(i))
    # reference files

    return filenames


def _run_focus(focus_mode):
    # code to run the script starts here
    pearl_routines.PEARL_startup(usern="Mantid_Tester", thiscycle="15_4")

    # setting files directory here
    # DIRS[0] is the system test directory

    # setting raw files directory
    raw_path = os.path.join(DIRS[0], "PEARL/Focus_Test/RawFiles/")
    pearl_routines.pearl_set_currentdatadir(raw_path)
    pearl_routines.PEARL_setdatadir(raw_path)

    # setting calibration files directory
    cali_path = os.path.join(DIRS[0], "PEARL/Focus_Test/Calibration/")
    pearl_routines.pearl_initial_dir(cali_path)
    atten_path = os.path.join(DIRS[0], "PEARL/Focus_Test/Attentuation/PRL112_DC25_10MM_FF.OUT")
    pearl_routines.PEARL_setattenfile(atten_path)

    # setting data output folder
    data_out_path = os.path.join(DIRS[0], "PEARL/Focus_Test/DataOut/")
    pearl_routines.pearl_set_userdataoutput_dir(data_out_path)

    # run the script by calling PEARL_focus function
    ws_list = pearl_routines.PEARL_focus('92476_92479', 'raw', fmode=focus_mode, ttmode='TT70',
                                         atten=True, van_norm=True, debug=True)

    # We want to be sure that the nxs file loaded back in is the one used for reference
    for ws in ws_list:
        mantid.DeleteWorkspace(Workspace=ws)
        del ws


def _validate_wrapper(cls, focus_mode):
    cls.disableChecking.append('Instrument')
    cls.disableChecking.append('Sample')
    cls.disableChecking.append('SpectraMap')
    out_name = "PEARL_routines_fmode_" + focus_mode
    mantid.LoadNexus(Filename=DIRS[0] + "PEARL/Focus_Test/DataOut/15_4/Mantid_Tester/PEARL92476_92479.nxs",
                     OutputWorkspace=out_name)
    reference_file_name = "ISIS_Powder-PEARL92476_92479_" + focus_mode + ".nxs"
    return out_name, reference_file_name
