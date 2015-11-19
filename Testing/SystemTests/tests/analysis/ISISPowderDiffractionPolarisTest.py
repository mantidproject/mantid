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


class ISISPowderDiffractionPol(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["POLARIS/POL78338.raw", "POLARIS/POL78339.raw",
                    "POLARIS/POL79514.raw", "POLARIS/VanaPeaks.dat",
                    "POLARIS/test/GrpOff/offsets_2011_cycle111b.cal",
                    "POLARIS/test/Cycle_15_2/Mantid_tester/UserPrefFile_15_2.pref"])
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
        expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=DIRS[0])
        expt.initialize('Cycle_15_2', user='Mantid_tester', prefFile='UserPrefFile_15_2.pref')
        expt.tell()
        cry_focus.focus_all(expt, "79514", Write_ExtV=False)

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
        filenames = set(["POLARIS/test/Cycle_15_2/Calibration/POL_ini.cal",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-0.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-1.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-2.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-3.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-4.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-5.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-0.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-0_.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-1.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-1_.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-2.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-2_.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-3.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-3_.dat",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-4.nxs",
                         "POLARIS/test/Cycle_15_2/Calibration/"
                         "POL_2015_2_5mm_vrod_78338_152_calfile_new-4_.dat",

                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL_ini.cal",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.gss",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.nxs",

                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b1_D.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b1_TOF.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b2_D.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b2_TOF.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b3_D.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b3_TOF.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b4_D.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b4_TOF.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b5_D.dat",
                         "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b5_TOF.dat"])

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
        expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=DIRS[0])
        expt.initialize('Cycle_15_2', user='Mantid_tester', prefFile='UserPrefFile_15_2.pref')
        expt.tell()
        cry_focus.focus_all(expt, "79514", Write_ExtV=False)

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'POLARIS/test/Cycle_15_2/Calibration/POL_ini.cal')
        calfile2 = (DIRS[0] + 'POLARIS/test/Cycle_15_2/Mantid_tester/POL_ini.cal')
        data1 = LoadCalFile(InstrumentName="POL", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="POL", CalFilename=calfile2,
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
        nxsfile = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(5, data[0].getNumberOfEntries())

        self.assertTrue(isinstance(data[1], MatrixWorkspace))
        self.assertTrue(isinstance(data[2], MatrixWorkspace))
        self.assertTrue(isinstance(data[3], MatrixWorkspace))
        self.assertTrue(isinstance(data[4], MatrixWorkspace))
        self.assertTrue(isinstance(data[5], MatrixWorkspace))

        self.assertTrue('POL79514', self.wsname[0])

        self.assertTrue("ResultTOF-1" in data[1].getName())
        self.assertTrue("ResultTOF-2" in data[2].getName())
        self.assertTrue("ResultTOF-3" in data[3].getName())
        self.assertTrue("ResultTOF-4" in data[4].getName())
        self.assertTrue("ResultTOF-5" in data[5].getName())

    def test_gssfile_with_workspace(self):
        self.wsname = "GSSFile"
        gssfile = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(5, data.getNumberHistograms())
        self.assertEquals(1001, data.blocksize())
        self.assertAlmostEqual(204.75, data.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.89860259, data.readY(4)[100], places=DIFF_PLACES)

    def test_datfile_with_workspace(self):
        datfile1 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b1_D.dat")
        datfile2 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b1_TOF.dat")
        datfile3 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b2_D.dat")
        datfile4 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b2_TOF.dat")
        datfile5 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b3_D.dat")
        datfile6 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b3_TOF.dat")
        datfile7 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b4_D.dat")
        datfile8 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b4_TOF.dat")
        datfile9 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b5_D.dat")
        datfile10 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b5_TOF.dat")

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

        self.assertEquals(1, data1.getNumberHistograms())
        self.assertEquals(1001, data1.blocksize())
        self.assertAlmostEqual(0.28597, data1.readX(0)[2], places=DIFF_PLACES)

        self.assertEquals(1, data2.getNumberHistograms())
        self.assertEquals(1001, data2.blocksize())
        self.assertAlmostEqual(data1.readY(0)[6], data2.readY(0)[6], places=DIFF_PLACES)

        self.assertEquals(1, data3.getNumberHistograms())
        self.assertEquals(4576, data3.blocksize())
        self.assertAlmostEqual(0.17968, data3.readX(0)[199], places=DIFF_PLACES)

        self.assertEquals(1, data4.getNumberHistograms())
        self.assertEquals(4576, data4.blocksize())
        self.assertAlmostEqual(0.17296326, data4.readE(0)[10], places=DIFF_PLACES)

        self.assertEquals(1, data5.getNumberHistograms())
        self.assertEquals(4543, data5.blocksize())
        self.assertAlmostEqual(data5.readY(0)[2228], data6.readY(0)[2228], places=DIFF_PLACES)

        self.assertEquals(1, data6.getNumberHistograms())
        self.assertEquals(4543, data6.blocksize())
        self.assertAlmostEqual(1.09539144, data6.readY(0)[0], places=DIFF_PLACES)

        self.assertEquals(1, data7.getNumberHistograms())
        self.assertEquals(4413, data7.blocksize())
        self.assertAlmostEqual(0.0353677, data7.readE(0)[0], places=DIFF_PLACES)

        self.assertEquals(1, data8.getNumberHistograms())
        self.assertEquals(4413, data8.blocksize())
        self.assertAlmostEqual(data7.readY(0)[6], data8.readY(0)[6], places=DIFF_PLACES)

        self.assertEquals(1, data9.getNumberHistograms())
        self.assertEquals(6107, data9.blocksize())
        self.assertAlmostEqual(data9.readE(0)[3369], data10.readE(0)[3369], places=DIFF_PLACES)

        self.assertEquals(1, data10.getNumberHistograms())
        self.assertEquals(6107, data10.blocksize())
        self.assertAlmostEqual(299.75529, data10.readX(0)[10], places=DIFF_PLACES)

    def test_upstripped_files(self):
        global DIFF_PLACES
        DIFF_PLACES = 3

        nxsfile = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                             "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped.nxs")

        datfile1 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-0.dat")
        datfile2 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-1.dat")
        datfile3 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-2.dat")
        datfile4 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-3.dat")
        datfile5 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-4.dat")
        datfile6 = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-5.dat")

        nxsdata = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace="nxs_workspace")
        data1 = LoadAscii(Filename=datfile1, OutputWorkspace="dat_workspace1")
        data2 = LoadAscii(Filename=datfile2, OutputWorkspace="dat_workspace2")
        data3 = LoadAscii(Filename=datfile3, OutputWorkspace="dat_workspace3")
        data4 = LoadAscii(Filename=datfile4, OutputWorkspace="dat_workspace3")
        data5 = LoadAscii(Filename=datfile5, OutputWorkspace="dat_workspace3")
        data6 = LoadAscii(Filename=datfile6, OutputWorkspace="dat_workspace3")

        self.assertTrue(isinstance(nxsdata, MatrixWorkspace))
        self.assertEquals(6, nxsdata.getNumberHistograms())
        self.assertEquals(7793, nxsdata.blocksize())

        self.assertAlmostEqual(nxsdata.readY(0)[0], data1.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(0)[4500], data1.readY(0)[4500], places=DIFF_PLACES)

        self.assertAlmostEqual(nxsdata.readY(1)[17000], data2.readY(0)[17000], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(1)[22000], data2.readY(0)[22000], places=DIFF_PLACES)

        self.assertAlmostEqual(nxsdata.readY(2)[0], data3.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(2)[4500], data3.readY(0)[4500], places=DIFF_PLACES)

        self.assertAlmostEqual(nxsdata.readY(3)[17000], data4.readY(0)[17000], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(3)[22000], data4.readY(0)[22000], places=DIFF_PLACES)

        self.assertAlmostEqual(nxsdata.readY(4)[0], data5.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(4)[4500], data5.readY(0)[4500], places=DIFF_PLACES)

        self.assertAlmostEqual(nxsdata.readY(5)[17000], data6.readY(0)[17000], places=DIFF_PLACES)
        self.assertAlmostEqual(nxsdata.readY(5)[22000], data6.readY(0)[22000], places=DIFF_PLACES)




