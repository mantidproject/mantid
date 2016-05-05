# pylint: disable= attribute-defined-outside-init, no-init

import unittest
import os.path
import shutil

from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.simpleapi import *
from mantid import config
import stresstesting
import pearl_routines

DIFF_PLACES = 8
# initial directory to the system test data files
DIRS = config['datasearch.directories'].split(';')


class PearlPowderDiffractionScriptTest(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []

        # existing calibration files
        filenames.extend(('PEARL/Calibration/pearl_group_12_1_TT70.cal',
                          'PEARL/Calibration/pearl_offset_15_3.cal',
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
            cali_path = os.path.join(directories[0], "PEARL/DataOut")
            shutil.rmtree(cali_path)
        except OSError, ose:
            print 'could not delete the generated file: ', ose.filename

    def runTest(self):
        self._success = False

        # code to run the script starts here
        pearl_routines.PEARL_startup(usern="Mantid_Tester", thiscycle="15_4")

        # setting files directory here
        # DIRS[0] is the system test directory

        # setting raw files directory
        raw_path = os.path.join(DIRS[0], "PEARL/RawFiles/")
        pearl_routines.pearl_set_currentdatadir(raw_path)
        pearl_routines.PEARL_setdatadir(raw_path)

        # setting calibration files directory
        cali_path = os.path.join(DIRS[0], "PEARL/Calibration/")
        pearl_routines.pearl_initial_dir(cali_path)
        atten_path = os.path.join(DIRS[0], "PEARL/Attentuation/PRL112_DC25_10MM_FF.OUT")
        pearl_routines.PEARL_setattenfile(atten_path)

        # setting data output folder
        data_out_path = os.path.join(DIRS[0], "PEARL/DataOut/")
        pearl_routines.pearl_set_userdataoutput_dir(data_out_path)

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
        filenames.extend(("PEARL/DataOut/PRL92476_92479.nxs",
                          "PEARL/DataOut/PRL92476_92479_d_xye-0.dat",
                          "PEARL/DataOut/PRL92476_92479_tof_xye-0.dat",
                          "PEARL/DataOut/PRL92476_92479-0.gss"))
        self._clean_up_files(filenames, DIRS)


# ======================================================================
# pylint: disable = too-many-public-methods
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

    def test_vanadium_workspace(self):
        for i in range(1, 15):
            van_file = mtd["van" + str(i)]
            self.assertTrue(isinstance(van_file, MatrixWorkspace))
            self.assertEquals(1, van_file.getNumberHistograms())
            self.assertEquals(4310, van_file.blocksize())

        # check random values, places variables doesnt seem to work when
        van1 = mtd["van1"]
        self.assertAlmostEqual(1690.17031579, van1.readX(0)[199], places=DIFF_PLACES)
        van4 = mtd["van4"]
        self.assertAlmostEqual(6.5605707095e-06, van4.readY(0)[4000], places=DIFF_PLACES)
        van7 = mtd["van7"]
        self.assertAlmostEquals(1.03406081396e-08, van7.readE(0)[50], places=DIFF_PLACES)
        van10 = mtd["van10"]
        self.assertAlmostEquals(11667.996525642744, van10.readX(0)[3420], places=DIFF_PLACES)
        van14 = mtd["van14"]
        self.assertAlmostEqual(3.39159292684e-05, van14.readY(0)[187], places=DIFF_PLACES)

    def test_rdata_workspace(self):
        expected_title = "glucose+Pb 6 tns in ZTA anvils"
        for i in range(1, 15, 3):
            rfile = mtd["rdata" + str(i)]
            self.assertTrue(isinstance(rfile, MatrixWorkspace))
            self.assertEquals(1, rfile.getNumberHistograms())
            self.assertEquals(4310, rfile.blocksize())
            # Not using assertIn - not supported on rhel6 (python 2.6)
            self.assertTrue(expected_title in rfile.getTitle())
            van_file = mtd["van" + str(i)]
            self.assertAlmostEquals(van_file.readX(0)[i], rfile.readX(0)[i], places=DIFF_PLACES)

        focus_file = mtd["focus"]
        self.assertTrue(isinstance(focus_file, MatrixWorkspace))
        self.assertEquals(14, focus_file.getNumberHistograms())
        self.assertEquals(4310, focus_file.blocksize())
        self.assertTrue(expected_title in focus_file.getTitle())

    def test_mod_files(self):
        mod_group_table = mtd["PRL92476_92479"]
        self.assertTrue("PRL92476_92479_mods1-9_1" in mod_group_table[0].getName())

        for i in range(1, 10):
            self.assertTrue("PRL92476_92479_mod" + str(i) in mod_group_table[i].getName())
            self.assertEquals(4310, mod_group_table[i].blocksize())
            ind_mod_file = mtd["PRL92476_92479_mod" + str(i)]
            self.assertAlmostEqual(mod_group_table[i].readY(0)[50],
                                   ind_mod_file.readY(0)[50], places=DIFF_PLACES)

        no_atten = mtd["PRL92476_92479_noatten"]
        self.assertAlmostEqual(1.06842521, no_atten.readY(0)[17], places=DIFF_PLACES)
        self.assertAlmostEqual(1.52553392, no_atten.readX(0)[2663], places=DIFF_PLACES)
        self.assertAlmostEqual(0.01567764, no_atten.readE(0)[577], places=DIFF_PLACES)
        self.assertAlmostEqual(3.61208767, no_atten.readX(0)[4100], places=DIFF_PLACES)
        self.assertAlmostEqual(0.94553930, no_atten.readY(0)[356], places=DIFF_PLACES)

    def test_saved_data_files(self):
        files_data = []

        wsname = "GSSFile"
        gss_file = os.path.join(DIRS[0], "PEARL/DataOut/PRL92476_92479-0.gss")
        files_data.append(LoadGSS(Filename=gss_file, OutputWorkspace=wsname))

        xye_dSpacing_ws = "xye_dSpacing"
        dSpacing_file = os.path.join(DIRS[0], "PEARL/DataOut/PRL92476_92479_d_xye-0.dat")
        files_data.append(Load(Filename=dSpacing_file, OutputWorkspace=xye_dSpacing_ws))

        xye_ToF_ws = "xye_ToF"
        ToF_file = os.path.join(DIRS[0], "PEARL/DataOut/PRL92476_92479_tof_xye-0.dat")
        files_data.append(Load(Filename=ToF_file, OutputWorkspace=xye_ToF_ws))

        for data in files_data:
            self.assertTrue(isinstance(data, MatrixWorkspace))
            self.assertEquals(1, data.getNumberHistograms())
            self.assertEquals(4310, data.blocksize())

        self.assertAlmostEqual(files_data[1].readY(0)[356], files_data[2].readY(0)[356],
                               places=DIFF_PLACES)
        self.assertAlmostEqual(files_data[1].readE(0)[943], files_data[2].readE(0)[943],
                               places=DIFF_PLACES)
        self.assertAlmostEqual(files_data[1].readY(0)[3293], files_data[2].readY(0)[3293],
                               places=DIFF_PLACES)
