# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Direct.PropertyManager import PropertyManager
from Direct.RunDescriptor import RunDescriptor

import os
from mantid import api
from mantid.api import mtd
from mantid.simpleapi import (
    config,
    AddSampleLog,
    CloneWorkspace,
    CompareWorkspaces,
    ConvertToEventWorkspace,
    CreateSampleWorkspace,
    DeleteWorkspace,
    ExtractMonitors,
    LoadEmptyInstrument,
    Rebin,
    RenameWorkspace,
)


# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorTest(unittest.TestCase):
    def __init__(self, methodName):
        self.prop_man = PropertyManager("MAR")
        return super(RunDescriptorTest, self).__init__(methodName)

    def setUp(self):
        if self.prop_man is None or type(self.prop_man) is not type(PropertyManager):
            self.prop_man = PropertyManager("MAR")

    def tearDown(self):
        api.AnalysisDataService.clear()

    @staticmethod
    def getInstrument(InstrumentName="MAR"):
        """test method used to obtain default instrument for testing"""
        idf_file = api.ExperimentInfo.getInstrumentFilename(InstrumentName)
        tmp_ws_name = "__empty_" + InstrumentName
        if not mtd.doesExist(tmp_ws_name):
            LoadEmptyInstrument(Filename=idf_file, OutputWorkspace=tmp_ws_name)
        return mtd[tmp_ws_name].getInstrument()

    def test_descr_basic(self):
        propman = PropertyManager("MAR")

        self.assertEqual(propman.sample_run, None)
        self.assertEqual(PropertyManager.sample_run.get_workspace(), None)

        propman.sample_run = 10
        self.assertEqual(propman.sample_run, 10)

        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", NumBanks=1, BankPixelWidth=4, NumEvents=100)
        propman.sample_run = "run_ws"
        self.assertEqual(PropertyManager.sample_run.run_number(), 0)

        stor_ws = PropertyManager.sample_run.get_workspace()
        rez = CompareWorkspaces(Workspace1=run_ws, Workspace2=stor_ws)

        self.assertTrue(rez[0])

        propman.sample_run = "MAR11001.RAW"
        self.assertFalse("run_ws" in mtd)
        self.assertEqual(PropertyManager.sample_run.run_number(), 11001)
        self.assertEqual(PropertyManager.sample_run._fext, ".RAW")

    def test_descr_dependend(self):
        propman = self.prop_man
        propman.wb_run = 100
        self.assertTrue(PropertyManager.wb_run.has_own_value())
        self.assertEqual(propman.wb_run, 100)
        self.assertEqual(propman.wb_for_monovan_run, 100)
        self.assertFalse(PropertyManager.wb_for_monovan_run.has_own_value())
        self.assertTrue(PropertyManager.wb_run.has_own_value())

        propman.wb_for_monovan_run = 200
        self.assertEqual(propman.wb_for_monovan_run, 200)
        self.assertEqual(propman.wb_run, 100)
        self.assertTrue(PropertyManager.wb_run.has_own_value())
        self.assertTrue(PropertyManager.wb_for_monovan_run.has_own_value())

        propman.wb_for_monovan_run = None
        self.assertFalse(PropertyManager.wb_for_monovan_run.has_own_value())
        self.assertEqual(propman.wb_for_monovan_run, 100)
        self.assertEqual(propman.wb_run, 100)

    def test_find_file(self):
        propman = self.prop_man
        propman.sample_run = 11001

        ok, file = PropertyManager.sample_run.find_file(propman)
        self.assertTrue(ok)
        self.assertGreater(len(file), 0)

        ext = PropertyManager.sample_run.get_fext()
        self.assertEqual(ext.casefold(), ".raw".casefold())

        PropertyManager.sample_run.set_file_ext("nxs")
        ext = PropertyManager.sample_run.get_fext()
        self.assertEqual(ext.casefold(), ".nxs".casefold())

        test_dir = config.getString("defaultsave.directory")

        testFile1 = os.path.normpath(test_dir + "MAR101111.nxs")
        testFile2 = os.path.normpath(test_dir + "MAR101111.raw")

        # create two test files to check search for appropriate extension
        f = open(testFile1, "w")
        f.write("aaaaaa")
        f.close()

        f = open(testFile2, "w")
        f.write("bbbb")
        f.close()

        propman.sample_run = 101111
        PropertyManager.sample_run.set_file_ext("nxs")

        ok, file = PropertyManager.sample_run.find_file(propman)
        self.assertEqual(testFile1, os.path.normpath(file))
        PropertyManager.sample_run.set_file_ext(".raw")
        ok, file = PropertyManager.sample_run.find_file(propman)
        self.assertEqual(testFile2, os.path.normpath(file))

        os.remove(testFile1)
        os.remove(testFile2)

    def test_alsk_one_find_another_ext_file(self):
        propman = self.prop_man
        propman.sample_run = 11001
        PropertyManager.sample_run.set_file_ext("nxs")

        ok, file = PropertyManager.sample_run.find_file(propman)
        self.assertTrue(ok)
        self.assertGreater(len(file), 12)

        ext = PropertyManager.sample_run.get_fext()
        self.assertEqual(ext.casefold(), ".raw".casefold())

    def test_alsk_one_find_another_ext_blocked(self):
        propman = self.prop_man
        propman.sample_run = 11001
        PropertyManager.sample_run.set_file_ext("raw")

        ok, file = PropertyManager.sample_run.find_file(propman, force_extension=".nxs")
        self.assertFalse(ok)
        self.assertEqual(file.strip(), "*** Rejecting file matching hint: MAR11001.nxs as found file has wrong extension: .raw")

    #
    def test_load_workspace(self):
        propman = self.prop_man

        # MARI run with number 11001 and extension raw must among unit test files
        propman.sample_run = 11001
        PropertyManager.sample_run.set_file_ext("nxs")

        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(PropertyManager.sample_run.get_fext(), ".raw")

        self.assertTrue(isinstance(ws, api.Workspace))

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(isinstance(mon_ws, api.Workspace))
        self.assertEqual(mon_ws.name(), ws.name())

    def test_copy_spectra2monitors(self):
        propman = self.prop_man
        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", WorkspaceType="Event", NumBanks=1, BankPixelWidth=5, NumEvents=100)
        run_ws_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", WorkspaceType="Histogram", NumBanks=2, BankPixelWidth=1, NumEvents=100
        )
        run_ws.setMonitorWorkspace(run_ws_monitors)

        self.assertEqual(run_ws_monitors.getNumberHistograms(), 2)

        propman.monovan_run = run_ws
        propman.spectra_to_monitors_list = 3

        mon_ws = PropertyManager.monovan_run.get_monitors_ws()
        self.assertTrue(isinstance(mon_ws, api.Workspace))
        self.assertEqual(mon_ws.getNumberHistograms(), 3)
        self.assertEqual(mon_ws.getIndexFromSpectrumNumber(3), 2)

    def test_copy_spectra2monitors_heterogen(self):
        propman = self.prop_man
        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", WorkspaceType="Event", NumBanks=1, BankPixelWidth=5, NumEvents=100)
        run_ws_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", WorkspaceType="Histogram", NumBanks=2, BankPixelWidth=1, NumEvents=100
        )
        run_ws_monitors = Rebin(run_ws_monitors, Params="1,-0.01,20000")
        run_ws.setMonitorWorkspace(run_ws_monitors)

        x = run_ws_monitors.readX(0)
        dx = x[1:] - x[:-1]
        min_step0 = min(dx)

        propman.monovan_run = run_ws
        propman.spectra_to_monitors_list = 3

        mon_ws = PropertyManager.monovan_run.get_monitors_ws()
        self.assertTrue(isinstance(mon_ws, api.Workspace))
        self.assertEqual(mon_ws.getNumberHistograms(), 3)
        self.assertEqual(mon_ws.getIndexFromSpectrumNumber(3), 2)

        x = mon_ws.readX(0)
        dx = x[1:] - x[:-1]
        min_step1 = min(dx)
        max_step1 = max(dx)

        self.assertAlmostEqual(min_step0, min_step1, 5)
        self.assertAlmostEqual(max_step1, min_step1, 5)

    def test_ws_name(self):
        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", NumBanks=1, BankPixelWidth=4, NumEvents=100)
        propman = self.prop_man
        propman.sample_run = run_ws

        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws.name(), "SR_run_ws")

        propman.sample_run = ws
        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws.name(), "SR_run_ws")
        self.assertTrue("SR_run_ws" in mtd)

        propman.sample_run = 11001
        self.assertFalse("SR_run_ws" in mtd)

        propman.load_monitors_with_workspace = False
        ws = PropertyManager.sample_run.get_workspace()
        ws_name = ws.name()
        self.assertEqual("SR_MAR011001", ws_name)
        self.assertTrue(ws_name in mtd)
        self.assertTrue(ws_name + "_monitors" in mtd)

        propman.sample_run = ws
        self.assertTrue(ws_name in mtd)

        ws1 = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws1.name(), ws_name)
        #
        PropertyManager.sample_run.set_action_suffix("_modified")
        PropertyManager.sample_run.synchronize_ws(ws1)

        ws1 = PropertyManager.sample_run.get_workspace()
        self.assertGreater(str.find(ws1.name(), "_modified"), 0)

        propman.sample_run = ws1
        self.assertEqual(ws1.name(), PropertyManager.sample_run._ws_name)

        ws = PropertyManager.sample_run.get_workspace()
        ws_name = ws.name()

        # if no workspace is available, attempt to get workspace name fails
        DeleteWorkspace(ws_name)

        propman.sample_run = None
        self.assertFalse(ws_name + "_monitors" in mtd)

    def test_assign_fname(self):
        propman = self.prop_man
        propman.sample_run = "MAR11001.RAW"

        self.assertEqual(PropertyManager.sample_run.run_number(), 11001)
        self.assertEqual(PropertyManager.sample_run._fext, ".RAW")

    def test_chop_ws_part(self):
        propman = self.prop_man
        ws = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=100, XUnit="TOF", XMin=2000, XMax=20000, BinWidth=1
        )

        CreateSampleWorkspace(
            Function="Multiple Peaks",
            NumBanks=4,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
            OutputWorkspace="ws_monitors",
        )

        propman.sample_run = ws

        ws1 = PropertyManager.sample_run.chop_ws_part(None, (2000, 1, 5000), False, 1, 2)

        rez = CompareWorkspaces("SR_ws", ws1)
        self.assertTrue(rez[0])

        wsc = PropertyManager.sample_run.get_workspace()
        x = wsc.readX(0)
        self.assertAlmostEqual(x[0], 2000)
        self.assertAlmostEqual(x[-1], 5000)

        self.assertEqual(wsc.name(), "SR_#1/2#ws")
        self.assertTrue("SR_#1/2#ws_monitors" in mtd)

        ws1 = PropertyManager.sample_run.chop_ws_part(ws1, (10000, 100, 20000), True, 2, 2)
        x = ws1.readX(0)
        self.assertAlmostEqual(x[0], 10000)
        self.assertAlmostEqual(x[-1], 20000)

        self.assertEqual(ws1.name(), "SR_#2/2#ws")
        self.assertTrue("SR_#2/2#ws_monitors" in mtd)

    def test_get_run_list(self):
        """verify the logic behind setting a run file as
        a sample run when run list is defined
        """
        propman = PropertyManager("MAR")
        propman.sample_run = [10204]

        self.assertEqual(propman.sample_run, 10204)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0], 10204)
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 0)
        self.assertEqual(sum_ws, None)
        self.assertEqual(n_sum, 0)

        # the same run list changes nothing
        propman.sample_run = [10204]
        self.assertEqual(propman.sample_run, 10204)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0], 10204)

        propman.sample_run = [11230, 10382, 10009]
        self.assertEqual(propman.sample_run, 11230)

        propman.sum_runs = True
        self.assertEqual(propman.sample_run, 10009)
        propman.sample_run = [11231, 10382, 10010]
        self.assertEqual(propman.sample_run, 10010)

        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 3)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(runs[0], sum_list[0])

        # Autoreduction workflow with summation. Runs appear
        # one by one and summed together when next run appears
        propman.sample_run = 11231
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()

        self.assertEqual(len(sum_list), 1)
        self.assertEqual(sum_list[0], 11231)
        self.assertEqual(propman.sample_run, 11231)

        propman.sample_run = 10382
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 2)
        self.assertEqual(sum_list[0], 11231)
        self.assertEqual(sum_list[1], 10382)
        self.assertEqual(propman.sample_run, 10382)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 3)

        propman.sample_run = 10010
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 3)
        self.assertEqual(sum_list[0], 11231)
        self.assertEqual(sum_list[1], 10382)
        self.assertEqual(sum_list[2], 10010)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 3)
        self.assertTrue(propman.sum_runs)

        # check asigning file with path
        propman.sample_run = "c:/data/MAR10382.nxs"
        self.assertEqual(propman.sample_run, 10382)
        self.assertEqual(PropertyManager.sample_run._run_list._last_ind2sum, 1)
        self.assertEqual(PropertyManager.sample_run._run_list._file_path[1], "c:/data")
        self.assertEqual(PropertyManager.sample_run._run_list._fext[1], ".nxs")

        # check extend when summing
        propman.sample_run = 10999
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 4)
        self.assertEqual(sum_list[0], 11231)
        self.assertEqual(sum_list[1], 10382)
        self.assertEqual(sum_list[2], 10010)
        self.assertEqual(sum_list[3], 10999)
        self.assertTrue(propman.sum_runs)

        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 4)
        self.assertTrue(propman.sum_runs)

        propman.sum_runs = False
        run_list = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(run_list), 4)
        self.assertEqual(propman.sample_run, 11231)
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 0)

        # check clear list when not summing
        propman.sample_run = 11999
        run_list = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(run_list), 1)

        # check if run list can be set up using range
        propman.sample_run = range(1, 4)
        run_list = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(run_list), 3)
        self.assertTrue(isinstance(run_list, list))

    def test_sum_runs(self):
        propman = self.prop_man
        propman.sample_run = [11001, 11001]
        ws = PropertyManager.sample_run.get_workspace()
        test_val1 = ws.dataY(3)[0]
        test_val2 = ws.dataY(6)[100]
        test_val3 = ws.dataY(50)[200]
        self.assertEqual(ws.name(), "SR_MAR011001")
        self.assertEqual(ws.getNEvents(), 2455286)

        # propman.sample_run = [11001,11001]
        propman.sum_runs = True
        self.assertFalse("SR_MAR011001" in mtd)
        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws.name(), "SR_MAR011001SumOf2")

        self.assertEqual(2 * test_val1, ws.dataY(3)[0])
        self.assertEqual(2 * test_val2, ws.dataY(6)[100])
        self.assertEqual(2 * test_val3, ws.dataY(50)[200])

        propman.sample_run = "MAR11001.raw,11001.nxs,MAR11001.raw"
        self.assertFalse("SR_MAR011001SumOf2" in mtd)
        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws.name(), "SR_MAR011001SumOf3")

        self.assertEqual(3 * test_val1, ws.dataY(3)[0])
        self.assertEqual(3 * test_val2, ws.dataY(6)[100])
        self.assertEqual(3 * test_val3, ws.dataY(50)[200])

        # TODO: Partial sum is not implemented. Should it?
        # propman.sum_runs = 2
        # propman.sample_run = "/home/my_path/MAR11001.raw,c:/somewhere/11001.nxs,MAR11001.raw"
        # self.assertFalse('SR_MAR011001SumOf3' in mtd)
        # self.assertFalse('SR_MAR011001SumOf2' in mtd)
        # ws = PropertyManager.sample_run.get_workspace()
        # self.assertEqual(ws.name(),'SR_MAR011001SumOf2')

        propman.sample_run = 11111
        sum_list, sum_ws, n_sum = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 4)

        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 4)
        self.assertEqual(runs[3], 11111)
        self.assertTrue(propman.sum_runs)
        self.assertTrue("SR_MAR011001SumOf3" in mtd)

        propman.cashe_sum_ws = True  # Not used at this stage but will be used at loading
        PropertyManager.sample_run._run_list.set_cashed_sum_ws(mtd["SR_MAR011001SumOf3"], "SumWS_cashe")
        self.assertTrue("SumWS_cashe" in mtd)
        self.assertFalse("SR_MAR011001SumOf3" in mtd)

        sum_list, sum_ws, n_summed = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sum_list), 1)
        self.assertEqual(n_summed, 3)
        self.assertEqual(sum_ws.name(), "SumWS_cashe")
        self.assertEqual(sum_list[0], 11111)

        # file 1 does not exist, so can not be found. Otherwise it should load it
        self.assertRaises(IOError, PropertyManager.sample_run.get_workspace)

        # Clear all
        propman.sum_runs = False
        self.assertFalse("SR_MAR011001SumOf3" in mtd)
        # disabling sum_runs clears sum cash
        self.assertFalse("SumWS_cashe" in mtd)
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 4)
        self.assertEqual(propman.sample_run, 11001)

        propman.sample_run = 10111
        runs = PropertyManager.sample_run.get_run_list()
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0], 10111)
        self.assertEqual(propman.sample_run, 10111)
        sums, ws, n_sums = PropertyManager.sample_run.get_runs_to_sum()
        self.assertEqual(len(sums), 0)
        self.assertEqual(ws, None)
        self.assertEqual(n_sums, 0)

    def test_find_runfiles(self):
        propman = self.prop_man
        propman.sample_run = [11001, 11111]
        files = PropertyManager.sample_run.get_run_file_list()
        self.assertEqual(len(files), 2)

        nf, found = PropertyManager.sample_run._run_list.find_run_files("MAR")
        self.assertEqual(len(nf), 1)
        self.assertEqual(len(found), 1)
        self.assertEqual(nf[0], 11111)

        files = PropertyManager.sample_run.get_run_file_list()
        self.assertEqual(len(files), 2)

    def test_add_masks(self):
        propman = self.prop_man
        ws = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=4,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )

        PropertyManager.sample_run.add_masked_ws(ws)

        masks, masked = PropertyManager.sample_run.get_masking()
        self.assertEqual(masked, 0)
        self.assertTrue(isinstance(masks, api.MatrixWorkspace))
        ws_name = PropertyManager.sample_run._mask_ws_name
        self.assertTrue(ws_name in mtd)

        masks = PropertyManager.sample_run.get_masking()
        self.assertTrue(isinstance(masks, api.MatrixWorkspace))
        ws_name = masks.name()
        self.assertTrue(ws_name in mtd)

        masks1 = PropertyManager.sample_run.get_masking(1)
        self.assertTrue(isinstance(masks1, api.MatrixWorkspace))

        propman.sample_run = None
        self.assertFalse(ws_name in mtd)

    def test_runDescriptorDependant(self):
        propman = self.prop_man
        self.assertTrue(PropertyManager.wb_run.has_own_value())
        propman.wb_for_monovan_run = None
        self.assertFalse(PropertyManager.wb_for_monovan_run.has_own_value())
        propman.wb_run = 2000
        self.assertEqual(propman.wb_for_monovan_run, 2000)
        self.assertEqual(propman.wb_run, 2000)
        self.assertFalse(PropertyManager.wb_for_monovan_run.has_own_value())
        propman.wb_for_monovan_run = None
        self.assertEqual(propman.wb_for_monovan_run, 2000)
        self.assertEqual(propman.wb_run, 2000)

        propman.wb_for_monovan_run = 3000
        self.assertTrue(PropertyManager.wb_for_monovan_run.has_own_value())
        self.assertEqual(propman.wb_run, 2000)
        self.assertEqual(propman.wb_for_monovan_run, 3000)

    def test_get_correct_fext(self):
        propman = self.prop_man
        propman.sample_run = None

        def_fext = propman.data_file_ext
        real_fext = PropertyManager.sample_run.get_fext()
        self.assertEqual(def_fext, real_fext)

        propman.data_file_ext = ".bla_bla"
        real_fext = PropertyManager.sample_run.get_fext()
        self.assertEqual(".bla_bla", real_fext)

        propman.sample_run = "MAR11001.nxs"
        real_fext = PropertyManager.sample_run.get_fext()
        self.assertEqual(".nxs", real_fext)

        propman.data_file_ext = ".raw"
        real_fext = PropertyManager.sample_run.get_fext()
        self.assertEqual(".nxs", real_fext)

    def test_change_normalization(self):
        propman = self.prop_man
        a_wksp = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=4,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )
        source_wksp = a_wksp / 5.0
        propman.sample_run = source_wksp
        self.assertRaises(RuntimeError, PropertyManager.sample_run.export_normalization, a_wksp)

        AddSampleLog(source_wksp, LogName="NormalizationFactor", LogText="5.", LogType="Number")
        PropertyManager.sample_run.export_normalization(a_wksp)

        rez = CompareWorkspaces(Workspace1=source_wksp, Workspace2=a_wksp, Tolerance=1.0e-8)
        self.assertTrue(rez[0])

        # divide by 20 to get final normalization factor equal 100 (ws/(5*20))
        a_wksp /= 20
        AddSampleLog(a_wksp, LogName="NormalizationFactor", LogText="100", LogType="Number")
        # export normalization by 5
        PropertyManager.sample_run.export_normalization(a_wksp)
        rez = CompareWorkspaces(Workspace1=source_wksp, Workspace2=a_wksp, Tolerance=1.0e-6)
        self.assertTrue(rez[0])
        self.assertAlmostEqual(a_wksp.getRun().getLogData("NormalizationFactor").value, 5.0)

        propman.sample_run = None
        DeleteWorkspace(a_wksp)

    def test_monitors_renamed(self):
        wksp = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=1,
            BankPixelWidth=1,
            NumEvents=1,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )
        CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Histogram",
            NumBanks=3,
            BankPixelWidth=1,
            NumEvents=1,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
            OutputWorkspace="wksp_monitors",
        )
        propman = self.prop_man

        propman.sample_run = wksp

        try:
            wksp.getMonitorWorkspace()
        except:
            self.fail()

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertEqual(mon_ws.name(), "SR_wksp_monitors")

        wsr = RenameWorkspace(wksp, OutputWorkspace="Renamed1", RenameMonitors=True)
        PropertyManager.sample_run.synchronize_ws(wsr)

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertEqual(mon_ws.name(), "SR_wksp_monitors")

        wsr.clearMonitorWorkspace()
        try:
            mon_ws = wksp.getMonitorWorkspace()
        except:
            pass
        else:
            self.fail()

        mon_ws = PropertyManager.sample_run.get_monitors_ws()

        try:
            mon_ws = wksp.getMonitorWorkspace()
        except:
            self.fail()

    def test_remove_background_matching_workspaces(self):
        wksp = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=3,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )
        CloneWorkspace(wksp, OutputWorkspace="bg_ws")
        AddSampleLog(Workspace=wksp, LogName="gd_prtn_chrg", LogText="10", LogType="Number")
        AddSampleLog(Workspace="bg_ws", LogName="gd_prtn_chrg", LogText="100", LogType="Number")
        CloneWorkspace("bg_ws", OutputWorkspace="ref_ws")

        self.assertFalse(wksp.run().hasProperty("empty_bg_removed"))

        propman = self.prop_man

        propman.sample_run = wksp
        propman.empty_bg_run = "bg_ws"

        bgws = PropertyManager.empty_bg_run.get_workspace()
        PropertyManager.sample_run.remove_empty_background(bgws)
        ws = PropertyManager.sample_run.get_workspace()

        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        # the subtaction occurs only once
        PropertyManager.sample_run.remove_empty_background(bgws)
        # operation above does nothing
        ws = PropertyManager.sample_run.get_workspace()
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)
        # ensure that bg Workspace remains unchanged
        bg_ws_restored = PropertyManager.empty_bg_run.get_workspace()
        difr = CompareWorkspaces(bg_ws_restored, "ref_ws")
        self.assertTrue(difr.Result)

    def test_add_emptpy_sectra_first(self):
        CreateSampleWorkspace(
            OutputWorkspace="wksp",
            Function="Multiple Peaks",
            WorkspaceType="Histogram",
            NumBanks=3,
            NumMonitors=3,
            BankPixelWidth=1,
            XUnit="TOF",
            XMin=200,
            XMax=20000,
            BinWidth=200,
        )
        ExtractMonitors(InputWorkspace="wksp", DetectorWorkspace="wksp", MonitorWorkspace="Monitors")

        wksp = mtd["wksp"]
        mod_ws = RunDescriptor._add_empty_spectra(wksp, 3, True)
        self.assertEqual(mod_ws.getNumberHistograms(), 6)
        self.assertEqual("wksp", mod_ws.name())

    def test_add_emptpy_sectra_last(self):
        CreateSampleWorkspace(
            OutputWorkspace="wksp",
            Function="Multiple Peaks",
            WorkspaceType="Histogram",
            NumBanks=3,
            NumMonitors=3,
            BankPixelWidth=1,
            XUnit="TOF",
            XMin=200,
            XMax=20000,
            BinWidth=200,
        )
        ExtractMonitors(InputWorkspace="wksp", DetectorWorkspace="wksp", MonitorWorkspace="Monitors")

        wksp = mtd["Monitors"]
        mod_ws = RunDescriptor._add_empty_spectra(wksp, 3, False)
        self.assertEqual(mod_ws.getNumberHistograms(), 6)
        self.assertEqual("Monitors", mod_ws.name())

    def test_remove_background_matrix_minus_event(self):
        # Verify situation if sampe workspace is in histogram mode with monitors loaded with workspace
        # and background is event workspace with monitors loaded separately
        wksp = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Histogram",
            NumBanks=3,
            NumMonitors=3,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )
        ExtractMonitors(InputWorkspace="wksp", DetectorWorkspace="bg_ws", MonitorWorkspace="refMonitors")

        CloneWorkspace("bg_ws", OutputWorkspace="ref_ws")
        bg_ws = ConvertToEventWorkspace("bg_ws")

        AddSampleLog(Workspace=wksp, LogName="gd_prtn_chrg", LogText="10", LogType="Number")
        AddSampleLog(Workspace=bg_ws, LogName="gd_prtn_chrg", LogText="100", LogType="Number")

        self.assertFalse(wksp.run().hasProperty("empty_bg_removed"))

        propman = self.prop_man

        propman.sample_run = wksp
        propman.empty_bg_run = bg_ws

        bgws = PropertyManager.empty_bg_run.get_workspace()
        PropertyManager.sample_run.remove_empty_background(bgws)
        resWs = PropertyManager.sample_run.get_workspace()
        self.assertTrue(resWs.run().hasProperty("empty_bg_removed"))

        ExtractMonitors(InputWorkspace=resWs, DetectorWorkspace="resWs", MonitorWorkspace="resMonitors")
        difr_mon = CompareWorkspaces("refMonitors", "resMonitors")
        self.assertTrue(difr_mon.Result)

        ref_ws = mtd["ref_ws"]

        # Errors if two matrix workspaces extacted from each other are not extracted
        ref_ws = ref_ws - 0.1 * ref_ws

        resWs = mtd["resWs"]

        difr_sample = CompareWorkspaces(ref_ws, resWs, Tolerance=1.0e-7)
        self.assertTrue(difr_sample.Result)

    def test_remove_background_matrix_minus_matrix(self):
        # Verify situation if sample workspace is in event mode and
        # and background loaded as a histogram with monitors included
        bg_ws = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Histogram",
            NumBanks=3,
            NumMonitors=3,
            BankPixelWidth=1,
            NumEvents=100,
            XUnit="TOF",
            XMin=2000,
            XMax=20000,
            BinWidth=1,
        )
        ExtractMonitors(InputWorkspace="bg_ws", DetectorWorkspace="sample_run", MonitorWorkspace="refMonitors")
        ref_ws = CloneWorkspace("sample_run")

        AddSampleLog(Workspace="sample_run", LogName="gd_prtn_chrg", LogText="10", LogType="Number")
        AddSampleLog(Workspace=bg_ws, LogName="gd_prtn_chrg", LogText="100", LogType="Number")

        wksp = mtd["sample_run"]
        self.assertFalse(wksp.run().hasProperty("empty_bg_removed"))

        propman = self.prop_man

        propman.sample_run = wksp
        propman.empty_bg_run = bg_ws

        bgws = PropertyManager.empty_bg_run.get_workspace()
        PropertyManager.sample_run.remove_empty_background(bgws)
        resWs = PropertyManager.sample_run.get_workspace()
        self.assertTrue(resWs.run().hasProperty("empty_bg_removed"))

        # Errors if two matrix workspaces extacted from each other are not extracted
        ref_ws = ref_ws - 0.1 * ref_ws
        difr = CompareWorkspaces(ref_ws, resWs)
        self.assertTrue(difr.Result)
        # bg Workspace remains unchanged
        bg_ws_restored = PropertyManager.empty_bg_run.get_workspace()
        difr = CompareWorkspaces(bg_ws_restored, bg_ws)
        self.assertTrue(difr.Result)


if __name__ == "__main__" or __name__ == "mantidqt.widgets.codeeditor.execution":
    # tester=RunDescriptorTest('test_remove_background_matching_workspaces')
    # tester.run()
    unittest.main()
