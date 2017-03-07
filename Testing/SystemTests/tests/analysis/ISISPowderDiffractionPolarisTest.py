# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

from __future__ import (absolute_import, division, print_function)
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup, \
    ITableWorkspace
from mantid.simpleapi import *
from mantid import config
import os.path
import shutil
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
            cali_path = os.path.join(directories[0], "POLARIS/test/Cycle_15_2/Calibration")
            shutil.rmtree(cali_path)
        except OSError as ose:
            print('could not delete generated file : ', ose.filename)

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
        filenames = []
        filenames.extend(("POLARIS/test/Cycle_15_2/Calibration/offsets_2011_cycle111b.cal",
                          "POLARIS/test/Cycle_15_2/Mantid_tester/offsets_2011_cycle111b.cal",
                          "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.gss",
                          "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.nxs",
                          "POLARIS/test/Cycle_15_2/Calibration/"
                          "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped.nxs",))

        for i in range(0, 5):
            filenames.append("POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b" + str(i + 1) + "_D.dat")
            filenames.append("POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b" + str(i + 1) + "_TOF.dat")

            filenames.append("POLARIS/test/Cycle_15_2/Calibration/"
                             "POL_2015_2_5mm_vrod_78338_152_calfile_new-" + str(i) + ".nxs")
            filenames.append("POLARIS/test/Cycle_15_2/Calibration/"
                             "POL_2015_2_5mm_vrod_78338_152_calfile_new-" + str(i) + "_.dat")

        for i in range(0, 6):
            filenames.append("POLARIS/test/Cycle_15_2/Calibration/"
                             "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-" + str(i) + ".dat")

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

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'POLARIS/test/Cycle_15_2/Calibration/offsets_2011_cycle111b.cal')
        calfile2 = (DIRS[0] + 'POLARIS/test/Cycle_15_2/Mantid_tester/offsets_2011_cycle111b.cal')
        data1 = LoadCalFile(InstrumentName="POL", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="POL", CalFilename=calfile2,
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
        self.assertTrue(isinstance(data2[3], ITableWorkspace))

        self.assertTrue("CalWorkspace2_group" in data2[0].getName())
        self.assertTrue("CalWorkspace2_offsets" in data2[1].getName())
        self.assertTrue("CalWorkspace2_mask" in data2[2].getName())
        self.assertTrue("CalWorkspace2_cal" in data2[3].getName())

    def test_nxsfile_with_workspace(self):
        self.wsname = "NexusWorkspace"
        nxsfile = (DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514.nxs")
        nxs_data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(nxs_data[0], WorkspaceGroup))
        self.assertEquals(5, nxs_data[0].getNumberOfEntries())
        self.assertTrue('POL79514', self.wsname[0])

        for i in range(1, 6):
            self.assertTrue(isinstance(nxs_data[i], MatrixWorkspace))
            self.assertTrue(("ResultTOF-" + str(i)) in nxs_data[i].getName())

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
        dat_files = []
        for i in range(1, 6):
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b" + str(i) + "_D.dat")
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2/Mantid_tester/POL79514_b" + str(i) + "_TOF.dat")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace=("datWorkspace" + str(i))))

        for data in dat_data:
            self.assertTrue(isinstance(data, MatrixWorkspace))
            self.assertEquals(1, data.getNumberHistograms())

        for i in range(0, len(dat_data), 2):
            b_size_avg = ((dat_data[i].blocksize() + dat_data[i + 1].blocksize()) / 2)
            self.assertEquals(b_size_avg, dat_data[i + 1].blocksize())

        self.assertAlmostEqual(0.28597, dat_data[0].readX(0)[2], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[0].readY(0)[6], dat_data[1].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(0.17968, dat_data[2].readX(0)[199], places=DIFF_PLACES)
        self.assertAlmostEqual(0.17296326, dat_data[3].readE(0)[10], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[4].readY(0)[2228], dat_data[5].readY(0)[2228], places=DIFF_PLACES)
        self.assertAlmostEqual(1.09539144, dat_data[5].readY(0)[0], places=DIFF_PLACES)

        self.assertAlmostEqual(0.0353677, dat_data[6].readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[6].readY(0)[6], dat_data[7].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[8].readE(0)[3369], dat_data[9].readE(0)[3369], places=DIFF_PLACES)
        self.assertAlmostEqual(299.75529, dat_data[9].readX(0)[10], places=DIFF_PLACES)

    def test_upstripped_files(self):
        diff_places = 3

        nxs_file = (DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                              "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped.nxs")
        nxs_data = LoadNexusProcessed(Filename=nxs_file, OutputWorkspace="nxs_workspace")

        dat_files = []
        for i in range(0, 6):
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                                       "POL_2015_2_5mm_vrod_78338_152_calfile_new_unstripped-" + str(i) + ".dat")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace=("datWorkspace" + str(i))))

        self.assertTrue(isinstance(nxs_data, MatrixWorkspace))
        self.assertEquals(6, nxs_data.getNumberHistograms())
        self.assertEquals(7793, nxs_data.blocksize())

        self.assertAlmostEqual(nxs_data.readY(0)[0], dat_data[0].readY(0)[0], places=diff_places)
        self.assertAlmostEqual(nxs_data.readY(0)[4500], dat_data[0].readY(0)[4500], places=diff_places)

        self.assertAlmostEqual(nxs_data.readY(1)[1700], dat_data[1].readY(0)[1700], places=diff_places)
        self.assertAlmostEqual(nxs_data.readY(1)[2200], dat_data[1].readY(0)[2200], places=diff_places)

        self.assertAlmostEqual(nxs_data.readY(2)[4500], dat_data[2].readY(0)[4500], places=diff_places)

        self.assertAlmostEqual(nxs_data.readY(3)[1700], dat_data[3].readY(0)[1700], places=diff_places)
        self.assertAlmostEqual(nxs_data.readY(3)[2300], dat_data[3].readY(0)[2300], places=diff_places)

        self.assertAlmostEqual(nxs_data.readY(4)[0], dat_data[4].readY(0)[0], places=diff_places)
        self.assertAlmostEqual(nxs_data.readY(4)[4500], dat_data[4].readY(0)[4500], places=diff_places)

        self.assertAlmostEqual(nxs_data.readY(5)[1700], dat_data[5].readY(0)[1700], places=diff_places)
        self.assertAlmostEqual(nxs_data.readY(5)[7000], dat_data[5].readY(0)[7000], places=diff_places)

    def test_POL_2015_2_5mm_files(self):
        diff_places = 3

        dat_files = []
        for i in range(0, 5):
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                                       "POL_2015_2_5mm_vrod_78338_152_calfile_new-" + str(i) + "_.dat")

        nxs_files = []
        for i in range(0, 5):
            nxs_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2/Calibration/"
                                       "POL_2015_2_5mm_vrod_78338_152_calfile_new-" + str(i) + ".nxs")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace=("datWorkspace" + str(i + 1))))

        nxs_data = []
        for i in range(0, len(nxs_files)):
            nxs_data.append(LoadNexusProcessed(Filename=nxs_files[i], OutputWorkspace=("nxs_workspace" + str(i + 1))))

        for data in nxs_data:
            self.assertTrue(isinstance(data, MatrixWorkspace))

        self.assertEquals(7793, nxs_data[0].blocksize())
        self.assertEquals(7793, nxs_data[4].blocksize())

        self.assertAlmostEqual(nxs_data[0].readY(0)[0], dat_data[0].readY(0)[0], places=diff_places)
        self.assertAlmostEqual(nxs_data[1].readY(0)[4500], dat_data[1].readY(0)[4500], places=diff_places)
        self.assertAlmostEqual(nxs_data[2].readY(0)[7000], dat_data[2].readY(0)[7000], places=diff_places)
        self.assertAlmostEqual(nxs_data[3].readY(0)[2000], dat_data[3].readY(0)[2000], places=diff_places)
        self.assertAlmostEqual(nxs_data[4].readY(0)[0], dat_data[4].readY(0)[0], places=diff_places)


