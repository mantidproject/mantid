# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup,\
    ITableWorkspace
from mantid.simpleapi import *
from mantid import config
import os.path
import stresstesting
import unittest

import cry_ini
import cry_focus

DIFF_PLACES = 8
DIRS = config['datasearch.directories'].split(';')


class ISISPowderDiffractionGem(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["GEM/GEM46487.raw", "GEM/GEM46488.raw",
                    "GEM/GEM46489.raw", "GEM/VanaPeaks.dat",
                    "GEM/test/GrpOff/offsets_2009_cycle094.cal",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-0.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-1.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-2.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-3.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-4.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-5.nxs",
                    "GEM/test/Cycle_09_5/mantid_tester/GEM_095_calibration.pref"])

        # note: VanaPeaks.dat is used only if provided in the directory

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError, ose:
            print 'could not delete generated file : ', ose.filename

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('GEM', RawDir=(DIRS[0] + "GEM/"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=(DIRS[0]))
        expt.initialize('Cycle_09_5', user='mantid_tester', prefFile='GEM_095_calibration.pref')
        expt.tell()
        cry_focus.focus_all(expt, "46489")

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
        filenames = set(["GEM/test/Cycle_09_5/Calibration/offsets_2009_cycle094.cal",
                         "GEM/test/Cycle_09_5/mantid_tester/GEM46489.gss",
                         "GEM/test/Cycle_09_5/mantid_tester/GEM46489.nxs",
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/offsets_2009_cycle094.cal'])

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

    # ============================ Success ==============================
    def runTest(self):
        expt = cry_ini.Files('GEM', RawDir=(DIRS[0] + "GEM/"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=(DIRS[0]))
        expt.initialize('Cycle_09_5', user='mantid_tester', prefFile='GEM_095_calibration.pref')
        expt.tell()
        cry_focus.focus_all(expt, "46489")

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'GEM/test/Cycle_09_5/Calibration/offsets_2009_cycle094.cal')
        calfile2 = (DIRS[0] + 'GEM/test/Cycle_09_5/mantid_tester/offsets_2009_cycle094.cal')
        data1 = LoadCalFile(InstrumentName="GEM", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="GEM", CalFilename=calfile2,
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
        nxsfile = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(6, data[0].getNumberOfEntries())

        self.assertTrue(isinstance(data[1], MatrixWorkspace))
        self.assertTrue(isinstance(data[2], MatrixWorkspace))
        self.assertTrue(isinstance(data[3], MatrixWorkspace))
        self.assertTrue(isinstance(data[4], MatrixWorkspace))
        self.assertTrue(isinstance(data[5], MatrixWorkspace))
        self.assertTrue(isinstance(data[6], MatrixWorkspace))

        self.assertTrue("ResultTOF-1" in data[1].getName())
        self.assertTrue("ResultTOF-2" in data[2].getName())
        self.assertTrue("ResultTOF-3" in data[3].getName())
        self.assertTrue("ResultTOF-4" in data[4].getName())
        self.assertTrue("ResultTOF-5" in data[5].getName())
        self.assertTrue("ResultTOF-6" in data[6].getName())

        self.assertTrue('ResultTOFgrp', self.wsname[0])

    def test_gssfile_with_workspace(self):
        gssfile = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(6, data.getNumberHistograms())
        self.assertEquals(3572, data.blocksize())
        self.assertAlmostEqual(435.20061, data.readX(1)[7], places=DIFF_PLACES)
        self.assertAlmostEqual(26.909216575, data.readY(5)[5], places=DIFF_PLACES)


    def test_datfile_with_workspace(self):
        datfile1 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_D.dat")
        datfile2 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_TOF.dat")
        datfile3 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_D.dat")
        datfile4 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_TOF.dat")
        datfile5 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_D.dat")
        datfile6 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_TOF.dat")
        datfile7 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_D.dat")
        datfile8 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_TOF.dat")
        datfile9 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_D.dat")
        datfile10 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_TOF.dat")
        datfile11 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_D.dat")
        datfile12 = (DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_TOF.dat")

        data1 = LoadAscii(Filename=datfile1, OutputWorkspace="datWorkspace1")
        data2 = LoadAscii(Filename=datfile2, OutputWorkspace="datWorkspace2")
        data3 = LoadAscii(Filename=datfile3, OutputWorkspace="datWorkspace3")
        data4 = LoadAscii(Filename=datfile4, OutputWorkspace="datWorkspace4")
        data5 = LoadAscii(Filename=datfile5, OutputWorkspace="datWorkspace5")
        data6 = LoadAscii(Filename=datfile6, OutputWorkspace="datWorkspace6")
        data7 = LoadAscii(Filename=datfile7, OutputWorkspace="datWorkspace7")
        data8 = LoadAscii(Filename=datfile8, OutputWorkspace="datWorkspace8")
        data9 = LoadAscii(Filename=datfile9, OutputWorkspace="datWorkspace9")
        data10 = LoadAscii(Filename=datfile10, OutputWorkspace="datWorkspace10")
        data11 = LoadAscii(Filename=datfile11, OutputWorkspace="datWorkspace11")
        data12 = LoadAscii(Filename=datfile12, OutputWorkspace="datWorkspace12")

        self.assertTrue(isinstance(data1, MatrixWorkspace))
        self.assertTrue(isinstance(data2, MatrixWorkspace))
        self.assertTrue(isinstance(data3, MatrixWorkspace))
        self.assertTrue(isinstance(data4, MatrixWorkspace))
        self.assertTrue(isinstance(data5, MatrixWorkspace))
        self.assertTrue(isinstance(data6, MatrixWorkspace))
        self.assertTrue(isinstance(data7, MatrixWorkspace))
        self.assertTrue(isinstance(data8, MatrixWorkspace))
        self.assertTrue(isinstance(data9, MatrixWorkspace))
        self.assertTrue(isinstance(data10, MatrixWorkspace))
        self.assertTrue(isinstance(data11, MatrixWorkspace))
        self.assertTrue(isinstance(data12, MatrixWorkspace))

        self.assertTrue("datWorkspace1" in data1.getName())
        self.assertTrue("datWorkspace2" in data2.getName())
        self.assertTrue("datWorkspace3" in data3.getName())
        self.assertTrue("datWorkspace4" in data4.getName())
        self.assertTrue("datWorkspace5" in data5.getName())
        self.assertTrue("datWorkspace6" in data6.getName())
        self.assertTrue("datWorkspace7" in data7.getName())
        self.assertTrue("datWorkspace8" in data8.getName())
        self.assertTrue("datWorkspace9" in data9.getName())
        self.assertTrue("datWorkspace10" in data10.getName())
        self.assertTrue("datWorkspace11" in data11.getName())
        self.assertTrue("datWorkspace12" in data12.getName())

        self.assertEquals(1, data1.getNumberHistograms())
        self.assertEquals(3572, data1.blocksize())
        self.assertAlmostEqual(0.43865, data1.readX(0)[2], places=DIFF_PLACES)

        self.assertEquals(1, data2.getNumberHistograms())
        self.assertEquals(3572, data2.blocksize())
        self.assertAlmostEqual(data1.readY(0)[6], data2.readY(0)[6], places=DIFF_PLACES)
        self.assertEquals(1, data3.getNumberHistograms())
        self.assertEquals(3427, data3.blocksize())
        self.assertAlmostEqual(0.36329, data3.readX(0)[199], places=DIFF_PLACES)

        self.assertEquals(1, data4.getNumberHistograms())
        self.assertEquals(3427, data4.blocksize())
        self.assertAlmostEqual(9.7999821399, data4.readE(0)[10], places=DIFF_PLACES)

        self.assertEquals(1, data5.getNumberHistograms())
        self.assertEquals(3462, data5.blocksize())
        self.assertAlmostEqual(data5.readY(0)[2228], data6.readY(0)[2228], places=DIFF_PLACES)

        self.assertEquals(1, data6.getNumberHistograms())
        self.assertEquals(3462, data6.blocksize())
        self.assertAlmostEqual(62.05717607, data6.readY(0)[0], places=DIFF_PLACES)

        self.assertEquals(1, data7.getNumberHistograms())
        self.assertEquals(3409, data7.blocksize())
        self.assertAlmostEqual(8.85650716, data7.readE(0)[0], places=DIFF_PLACES)

        self.assertEquals(1, data8.getNumberHistograms())
        self.assertEquals(3409, data8.blocksize())
        self.assertAlmostEqual(data7.readY(0)[6], data8.readY(0)[6], places=DIFF_PLACES)

        self.assertEquals(1, data9.getNumberHistograms())
        self.assertEquals(3370, data9.blocksize())
        self.assertAlmostEqual(data9.readE(0)[3369], data10.readE(0)[3369], places=DIFF_PLACES)

        self.assertEquals(1, data10.getNumberHistograms())
        self.assertEquals(3370, data10.blocksize())
        self.assertAlmostEqual(458.71929, data10.readX(0)[10], places=DIFF_PLACES)

        self.assertEquals(1, data11.getNumberHistograms())
        self.assertEquals(3307, data11.blocksize())
        self.assertAlmostEqual(data11.readY(0)[50], data12.readY(0)[50], places=DIFF_PLACES)

        self.assertEquals(1, data12.getNumberHistograms())
        self.assertEquals(3307, data12.blocksize())
        self.assertAlmostEqual(499.0768, data12.readX(0)[0], places=DIFF_PLACES)
