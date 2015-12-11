# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup, ITableWorkspace
from mantid.simpleapi import *
from mantid import config
import os.path
import stresstesting
import unittest

import cry_ini
import cry_focus

DIFF_PLACES = 8
DIRS = config['datasearch.directories'].split(';')


class ISISPowderDiffractionHrpd1(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["hrp39191.raw", "hrp39187.raw", "hrp43022.raw", "hrpd/test/GrpOff/hrpd_new_072_01.cal",
                    "hrpd/test/GrpOff/hrpd_new_072_01_corr.cal", "hrpd/test/cycle_09_2/Calibration/van_s1_old-0.nxs",
                    "hrpd/test/cycle_09_2/Calibration/van_s1_old-1.nxs",
                    "hrpd/test/cycle_09_2/Calibration/van_s1_old-2.nxs", "hrpd/test/cycle_09_2/tester/mtd.pref"
                    ])

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError, ose:
            print 'could not delete generated file : ', ose.filename

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('hrpd', RawDir=(DIRS[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=DIRS[0])
        expt.initialize('cycle_09_2', user='tester', prefFile='mtd.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022")

        # Custom code to create and run this single test suite
        # and then mark as success or failure
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
        filenames = set(["hrpd/test/cycle_09_2/Calibration/hrpd_new_072_01_corr.cal",
                         "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.gss",
                         "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs",
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_D.dat',
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_TOF.dat',
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_D.dat',
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_TOF.dat',
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_D.dat',
                         'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_TOF.dat',
                         'hrpd/test/cycle_09_2/tester/hrpd_new_072_01_corr.cal'])
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

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'hrpd/test/cycle_09_2/Calibration/hrpd_new_072_01_corr.cal')
        calfile2 = (DIRS[0] + 'hrpd/test/cycle_09_2/tester/hrpd_new_072_01_corr.cal')
        data1 = LoadCalFile(InstrumentName="hrpd", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="hrpd", CalFilename=calfile2,
                            WorkspaceName="CalWorkspace2")

        for i in range(0, 3):
            self.assertTrue(isinstance(data1[i], MatrixWorkspace))

        self.assertTrue("CalWorkspace1_group" in data1[0].getName())
        self.assertTrue("CalWorkspace1_offsets" in data1[1].getName())
        self.assertTrue("CalWorkspace1_mask" in data1[2].getName())
        self.assertTrue("CalWorkspace1_cal" in data1[3].getName())

        for i in range(0, 3):
            self.assertTrue(isinstance(data2[i], MatrixWorkspace))

        self.assertTrue("CalWorkspace2_group" in data2[0].getName())
        self.assertTrue("CalWorkspace2_offsets" in data2[1].getName())
        self.assertTrue("CalWorkspace2_mask" in data2[2].getName())
        self.assertTrue("CalWorkspace2_cal" in data2[3].getName())

    def test_nxsfile_with_workspace(self):
        self.wsname = "NexusWorkspace"
        nxsfile = (DIRS[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(3, data[0].getNumberOfEntries())

        for i in range(1, 3):
            self.assertTrue(isinstance(data[i], MatrixWorkspace))

        for i in range(1, 3):
            self.assertTrue(("ResultTOF-" + str(i)) in data[i].getName())

        self.assertTrue('ResultTOFgrp', self.wsname[0])

    def test_gssfile_with_workspace(self):
        gssfile = (DIRS[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(3, data.getNumberHistograms())
        self.assertEquals(22981, data.blocksize())
        self.assertAlmostEqual(9783.15138, data.readX(2)[1], places=DIFF_PLACES)

    def test_datfile_with_workspace(self):

        file_name = []
        for i in range(0, 3):
            file_name.append((DIRS[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b" + str(i+1) + "_D.dat"))
            file_name.append((DIRS[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b" + str(i+1) + "_TOF.dat"))

        data_list = []
        for i in range(0, len(file_name)):
            data_list.append((LoadAscii(Filename=file_name[i], OutputWorkspace=("datWorkspace" + str(i+1)))))
            self.assertTrue(isinstance(data_list[i], MatrixWorkspace))
            self.assertTrue(("datWorkspace"+str(i+1)) in data_list[i].getName())
            self.assertEquals(1, data_list[i].getNumberHistograms())

        self.assertEquals(22981, data_list[0].blocksize())
        self.assertAlmostEqual(0.2168, data_list[0].readX(0)[2], places=DIFF_PLACES)

        self.assertEquals(22981, data_list[1].blocksize())
        self.assertAlmostEqual(data_list[0].readY(0)[6], data_list[1].readY(0)[6], places=DIFF_PLACES)

        self.assertEquals(23038, data_list[2].blocksize())
        self.assertAlmostEqual(0.44656, data_list[2].readX(0)[4311], places=DIFF_PLACES)

        self.assertEquals(23038, data_list[3].blocksize())
        self.assertAlmostEqual(1.58185696, data_list[3].readE(0)[10], places=DIFF_PLACES)

        self.assertEquals(23025, data_list[4].blocksize())
        self.assertAlmostEqual(data_list[4].readY(0)[15098], data_list[5].readY(0)[15098], places=DIFF_PLACES)

        self.assertEquals(23025, data_list[5].blocksize())
        self.assertAlmostEqual(9782.63819, data_list[5].readX(0)[0], places=DIFF_PLACES)

# ================ Below test cases use different pref file ==================
# =================== when 'ExistingV' = 'no' in pref file ===================


class ISISPowderDiffractionHrpd2(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["hrp39191.raw", "hrp39187.raw", "hrp43022.raw", "hrpd/test/GrpOff/hrpd_new_072_01.cal",
                    "hrpd/test/GrpOff/hrpd_new_072_01_corr.cal", "hrpd/test/cycle_09_2_no_existv/tester/mtd2.pref"])

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError, ose:
            print 'could not delete generated file : ', ose.filename

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('hrpd', RawDir=(DIRS[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=DIRS[0])
        expt.initialize('cycle_09_2_no_existv', user='tester', prefFile='mtd2.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022", Write_ExtV=False)

        # Custom code to create and run this single test suite
        # and then mark as success or failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadTests2, "test"))
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
        filenames = (["hrpd/test/cycle_09_2_no_existv/Calibration/hrpd_new_072_01_corr.cal",
                      "hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old.gss",
                      "hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old.nxs",
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b1_D.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b1_TOF.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b2_D.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b2_TOF.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b3_D.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrp43022_s1_old_b3_TOF.dat',
                      'hrpd/test/cycle_09_2_no_existv/tester/hrpd_new_072_01_corr.cal',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped.nxs',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-0.dat',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-1.dat',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-2.dat',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-0.nxs',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-0_.dat',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-1.nxs',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-1_.dat',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-2.nxs',
                      'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-2_.dat'])
        self._clean_up_files(filenames, DIRS)


# ======================================================================
# work horse
class LoadTests2(unittest.TestCase):
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

    def test_upstripped_files(self):
        diff_places = 3

        nxsfile = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped.nxs')
        datfile1 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-0.dat')
        datfile2 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-1.dat')
        datfile3 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new_unstripped-2.dat')

        nxsdata = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace="nxs_workspace")
        data1 = LoadAscii(Filename=datfile1, OutputWorkspace="dat_workspace1")
        data2 = LoadAscii(Filename=datfile2, OutputWorkspace="dat_workspace2")
        data3 = LoadAscii(Filename=datfile3, OutputWorkspace="dat_workspace3")

        self.assertTrue(isinstance(nxsdata, MatrixWorkspace))
        self.assertEquals(3, nxsdata.getNumberHistograms())
        self.assertEquals(23987, nxsdata.blocksize())

        self.assertAlmostEqual(nxsdata.readX(0)[0], data1.readX(0)[0], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(0)[4500], data1.readX(0)[4500], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(0)[17000], data1.readX(0)[17000], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(0)[22000], data1.readX(0)[22000], places=diff_places)

        self.assertAlmostEqual(nxsdata.readX(1)[0], data2.readX(0)[0], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(1)[4500], data2.readX(0)[4500], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(1)[17000], data2.readX(0)[17000], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(1)[22000], data2.readX(0)[22000], places=diff_places)

        self.assertAlmostEqual(nxsdata.readX(2)[0], data3.readX(0)[0], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(2)[4500], data3.readX(0)[4500], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(2)[17000], data3.readX(0)[17000], places=diff_places)
        self.assertAlmostEqual(nxsdata.readX(2)[22000], data3.readX(0)[22000], places=diff_places)

    def test_van_s1_files(self):
        diff_places = 3

        nxsfile1 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-0.nxs')
        nxsfile2 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-1.nxs')
        nxsfile3 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-2.nxs')
        datfile1 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-0_.dat')
        datfile2 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-1_.dat')
        datfile3 = (DIRS[0] + 'hrpd/test/cycle_09_2_no_existv/Calibration/van_s1_old_new-2_.dat')

        nxsdata1 = LoadNexusProcessed(Filename=nxsfile1, OutputWorkspace="nxs_workspace1")
        nxsdata2 = LoadNexusProcessed(Filename=nxsfile2, OutputWorkspace="nxs_workspace2")
        nxsdata3 = LoadNexusProcessed(Filename=nxsfile3, OutputWorkspace="nxs_workspace3")
        data1 = LoadAscii(Filename=datfile1, OutputWorkspace="dat_workspace1")
        data2 = LoadAscii(Filename=datfile2, OutputWorkspace="dat_workspace2")
        data3 = LoadAscii(Filename=datfile3, OutputWorkspace="dat_workspace3")

        self.assertTrue(isinstance(nxsdata1, MatrixWorkspace))
        self.assertTrue(isinstance(nxsdata2, MatrixWorkspace))
        self.assertTrue(isinstance(nxsdata3, MatrixWorkspace))

        self.assertAlmostEqual(nxsdata1.readX(0)[0], data1.readX(0)[0], places=diff_places)
        self.assertAlmostEqual(nxsdata2.readX(0)[4500], data2.readX(0)[4500], places=diff_places)
        self.assertAlmostEqual(nxsdata3.readX(0)[17000], data3.readX(0)[17000], places=diff_places)
        self.assertAlmostEqual(nxsdata3.readX(0)[22000], data3.readX(0)[22000], places=diff_places)
        self.assertAlmostEqual(nxsdata2.readX(0)[0], data2.readX(0)[0], places=diff_places)
        self.assertAlmostEqual(nxsdata1.readX(0)[4500], data1.readX(0)[4500], places=diff_places)
