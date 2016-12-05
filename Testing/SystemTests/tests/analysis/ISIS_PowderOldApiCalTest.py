from __future__ import (absolute_import, division, print_function)
import unittest
import os.path

from mantid.api import AnalysisDataService, MatrixWorkspace, ITableWorkspace
from mantid.simpleapi import *
from mantid import config
import stresstesting
import pearl_routines

DIFF_PLACES = 8
# initial directory to the system test data files
DIRS = config['datasearch.directories'].split(';')


class PowderDiffOldApiCalibrateTest(stresstesting.MantidStressTest):
    _existing_config = None

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
        except OSError as ose:
            print ('could not delete the generated file: ', ose.filename)

    def runTest(self):
        self._success = False
        self._existing_config = config['datasearch.directories']

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
        self.assertAlmostEquals(-5.59125571e-06, van_tt35_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(1.45223820e-05, van_tt35_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(5.16652141e-06, van_tt35_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(2.30071459205e-08, van_tt35_data[4].readE(0)[17], places=DIFF_PLACES)
        self.assertAlmostEquals(177.218826007, van_tt35_data[13].readX(0)[278], places=DIFF_PLACES)

        tt_70_file = 'van_spline_TT70_cycle_15_3.nxs'
        vanadium_tt_70_dir = os.path.join(vanadium_base_dir, tt_70_file)
        van_tt70_data = LoadNexusProcessed(Filename=vanadium_tt_70_dir,
                                           OutputWorkspace=tt_70_file)
        self.matrix_workspaces_test(van_tt70_data, 'tt70')

        self.assertAlmostEquals(-1.00937319e-05, van_tt70_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(3.03206351e-05, van_tt70_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(1.04670371e-05, van_tt70_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(3.2808780582e-08, van_tt70_data[4].readE(0)[17], places=DIFF_PLACES)
        self.assertAlmostEquals(177.218826007, van_tt70_data[13].readX(0)[278], places=DIFF_PLACES)

        tt_88_file = 'van_spline_TT88_cycle_15_3.nxs'
        vanadium_tt_88_dir = os.path.join(vanadium_base_dir, tt_88_file)
        van_tt88_data = LoadNexusProcessed(Filename=vanadium_tt_88_dir,
                                           OutputWorkspace=tt_88_file)
        self.matrix_workspaces_test(van_tt88_data, 'tt88')

        self.assertAlmostEquals(-1.15778020e-05, van_tt88_data[1].readY(0)[2], places=DIFF_PLACES)
        self.assertAlmostEquals(3.61722568e-05, van_tt88_data[5].readY(0)[4545], places=DIFF_PLACES)
        self.assertAlmostEquals(1.27747701e-05, van_tt88_data[8].readY(0)[7553], places=DIFF_PLACES)
        self.assertAlmostEquals(3.21810924e-08, van_tt88_data[4].readE(0)[17], places=DIFF_PLACES)
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
