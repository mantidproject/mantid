import unittest
import os.pathimport

from mantid.simpleapi import *
from mantid import config
import stresstesting
import pearl_routines
from mantid.Framework.PythonInterface.mantid.api import AnalysisDataService  # is that already being imported?

# initial directory to the system test data files
DIRS = config['datasearch.directories'].split(';')


class PearlPowderDiffractionScriptTest(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []

        # existing calibration files
        filenames.extend(('PEARL/Calibration/pearl_group_12_1_TT70.cal', 'PEARL/Calibration/pearl_offset_15_3.cal',
                          'PEARL/Calibration/van_spline_TT70_cycle_15_4.nxs',
                          'PEARL/Attentuation/PRL112_DC25_10MM_FF.OUT'))
        # raw files / run numbers 92476-92479
        for i in range(6, 10):
            filenames.append('PEARL/RawFiles/PEARL0009247' + str(i))

        return filenames

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError, ose:
            print 'could not delete the generated file: ', ose.filename

    def runTest(self):
        self._success = False

        # code to run the script starts here
        pearl_routines.PEARL_startup(usern="Mantid_Tester", thiscycle="15_4")

        # setting files directory here
        # DIRS[0] is the system test directory

        # setting raw files directory
        current_directory = pearl_routines.pearl_set_currentdatadir(DIRS[0] + "PEARL\\RawFiles\\")
        pearl_routines.PEARL_setdatadir(current_directory)

        # setting calibration files directory
        pearl_routines.pearl_initial_dir(DIRS[0] + "\\PEARL\\")
        pearl_routines.PEARL_setattenfile(DIRS[0] + "\\PEARL\\Attentuation\PRL112_DC25_10MM_FF.OUT")

        # setting data output folder
        pearl_routines.pearl_set_userdataoutput_dir(DIRS[0] + '\\PEARL\\DataOut\\')

        # run the script by calling PEARL_focus function
        pearl_routines.PEARL_focus('92476_92479', 'raw', fmode='trans', ttmode='TT70',
                                   atten=True, van_norm=True, debug=True)

        # Custom code to create and run this single test suite
        # and then mark as success or a failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadTests, "test"))
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success

    def cleanup(self):
        filenames = []
        filenames.extend(("PRL92476_92479.nxs", "PRL92476_92479_d_xye-0.dat", "PRL92476_92479_tof_xye-0.dat",
                          "PRL92476_92479-0.gss"))
        self._clean_up_files(filenames, DIRS)


# ======================================================================
# work horse
class LoadTests(unittest.TestCase):
    wsname = "__LoadTest"
    cleanup_names = []

    def tearDown(self):
        self.cleanup_names.append(self.wsname)
        for name in self.cleanup_names:
            try:
                AnalysisDataService.remove(name)
            except KeyError:
                pass
        self.cleanup_names = []

        # ============================ Success =============================