# ================ Below test cases use different pref file ==================
# =================== when 'ExistingV' = 'yes' in pref file ===================


class ISISPowderDiffractionPol2(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []
        filenames.extend(("POLARIS/POL78338.raw", "POLARIS/POL78339.raw",
                          "POLARIS/POL79514.raw", "POLARIS/VanaPeaks.dat",
                          "POLARIS/test/GrpOff/Cycle_12_2_group_masked_collimator.cal",
                          "POLARIS/test/GrpOff/cycle_15_2_silicon_all_spectra.cal",
                          "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/UserPrefFile_15_2_original.pref"))
        # note: VanaPeaks.dat is used only if provided in the directory

        for i in range(0, 5):
            filenames.append("POLARIS/test/Cycle_15_2_exist_v/Calibration/"
                             "POL_2015_2_5mm_vrod_78338_152_calfile-" + str(i) + ".nxs")

        return filenames

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError as ose:
            print('could not delete generated file : ', ose.filename)

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=DIRS[0])
        expt.initialize('Cycle_15_2_exist_v', user='Mantid_tester', prefFile='UserPrefFile_15_2_original.pref')
        expt.tell()
        cry_focus.focus_all(expt, "79514", Write_ExtV=False)

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
        filenames = []
        filenames.extend(("POLARIS/test/Cycle_15_2_exist_v/Calibration/Cycle_12_2_group_masked_collimator.cal",
                          "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/Cycle_12_2_group_masked_collimator.cal",
                          "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514.gss",
                          "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514.nxs"))

        for i in range(0, 5):
            filenames.append("POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514_b" + str(i + 1) + "_D.dat")
            filenames.append("POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514_b" + str(i + 1) + "_TOF.dat")

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

    # ============================ Success ==============================

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'POLARIS/test/Cycle_15_2_exist_v/Calibration/Cycle_12_2_group_masked_collimator.cal')
        calfile2 = (DIRS[0] + 'POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/Cycle_12_2_group_masked_collimator.cal')
        data1 = LoadCalFile(InstrumentName="POL", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="POL", CalFilename=calfile2,
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
            self.assertTrue(isinstance(data2[3], ITableWorkspace))

        self.assertTrue("CalWorkspace2_group" in data2[0].getName())
        self.assertTrue("CalWorkspace2_offsets" in data2[1].getName())
        self.assertTrue("CalWorkspace2_mask" in data2[2].getName())
        self.assertTrue("CalWorkspace2_cal" in data2[3].getName())

    def test_nxsfile_with_workspace(self):
        self.wsname = "NexusWorkspace"
        nxsfile = (DIRS[0] + "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(5, data[0].getNumberOfEntries())

        for i in range(1, 6):
            self.assertTrue(isinstance(data[i], MatrixWorkspace))

        self.assertTrue('POL79514', self.wsname[0])

        for i in range(1, 6):
            self.assertTrue("ResultTOF-" + str(i) in data[i].getName())

    def test_gssfile_with_workspace(self):
        self.wsname = "GSSFile"
        gssfile = (DIRS[0] + "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(5, data.getNumberHistograms())
        self.assertEquals(990, data.blocksize())
        self.assertAlmostEqual(221.15625, data.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(1.003815133228, data.readY(0)[0], places=DIFF_PLACES)

    def test_datfile_with_workspace(self):
        dat_files = []
        for i in range(0, 5):
            dat_files.append(
                DIRS[0] + "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/POL79514_b" + str(i + 1) + "_D.dat")
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_15_2_exist_v/Mantid_tester/"
                                       "POL79514_b" + str(i + 1) + "_TOF.dat")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace=("datWorkspace" + str(i))))

        for data in dat_data:
            self.assertTrue(isinstance(data, MatrixWorkspace))
            self.assertEquals(1, data.getNumberHistograms())

        for i in range(0, len(dat_data), 2):
            b_size_avg = ((dat_data[i].blocksize() + dat_data[i + 1].blocksize()) / 2)
            self.assertEquals(b_size_avg, dat_data[i + 1].blocksize())

        self.assertAlmostEqual(0.30078, dat_data[0].readX(0)[2], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[0].readY(0)[6], dat_data[1].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(0.16076, dat_data[2].readX(0)[199], places=DIFF_PLACES)
        self.assertAlmostEqual(0.15179466, dat_data[3].readE(0)[10], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[4].readY(0)[2228], dat_data[5].readY(0)[2228], places=DIFF_PLACES)
        self.assertAlmostEqual(1.01458598, dat_data[5].readY(0)[0], places=DIFF_PLACES)

        self.assertAlmostEqual(0.03795158, dat_data[6].readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[6].readY(0)[6], dat_data[7].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[8].readE(0)[3369], dat_data[9].readE(0)[3369], places=DIFF_PLACES)
        self.assertAlmostEqual(300.4597, dat_data[9].readX(0)[10], places=DIFF_PLACES)


# ================ Below test cases uses cycle 16_1 pref file ==================
# =================== where 'ExistingV' = 'no' in pref file ===================

class ISISPowderDiffractionPol3(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []
        filenames.extend(("POLARIS/POL93105.raw", "POLARIS/POL93106.raw",
                          "POLARIS/POL93107.raw", "POLARIS/VanaPeaks.dat",
                          "POLARIS/test/GrpOff/Cycle_12_2_group_masked_collimator_no_b5mod6.cal",
                          "POLARIS/test/GrpOff/cycle_16_1_silicon_all_spectra.cal",
                          "POLARIS/test/Cycle_16_1/Mantid_tester/UserPrefFile_16_1_si_calfile_"
                          "from_all_spectra_emptysub.pref"))
        # note: VanaPeaks.dat is used only if provided in the directory
        return filenames

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
            cali_path = os.path.join(directories[0], "POLARIS/test/Cycle_16_1/Calibration")
            shutil.rmtree(cali_path)
        except OSError as ose:
            print('could not delete generated file : ', ose.filename)

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=DIRS[0])
        expt.initialize('Cycle_16_1', user='Mantid_tester', prefFile='UserPrefFile_16_1_si_calfile_'
                                                                     'from_all_spectra_emptysub.pref')
        expt.tell()
        cry_focus.focus_all(expt, "93107", Write_ExtV=False)

        # Custom code to create and run this single test suite
        # and then mark as success or failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadTests3, "test"))
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
        filenames.extend(("POLARIS/test/Cycle_16_1/Mantid_tester/Cycle_12_2_group_masked_collimator_no_"
                          "b5mod6.cal", "POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets.gss",
                          "POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets.nxs",
                          "POLARIS/test/Cycle_16_1/Calibration/Cycle_12_2_group_masked_collimator_no_"
                          "b5mod6.cal", "POLARIS/test/Cycle_16_1/Calibration/POL_2016_1_5mm_vrod_testing_"
                                        "newcalfile_unstripped.nxs"))

        for i in range(0, 5):
            filenames.append("POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets_b" + str(i + 1) + "_D.dat")
            filenames.append("POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets_b" + str(i + 1) + "_TOF.dat")

            filenames.append("POLARIS/test/Cycle_16_1/Calibration/"
                             "POL_2016_1_5mm_vrod_testing_newcalfile-" + str(i) + ".nxs")
            filenames.append("POLARIS/test/Cycle_16_1/Calibration/"
                             "POL_2016_1_5mm_vrod_testing_newcalfile-" + str(i) + "_.dat")

        for i in range(0, 5):
            filenames.append("POLARIS/test/Cycle_16_1/Calibration/"
                             "POL_2016_1_5mm_vrod_testing_newcalfile_unstripped-" + str(i) + ".dat")

        self._clean_up_files(filenames, DIRS)


# ======================================================================
# work horse
class LoadTests3(unittest.TestCase):
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

    def test_calfile_with_workspace(self):
        self.wsname = "CalWorkspace1"
        calfile1 = (DIRS[0] + 'POLARIS/test/Cycle_16_1/Calibration/Cycle_12_2_group_masked_collimator_no_b5mod6.cal')
        calfile2 = (DIRS[0] + 'POLARIS/test/Cycle_16_1/Mantid_tester/Cycle_12_2_group_masked_collimator_no_b5mod6.cal')
        data1 = LoadCalFile(InstrumentName="POL", CalFilename=calfile1,
                            WorkspaceName=self.wsname)
        data2 = LoadCalFile(InstrumentName="POL", CalFilename=calfile2,
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
            self.assertTrue(isinstance(data2[3], ITableWorkspace))

        self.assertTrue("CalWorkspace2_group" in data2[0].getName())
        self.assertTrue("CalWorkspace2_offsets" in data2[1].getName())
        self.assertTrue("CalWorkspace2_mask" in data2[2].getName())
        self.assertTrue("CalWorkspace2_cal" in data2[3].getName())

    def test_nxsfile_with_workspace(self):
        self.wsname = "NexusWorkspace"
        nxsfile = (DIRS[0] + "POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets.nxs")
        data = LoadNexusProcessed(Filename=nxsfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data[0], WorkspaceGroup))
        self.assertEquals(5, data[0].getNumberOfEntries())

        for i in range(1, 6):
            self.assertTrue(isinstance(data[i], MatrixWorkspace))

        self.assertTrue('POL93107_newoffsets', self.wsname[0])

        for i in range(1, 6):
            self.assertTrue("ResultTOF-" + str(i) in data[i].getName())

    def test_gssfile_with_workspace(self):
        self.wsname = "GSSFile"
        gssfile = (DIRS[0] + "POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets.gss")
        data = LoadGSS(Filename=gssfile, OutputWorkspace=self.wsname)

        self.assertTrue(isinstance(data, MatrixWorkspace))
        self.assertEquals(5, data.getNumberHistograms())
        self.assertEquals(988, data.blocksize())
        self.assertAlmostEqual(221.59375, data.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.1326732445697, data.readY(0)[0], places=DIFF_PLACES)

    def test_datfile_with_workspace(self):
        dat_files = []
        for i in range(0, 5):
            dat_files.append(
                DIRS[0] + "POLARIS/test/Cycle_16_1/Mantid_tester/POL93107_newoffsets_b" + str(i + 1) +
                "_D.dat")
            dat_files.append(DIRS[0] + "POLARIS/test/Cycle_16_1/Mantid_tester/"
                                       "POL93107_newoffsets_b" + str(i + 1) + "_TOF.dat")

        dat_data = []
        for i in range(0, len(dat_files)):
            dat_data.append(LoadAscii(Filename=dat_files[i], OutputWorkspace=("datWorkspace" + str(i))))

        for data in dat_data:
            self.assertTrue(isinstance(data, MatrixWorkspace))
            self.assertEquals(1, data.getNumberHistograms())

        for i in range(0, len(dat_data), 2):
            b_size_avg = ((dat_data[i].blocksize() + dat_data[i + 1].blocksize()) / 2)
            self.assertEquals(b_size_avg, dat_data[i + 1].blocksize())

        self.assertAlmostEqual(0.30138, dat_data[0].readX(0)[2], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[0].readY(0)[6], dat_data[1].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(0.16076, dat_data[2].readX(0)[199], places=DIFF_PLACES)
        self.assertAlmostEqual(0.03525161, dat_data[3].readE(0)[10], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[4].readY(0)[2228], dat_data[5].readY(0)[2228], places=DIFF_PLACES)
        self.assertAlmostEqual(0.24750087, dat_data[5].readY(0)[0], places=DIFF_PLACES)

        self.assertAlmostEqual(0.01483319, dat_data[6].readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(dat_data[6].readY(0)[6], dat_data[7].readY(0)[6], places=DIFF_PLACES)

        self.assertAlmostEqual(dat_data[8].readE(0)[3369], dat_data[9].readE(0)[3369], places=DIFF_PLACES)
        self.assertAlmostEqual(300.55868, dat_data[9].readX(0)[10], places=DIFF_PLACES)
