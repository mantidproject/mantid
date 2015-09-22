# pylint: disable=no-init

from mantid.api import MatrixWorkspace, WorkspaceGroup, ITableWorkspace
from mantid.api import FileFinder
from mantid.simpleapi import *
from mantid import config
import os.path
import stresstesting
import sys
import unittest

F_DIR = FileFinder.getFullPath("PowderISIS")
sys.path.append(F_DIR)
import cry_ini
import cry_focus

DIFF_PLACES = 8
dirs = config['datasearch.directories'].split(';')


class ISISPowderDiffraction(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["hrp39191.raw", "hrp39187.raw", "hrp43022.raw", "hrpd/test/GrpOff/hrpd_new_072_01.cal",
                    "hrpd/test/GrpOff/hrpd_new_072_01_corr.cal", "hrpd/test/cycle_09_2/Calibration/van_s1_old-0.nxs",
                    "hrpd/test/cycle_09_2/Calibration/van_s1_old-1.nxs",
                    "hrpd/test/cycle_09_2/Calibration/van_s1_old-2.nxs", "hrpd/test/cycle_09_2/tester/mtd.pref"])

    def _clean_up_files(self, filenames, directories):
        try:
            for file in filenames:
                path = os.path.join(directories[0], file)
                os.remove(path)
        except OSError, ose:
            print 'could not delete generated file : ', ose.filename

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('hrpd', RawDir=(dirs[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=dirs[0])
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

        self._clean_up_files(filenames, dirs)


# ------------------------------------------------------------------------------
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

    # --------------------------------------- Success ------------------
    def runTest(self):
        dirs = config['datasearch.directories'].split(';')
        expt = cry_ini.Files('hrpd', RawDir=(dirs[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=dirs[0])
        expt.initialize('cycle_09_2', user='tester', prefFile='mtd.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022")

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (dirs[0] + 'hrpd/test/cycle_09_2/Calibration/hrpd_new_072_01_corr.cal')
        calfile2 = (dirs[0] + 'hrpd/test/cycle_09_2/tester/hrpd_new_072_01_corr.cal')
        data1 = LoadCalFile(InstrumentName="ENGIN-X", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="ENGIN-X", CalFilename=calfile2,
                            WorkspaceName="CalWorkspace2")

        self.assertTrue(isinstance(data1[0], MatrixWorkspace))
        self.assertTrue(isinstance(data1[1], MatrixWorkspace))
        self.assertTrue(isinstance(data1[2], MatrixWorkspace))
        self.assertTrue(isinstance(data1[3], ITableWorkspace))

        self.assertTrue("CalWorkspace1_group" in data1[0].getName())
        self.assertTrue("CalWorkspace1_offsets" in data1[1].getName())
        self.assertTrue("CalWorkspace1_mask" in data1[2].getName())
        self.assertTrue("CalWorkspace1_cal" in data1[3].getName())

        self.assertTrue(isinstance(data2[0], MatrixWorkspace))
        self.assertTrue(isinstance(data2[1], MatrixWorkspace))
        self.assertTrue(isinstance(data2[2], MatrixWorkspace))
        self.assertTrue(isinstance(data2[3], ITableWorkspace))

        self.assertTrue("CalWorkspace2_group" in data2[0].getName())
        self.assertTrue("CalWorkspace2_offsets" in data2[1].getName())
        self.assertTrue("CalWorkspace2_mask" in data2[2].getName())
        self.assertTrue("CalWorkspace2_cal" in data2[3].getName())

    def test_nxsfile_with_workspace(self):
        self.wsname = "NexusWorkspace"
        nxsfile = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(3, data[0].getNumberOfEntries())

        self.assertTrue(isinstance(data[1], MatrixWorkspace))
        self.assertTrue(isinstance(data[2], MatrixWorkspace))
        self.assertTrue(isinstance(data[3], MatrixWorkspace))

        self.assertTrue("ResultTOF-1" in data[1].getName())
        self.assertTrue("ResultTOF-2" in data[2].getName())
        self.assertTrue("ResultTOF-3" in data[3].getName())

        self.assertTrue('ResultTOFgrp', self.wsname[0])

    def test_gssfile_with_workspace(self):
        gssfile = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(3, data.getNumberHistograms())
        self.assertEquals(22981, data.blocksize())
        self.assertAlmostEqual(9783.15138, data.readX(2)[1], places=8)

    def test_datfile_with_workspace(self):
        datfile1 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_D.dat")
        datfile2 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_TOF.dat")
        datfile3 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_D.dat")
        datfile4 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_TOF.dat")
        datfile5 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_D.dat")
        datfile6 = (dirs[0] + "hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_TOF.dat")

        data1 = LoadAscii(Filename=datfile1, OutputWorkspace="datWorkspace1")
        data2 = LoadAscii(Filename=datfile2, OutputWorkspace="datWorkspace2")
        data3 = LoadAscii(Filename=datfile3, OutputWorkspace="datWorkspace3")
        data4 = LoadAscii(Filename=datfile4, OutputWorkspace="datWorkspace4")
        data5 = LoadAscii(Filename=datfile5, OutputWorkspace="datWorkspace5")
        data6 = LoadAscii(Filename=datfile6, OutputWorkspace="datWorkspace6")

        self.assertTrue(isinstance(data1, MatrixWorkspace))
        self.assertTrue(isinstance(data2, MatrixWorkspace))
        self.assertTrue(isinstance(data3, MatrixWorkspace))
        self.assertTrue(isinstance(data4, MatrixWorkspace))
        self.assertTrue(isinstance(data5, MatrixWorkspace))
        self.assertTrue(isinstance(data6, MatrixWorkspace))

        self.assertTrue("datWorkspace1" in data1.getName())
        self.assertTrue("datWorkspace2" in data2.getName())
        self.assertTrue("datWorkspace3" in data3.getName())
        self.assertTrue("datWorkspace4" in data4.getName())
        self.assertTrue("datWorkspace5" in data5.getName())
        self.assertTrue("datWorkspace6" in data6.getName())

        self.assertEquals(1, data1.getNumberHistograms())
        self.assertEquals(22981, data1.blocksize())

        self.assertEquals(1, data2.getNumberHistograms())
        self.assertEquals(22981, data2.blocksize())

        self.assertEquals(1, data3.getNumberHistograms())
        self.assertEquals(23038, data3.blocksize())

        self.assertEquals(1, data4.getNumberHistograms())
        self.assertEquals(23038, data4.blocksize())

        self.assertEquals(1, data5.getNumberHistograms())
        self.assertEquals(23025, data5.blocksize())

        self.assertEquals(1, data6.getNumberHistograms())
        self.assertEquals(23025, data6.blocksize())
