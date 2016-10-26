# pylint: disable= attribute-defined-outside-init, no-init

import unittest
import os.path
import shutil

from mantid.api import AnalysisDataService, MatrixWorkspace, ITableWorkspace
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
        filenames.extend(('PEARL/Focus_Test/Calibration/pearl_group_12_1_TT70.cal',
                          'PEARL/Focus_Test/Calibration/pearl_offset_15_3.cal',
                          'PEARL/Focus_Test/Calibration/van_spline_TT70_cycle_15_4.nxs',
                          'PEARL/Focus_Test/Attentuation/PRL112_DC25_10MM_FF.OUT',
                          # reference files
                          'PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479.nxs',
                          'PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479-0.gss',
                          'PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479_d_xye-0.dat',
                          'PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479_tof_xye-0.dat'
                          ))
        # raw files / run numbers 92476-92479
        for i in range(6, 10):
            filenames.append('PEARL/Focus_Test/RawFiles/PEARL0009247' + str(i))
        # reference files

        return filenames

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
            cali_path = os.path.join(directories[0], "PEARL/Focus_Test/DataOut")
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
        pearl_routines.PEARL_focus('92476_92479', 'raw', fmode='all', ttmode='TT70',
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
        filenames.extend(("PEARL/Focus_Test/DataOut/PEARL92476_92479.nxs",
                          "PEARL/Focus_Test/DataOut/PEARL92476_92479_d_xye-0.dat",
                          "PEARL/Focus_Test/DataOut/PEARL92476_92479_tof_xye-0.dat",
                          "PEARL/Focus_Test/DataOut/PEARL92476_92479-0.gss"))
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
        import pydevd
        pydevd.settrace('localhost', port=23000, stdoutToServer=True, stderrToServer=True)
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
        mod_group_table = mtd["PEARL92476_92479"]
        self.assertTrue("PEARL92476_92479_mods1-9_1" in mod_group_table[0].getName())

        for i in range(1, 10):
            self.assertTrue("PEARL92476_92479_mod" + str(i) in mod_group_table[i].getName())
            self.assertEquals(4310, mod_group_table[i].blocksize())
            ind_mod_file = mtd["PEARL92476_92479_mod" + str(i)]
            self.assertAlmostEqual(mod_group_table[i].readY(0)[50],
                                   ind_mod_file.readY(0)[50], places=DIFF_PLACES)

        no_atten = mtd["PEARL92476_92479_noatten"]
        self.assertAlmostEqual(1.06842521, no_atten.readY(0)[17], places=DIFF_PLACES)
        self.assertAlmostEqual(1.52553392, no_atten.readX(0)[2663], places=DIFF_PLACES)
        self.assertAlmostEqual(0.01567764, no_atten.readE(0)[577], places=DIFF_PLACES)
        self.assertAlmostEqual(3.61208767, no_atten.readX(0)[4100], places=DIFF_PLACES)
        self.assertAlmostEqual(0.94553930, no_atten.readY(0)[356], places=DIFF_PLACES)

    def test_saved_data_files(self):
        files_data = []

        wsname = "GSSFile"
        gss_file = os.path.join(DIRS[0], "PEARL/Focus_Test/DataOut/PEARL92476_92479-0.gss")
        files_data.append(LoadGSS(Filename=gss_file, OutputWorkspace=wsname))

        xye_dSpacing_ws = "xye_dSpacing"
        dSpacing_file = os.path.join(DIRS[0], "PEARL/Focus_Test/DataOut/PEARL92476_92479_d_xye-0.dat")
        files_data.append(Load(Filename=dSpacing_file, OutputWorkspace=xye_dSpacing_ws))

        xye_ToF_ws = "xye_ToF"
        ToF_file = os.path.join(DIRS[0], "PEARL/Focus_Test/DataOut/PEARL92476_92479_tof_xye-0.dat")
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

    def test_nexus_reference_files(self):
        nexus_ws = "Ref_PRL92476_92479"
        nxs_file = os.path.join(DIRS[0], "PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479.nxs")
        Load(Filename=nxs_file, OutputWorkspace=nexus_ws)

        mod_group_table = mtd["PEARL92476_92479"]
        return nexus_ws, mod_group_table

    def test_gss_referenece_files(self):
        gss_wsname = "GSSFile"
        gss_file = os.path.join(DIRS[0], "PEARL/Focus_Test/DataOut/PEARL92476_92479-0.gss")
        LoadGSS(Filename=gss_file, OutputWorkspace=gss_wsname)

        ref_gss_wsname = "Ref_GSSFile"
        ref_gss_file = os.path.join(DIRS[0], "PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479-0.gss")
        LoadGSS(Filename=ref_gss_file, OutputWorkspace=ref_gss_wsname)

        return gss_wsname, ref_gss_wsname

    def test_xye_d_reference_files(self):
        xye_dspacing_wsname = mtd["PEARL92476_92479_mods1-9"]

        ref_xye_dspacing_wsname = "Ref_PRL92476_92479_mods1-9_D"
        xye_dSpacing_file = os.path.join(DIRS[0], "PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479_d_xye-0.dat")
        Load(Filename=xye_dSpacing_file, OutputWorkspace=ref_xye_dspacing_wsname)

        return xye_dspacing_wsname, ref_xye_dspacing_wsname

    def test_xye_tof_reference_files(self):
        xye_tof_wsname = mtd["PEARL92476_92479_mods1-9"]

        ref_xye_tof_wsname = "Ref_PRL92476_92479_mods1-9_TOF"
        xye_tof_file = os.path.join(DIRS[0], "PEARL/Focus_Test/RefFiles/Ref_PRL92476_92479_tof_xye-0.dat")
        Load(Filename=xye_tof_file, OutputWorkspace=ref_xye_tof_wsname)

        return xye_tof_wsname, ref_xye_tof_wsname


class PearlPowderDiffractionScriptTestCalibration(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []

        # existing calibration files
        filenames.extend(('PEARL/Calibration_Test/Calibration/pearl_absorp_sphere_10mm_newinst2_long.nxs',
                          'PEARL/Calibration_Test/Calibration/pearl_group_12_1_TT35.cal',
                          'PEARL/Calibration_Test/Calibration/pearl_group_12_1_TT70.cal',
                          'PEARL/Calibration_Test/Calibration/pearl_group_12_1_TT88.cal',
                          # reference files
                          'PEARL/Calibration_Test/RefFiles/van_spline_TT35_cycle_15_3.nxs',
                          'PEARL/Calibration_Test/RefFiles/van_spline_TT70_cycle_15_3.nxs',
                          'PEARL/Calibration_Test/RefFiles/van_spline_TT88_cycle_15_3.nxs',

                          ))

        # raw files / run numbers 92476-92479
        for i in range(0, 2):
            filenames.append('PEARL/Calibration_Test/RawFiles/PEARL0009156' + str(i))
        for i in range(0, 2):
            filenames.append('PEARL/Calibration_Test/RawFiles/PEARL0009153' + str(i))
        for i in range(0, 2):
            filenames.append('PEARL/Calibration_Test/RawFiles/PEARL0009155' + str(i))

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

        pearl_routines.PEARL_startup("Calib", "15_3")

        # setting raw files directory
        DataDir = os.path.join(DIRS[0], 'PEARL/Calibration_Test/RawFiles/')
        pearl_routines.pearl_set_currentdatadir(DataDir)
        pearl_routines.PEARL_setdatadir(DataDir)

        # create calibration folder to process calibration files too
        CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
        # setting calibration files directory
        pearl_routines.pearl_initial_dir(CalibDir)

        # creating group cal file
        ngrpfile = os.path.join(CalibDir, 'test_cal_group_15_3.cal')
        pearl_routines.PEARL_creategroup("91560_91561", ngroupfile=ngrpfile)

        # create offset files
        offsetfile = os.path.join(CalibDir, 'pearl_offset_15_3.cal')
        pearl_routines.PEARL_createcal("91560_91561", noffsetfile=offsetfile, groupfile=ngrpfile)

        # Creates the vanadium file for a cycle, where the first set of runs are the vanadium and
        # the second are the background in each case.
        vanFile35 = os.path.join(CalibDir, 'van_spline_TT35_cycle_15_3.nxs')
        pearl_routines.PEARL_createvan("91530_91531", "91550_91551", ttmode="TT35",
                                       nvanfile=vanFile35, nspline=40, absorb=True, debug=True)
        CloneWorkspace(InputWorkspace="Van_data", OutputWorkspace="van_tt35")

        vanFile70 = os.path.join(CalibDir, 'van_spline_TT70_cycle_15_3.nxs')
        pearl_routines.PEARL_createvan("91530_91531", "91550_91551", ttmode="TT70",
                                       nvanfile=vanFile70, nspline=40, absorb=True, debug=True)
        CloneWorkspace(InputWorkspace="Van_data", OutputWorkspace="van_tt70")

        vanFile88 = os.path.join(CalibDir, 'van_spline_TT88_cycle_15_3.nxs')
        pearl_routines.PEARL_createvan("91530_91531", "91550_91551", ttmode="TT88",
                                       nvanfile=vanFile88, nspline=40, absorb=True, debug=True)
        CloneWorkspace(InputWorkspace="Van_data", OutputWorkspace="van_tt88")

        # Custom code to create and run this single test suite
        # and then mark as success or a failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadCalibTests, "test"))
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
        filenames.extend(('PEARL/Calibration_Test/Calibration/pearl_offset_15_3.cal',
                          'PEARL/Calibration_Test/Calibration/test_cal_group_15_3.cal',
                          'PEARL/Calibration_Test/Calibration/van_spline_TT35_cycle_15_3.nxs',
                          'PEARL/Calibration_Test/Calibration/van_spline_TT70_cycle_15_3.nxs',
                          'PEARL/Calibration_Test/Calibration/van_spline_TT88_cycle_15_3.nxs'))
        self._clean_up_files(filenames, DIRS)


# ======================================================================
# work horse

class LoadCalibTests(unittest.TestCase):
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

    def test_calfile_with_workspace(self):

        cal_group_file = os.path.join(DIRS[0], 'PEARL/Calibration_Test/Calibration/test_cal_group_15_3.cal')
        cal_offset_file = os.path.join(DIRS[0], 'PEARL/Calibration_Test/Calibration/pearl_offset_15_3.cal')
        LoadCalFile(InstrumentName="PEARL", CalFilename=cal_group_file,
                    WorkspaceName='cal_group_15_3')
        LoadCalFile(InstrumentName="PEARL", CalFilename=cal_offset_file,
                    WorkspaceName="cal_offset_15_3")

        cal_workspaces = []
        for i in range(0, 2):
            cal_type = 'group'
            if i == 1:
                cal_type = 'offset'
            cal_workspaces.extend((mtd['cal_' + cal_type + '_15_3_group'],
                                   mtd['cal_' + cal_type + '_15_3_mask'],
                                   mtd['cal_' + cal_type + '_15_3_offsets']))

        for workspace in cal_workspaces:
            self.assertTrue(isinstance(workspace, MatrixWorkspace))
            self.assertEquals(1056, workspace.getNumberHistograms())
            self.assertEquals(1, workspace.blocksize())

        table_workspaces = []
        table_workspaces.extend((mtd['cal_group_15_3_cal'], mtd['cal_offset_15_3_cal']))
        for table in table_workspaces:
            self.assertTrue(isinstance(table, ITableWorkspace))

        for i in range(0, 1000, 50):
            self.assertEqual(table_workspaces[0].cell(i, 0), table_workspaces[0].cell(i, 0))

        diff_places = 2

        self.assertAlmostEqual(4491.84, table_workspaces[0].cell(0, 1), places=diff_places)
        self.assertAlmostEqual(4826.7, table_workspaces[1].cell(34, 1), places=diff_places)
        self.assertAlmostEqual(4814.51, table_workspaces[0].cell(190, 1), places=diff_places)
        self.assertAlmostEqual(4639.09, table_workspaces[1].cell(405, 1), places=diff_places)
        self.assertAlmostEqual(5219.48, table_workspaces[0].cell(703, 1), places=diff_places)
        self.assertAlmostEqual(3018.97, table_workspaces[1].cell(1055, 1), places=diff_places)

    def test_vanadium_tt_mode_files(self):

        vanadium_base_dir = (DIRS[0] + 'PEARL/Calibration_Test/Calibration/')
        tt_35_file = 'van_spline_TT35_cycle_15_3.nxs'
        vanadium_tt_35_dir = os.path.join(vanadium_base_dir, tt_35_file)
        van_tt35_data = LoadNexusProcessed(Filename=vanadium_tt_35_dir)
        self.matrix_workspaces_test(van_tt35_data, 'tt35')

        self.assertAlmostEquals(-5.2637392907314393e-06, van_tt35_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(1.46103398829e-05, van_tt35_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(5.20516790953e-06, van_tt35_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(2.30071459205e-08, van_tt35_data[4].readE(0)[17], places=DIFF_PLACES)
        self.assertAlmostEquals(177.218826007, van_tt35_data[13].readX(0)[278], places=DIFF_PLACES)

        tt_70_file = 'van_spline_TT70_cycle_15_3.nxs'
        vanadium_tt_70_dir = os.path.join(vanadium_base_dir, tt_70_file)
        van_tt70_data = LoadNexusProcessed(Filename=vanadium_tt_70_dir,
                                           OutputWorkspace=tt_70_file)
        self.matrix_workspaces_test(van_tt70_data, 'tt70')

        self.assertAlmostEquals(-9.63683468044e-06, van_tt70_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(3.05213907891e-05, van_tt70_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(1.05450348217e-05, van_tt70_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(3.2808780582e-08, van_tt70_data[4].readE(0)[17], places=DIFF_PLACES)
        self.assertAlmostEquals(177.218826007, van_tt70_data[13].readX(0)[278], places=DIFF_PLACES)

        tt_88_file = 'van_spline_TT88_cycle_15_3.nxs'
        vanadium_tt_88_dir = os.path.join(vanadium_base_dir, tt_88_file)
        van_tt88_data = LoadNexusProcessed(Filename=vanadium_tt_88_dir,
                                           OutputWorkspace=tt_88_file)
        self.matrix_workspaces_test(van_tt88_data, 'tt88')

        self.assertAlmostEquals(-1.20275831674e-05, van_tt88_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(3.64097322294e-05, van_tt88_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(1.28680957601e-05, van_tt88_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(2.6326912352e-08, van_tt88_data[4].readE(0)[17], places=DIFF_PLACES)
        self.assertAlmostEquals(177.218826007, van_tt88_data[13].readX(0)[278], places=DIFF_PLACES)

    def matrix_workspaces_test(self, vanadium_tt_file, tt_mode='tt35'):
        for i in range(1, 15):
            self.assertTrue(isinstance(vanadium_tt_file[i], MatrixWorkspace))
            self.assertTrue(isinstance(vanadium_tt_file[i], MatrixWorkspace))
            self.assertEquals(1, vanadium_tt_file[i].getNumberHistograms())
            self.assertEquals(8149, vanadium_tt_file[i].blocksize())

        mtd_tt_workspace = mtd['van_' + tt_mode]
        for i in range(1, 15):
            self.assertEqual(mtd_tt_workspace[i - 1].readY(0)[2], vanadium_tt_file[i].readY(0)[2])
            self.assertEqual(mtd_tt_workspace[i - 1].readY(0)[345], vanadium_tt_file[i].readY(0)[345])
            self.assertEqual(mtd_tt_workspace[i - 1].readY(0)[1343], vanadium_tt_file[i].readY(0)[1343])
            self.assertEqual(mtd_tt_workspace[i - 1].readY(0)[5984], vanadium_tt_file[i].readY(0)[5984])
            self.assertEqual(mtd_tt_workspace[i - 1].readY(0)[7654], vanadium_tt_file[i].readY(0)[7654])
            self.assertEqual(mtd_tt_workspace[i - 1].readX(0)[734], vanadium_tt_file[i].readX(0)[734])
            self.assertEqual(mtd_tt_workspace[i - 1].readE(0)[17], vanadium_tt_file[i].readE(0)[17])

    def test_ttmode_nexus_ref_files(self, tt_mode="35"):
        nexus_ws = mtd["van_tt" + tt_mode]

        ref_nexus_ws = "ref_van_tt" + tt_mode
        ref_nxs_file = os.path.join(DIRS[0], "PEARL/Calibration_Test/RefFiles/van_spline_TT" +
                                    tt_mode + "_cycle_15_3.nxs")
        Load(Filename=ref_nxs_file, OutputWorkspace=ref_nexus_ws)

        return nexus_ws, ref_nexus_ws

    def test_nexus_reference_files(self):
        # testing different tt mode files
        self.test_ttmode_nexus_ref_files("35")
        self.test_ttmode_nexus_ref_files("70")
        self.test_ttmode_nexus_ref_files("88")
