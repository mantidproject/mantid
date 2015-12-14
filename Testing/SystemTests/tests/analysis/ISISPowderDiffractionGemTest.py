# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup, \
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
        filenames = []

        filenames.extend(("GEM/GEM46487.raw", "GEM/GEM46488.raw", "GEM/GEM46489.raw", "GEM/VanaPeaks.dat",
                          "GEM/test/GrpOff/offsets_2009_cycle094.cal",
                          "GEM/test/Cycle_09_5/mantid_tester/GEM_095_calibration.pref"))
        for i in range(0, 6):
            filenames.append("GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-" + str(i) + ".nxs")

        return filenames
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
        filenames = []
        filenames.extend(("GEM/test/Cycle_09_5/Calibration/offsets_2009_cycle094.cal",
                          "GEM/test/Cycle_09_5/mantid_tester/GEM46489.gss",
                          "GEM/test/Cycle_09_5/mantid_tester/GEM46489.nxs",
                          'GEM/test/Cycle_09_5/mantid_tester/offsets_2009_cycle094.cal'))

        for i in range(1, 7):
            filenames.append('GEM/test/Cycle_09_5/mantid_tester/GEM46489_b' + str(i) + '_D.dat')
            filenames.append('GEM/test/Cycle_09_5/mantid_tester/GEM46489_b' + str(i) + '_TOF.dat')
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

        for i in range(0, 3):
            self.assertTrue(isinstance(data1[i], MatrixWorkspace))
        self.assertTrue(isinstance(data1[3], ITableWorkspace))

        self.assertTrue("CalWorkspace1_group" in data1[0].getName())
        self.assertTrue("CalWorkspace1_offsets" in data1[1].getName())
        self.assertTrue("CalWorkspace1_mask" in data1[2].getName())
        self.assertTrue("CalWorkspace1_cal" in data1[3].getName())

        for i in range(0, 3):
            self.assertTrue(isinstance(data2[i], MatrixWorkspace))
            self.assertTrue(isinstance(data1[3], ITableWorkspace))

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

        for i in range(1, 7):
            self.assertTrue(isinstance(data[i], MatrixWorkspace))
            self.assertTrue("ResultTOF-" + str(i) in data[i].getName())

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
        dat_files = []
        for i in range(1, 7):
            dat_files.append(DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b" + str(i) + "_D.dat")
            dat_files.append(DIRS[0] + "GEM/test/Cycle_09_5/mantid_tester/GEM46489_b" + str(i) + "_TOF.dat")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace="datWorkspace" + str(i+1)))

        for _file in dat_data:
            self.assertTrue(isinstance(_file, MatrixWorkspace))

        for i in range(0, len(dat_data)):
            self.assertTrue(("datWorkspace" + str(i+1)) in dat_data[i].getName())

        for _file in dat_data:
            self.assertEquals(1, _file.getNumberHistograms())

        for i in range(0, len(dat_data), 2):
            b_size_avg = ((dat_data[i].blocksize() + dat_data[i+1].blocksize()) / 2)
            self.assertEquals(b_size_avg, dat_data[i+1].blocksize())

        self.assertAlmostEqual(0.43865, dat_data[0].readX(0)[2], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[0].readY(0)[6], dat_data[1].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(0.36329, dat_data[2].readX(0)[199], places=DIFF_PLACES)
        self.assertAlmostEqual(9.7999821399, dat_data[3].readE(0)[10], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[4].readY(0)[2228], dat_data[5].readY(0)[2228], places=DIFF_PLACES)
        self.assertAlmostEqual(62.05717607, dat_data[5].readY(0)[0], places=DIFF_PLACES)

        self.assertAlmostEqual(8.85650716, dat_data[6].readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[6].readY(0)[6], dat_data[7].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[8].readE(0)[3369], dat_data[9].readE(0)[3369], places=DIFF_PLACES)
        self.assertAlmostEqual(458.71929, dat_data[9].readX(0)[10], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[10].readY(0)[50], dat_data[10].readY(0)[50], places=DIFF_PLACES)
        self.assertAlmostEqual(499.0768, dat_data[11].readX(0)[0], places=DIFF_PLACES)
