# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import Direct.dgreduce as dgreduce
from Direct.DirectEnergyConversion import DirectEnergyConversion
from Direct.PropertyManager import PropertyManager

from mantid import api
from mantid.simpleapi import *


# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------


class DirectEnergyConversionTest(unittest.TestCase):
    def __init__(self, methodName):
        self.reducer = None
        return super(DirectEnergyConversionTest, self).__init__(methodName)

    def setUp(self):
        if self.reducer is None or type(self.reducer) is not type(DirectEnergyConversion):
            self.reducer = DirectEnergyConversion("MAR")

    def tearDown(self):
        api.AnalysisDataService.clear()
        pass

    def test_init_reducer(self):
        tReducer = self.reducer
        self.assertNotEqual(tReducer.prop_man, None)

        prop_man = tReducer.prop_man
        self.assertEqual(prop_man.instr_name, "MARI")

    def test_save_formats(self):
        tReducer = self.reducer

        files = ["save_formats_test_file.spe", "save_formats_test_file.nxspe" "save_formats_test_file", "save_formats_test_file.nxs"]

        def clean_up(file_list):
            for file in file_list:
                file = FileFinder.getFullPath(file)
                if len(file) > 0:
                    os.remove(file)

        def verify_absent(file_list):
            for file in file_list:
                file = FileFinder.getFullPath(file)
                self.assertEqual(len(file), 0)

        def verify_present_and_delete(file_list):
            for file in file_list:
                file = FileFinder.getFullPath(file)
                self.assertGreater(len(file), 0)
                os.remove(file)

        clean_up(files)
        tReducer.prop_man.save_format = ""

        tws = CreateSampleWorkspace(
            Function="Flat background", NumBanks=1, BankPixelWidth=1, NumEvents=10, XUnit="DeltaE", XMin=-10, XMax=10, BinWidth=0.1
        )

        self.assertEqual(len(tReducer.prop_man.save_format), 0)
        # do nothing
        tReducer.save_results(tws, "save_formats_test_file")
        #
        verify_absent(files)

        # redefine test save methods to produce test output
        tReducer.prop_man.save_format = ["spe", "nxspe", "nxs"]
        tReducer.save_results(tws, "save_formats_test_file.tt")

        files = ["save_formats_test_file.spe", "save_formats_test_file.nxspe", "save_formats_test_file.nxs"]
        verify_present_and_delete(files)

        tReducer.prop_man.save_format = None
        # do nothing
        tReducer.save_results(tws, "save_formats_test_file.tt")
        file = FileFinder.getFullPath("save_formats_test_file.tt")
        self.assertEqual(len(file), 0)

        # save file with given extension on direct request:
        tReducer.save_results(tws, "save_formats_test_file.nxs")
        verify_present_and_delete(["save_formats_test_file.nxs"])

        tReducer.prop_man.save_format = []
        # do nothing
        tReducer.save_results(tws, "save_formats_test_file")
        file = FileFinder.getFullPath("save_formats_test_file")
        self.assertEqual(len(file), 0)

        # save files with extensions on request
        tReducer.save_results(tws, "save_formats_test_file", ["nxs", ".nxspe"])
        verify_present_and_delete(["save_formats_test_file.nxspe", "save_formats_test_file.nxs"])

        # this is strange feature.
        self.assertEqual(len(tReducer.prop_man.save_format), 2)

    def test_diagnostics_wb(self):
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        LoadInstrument(wb_ws, InstrumentName="MARI", RewriteSpectraMap=True)

        tReducer = DirectEnergyConversion(wb_ws.getInstrument())

        mask_workspace = tReducer.diagnose(wb_ws)
        self.assertTrue(mask_workspace)

        api.AnalysisDataService.clear()

    def test_do_white_wb(self):
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        # LoadParameterFile(Workspace=wb_ws,ParameterXML = used_parameters)
        LoadInstrument(wb_ws, InstrumentName="MARI", RewriteSpectraMap=True)

        tReducer = DirectEnergyConversion(wb_ws.getInstrument())

        white_ws = tReducer.do_white(wb_ws, None, None)
        self.assertTrue(white_ws)

    def test_get_set_attributes(self):
        tReducer = self.reducer

        # prohibit accessing non-existing property
        self.assertRaises(KeyError, getattr, tReducer, "non_existing_property")
        self.assertRaises(KeyError, setattr, tReducer, "non_existing_property", 1000)
        self.assertRaises(KeyError, getattr, tReducer, "non_existing_property")

        # allow simple creation of a system property
        self.assertRaises(KeyError, getattr, tReducer, "_new_system_property")
        setattr(tReducer, "_new_system_property", True)
        self.assertTrue(tReducer._new_system_property)

        # direct and indirect access to prop_man properties
        tReducer.sample_run = None
        # sample run has not been defined
        self.assertEqual(getattr(tReducer, "sample_run"), None)
        prop_man = tReducer.prop_man
        self.assertEqual(getattr(prop_man, "sample_run"), None)
        # define sample run
        tReducer.sample_run = 10234
        self.assertEqual(tReducer.sample_run, 10234)
        self.assertEqual(tReducer.prop_man.sample_run, 10234)

    def test_get_abs_normalization_factor(self):
        mono_ws = CreateSampleWorkspace(
            NumBanks=1, BankPixelWidth=4, NumEvents=10000, XUnit="DeltaE", XMin=-5, XMax=15, BinWidth=0.1, function="Flat background"
        )
        LoadInstrument(mono_ws, InstrumentName="MARI", RewriteSpectraMap=True)

        tReducer = DirectEnergyConversion(mono_ws.getInstrument())
        tReducer.prop_man.incident_energy = 5.0
        tReducer.prop_man.monovan_integr_range = [-10, 10]
        tReducer.wb_run = mono_ws

        (nf1, nf2, nf3, nf4) = tReducer.get_abs_normalization_factor(PropertyManager.wb_run, 5.0)
        self.assertAlmostEqual(nf1, 0.58561121802167193, 7)
        self.assertAlmostEqual(nf1, nf2)
        self.assertAlmostEqual(nf2, nf3)
        self.assertAlmostEqual(nf3, nf4)

        # check warning. WB spectra with 0 signal indicate troubles.
        mono_ws = CreateSampleWorkspace(
            NumBanks=1, BankPixelWidth=4, NumEvents=10000, XUnit="DeltaE", XMin=-5, XMax=15, BinWidth=0.1, function="Flat background"
        )
        LoadInstrument(mono_ws, InstrumentName="MARI", RewriteSpectraMap=True)
        sig = mono_ws.dataY(0)
        sig[:] = 0

        tReducer.wb_run = mono_ws
        (nf1, nf2, nf3, nf4) = tReducer.get_abs_normalization_factor(PropertyManager.wb_run, 5.0)
        self.assertAlmostEqual(nf1, 0.585611218022, 7)
        self.assertAlmostEqual(nf1, nf2)
        self.assertAlmostEqual(nf2, nf3)
        self.assertAlmostEqual(nf3, nf4)

    def test_dgreduce_works(self):
        """Test for old interface"""
        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        LoadInstrument(run_ws, InstrumentName="MARI", RewriteSpectraMap=True)

        # mono_ws = CloneWorkspace(run_ws)
        wb_ws = CloneWorkspace(run_ws)
        AddSampleLog(wb_ws, LogName="run_number", LogText="300", LogType="Number")
        # wb_ws=CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=10000)

        dgreduce.setup("MAR")
        par = {}
        par["ei_mon_spectra"] = [4, 5]
        par["abs_units_van_range"] = [-4000, 8000]
        # overwrite parameters, which are necessary from command line, but we want them to have test values
        dgreduce.getReducer().map_file = None
        dgreduce.getReducer().monovan_mapfile = None
        dgreduce.getReducer().mono_correction_factor = 1
        # abs_units(wb_for_run,sample_run,monovan_run,wb_for_monovanadium,samp_rmm,samp_mass,ei_guess,rebin,
        # map_file='default',monovan_mapfile='default',**kwargs):
        ws = dgreduce.abs_units(wb_ws, run_ws, None, wb_ws, 10, 100, 8.8, [-10, 0.1, 7], None, None, **par)
        self.assertTrue(isinstance(ws, api.MatrixWorkspace))

    def test_dgreduce_works_with_name(self):
        """Test for old interface"""
        run_ws = CreateSampleWorkspace(Function="Multiple Peaks", NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        LoadInstrument(run_ws, InstrumentName="MARI", RewriteSpectraMap=True)
        AddSampleLog(run_ws, LogName="run_number", LogText="200", LogType="Number")
        # mono_ws = CloneWorkspace(run_ws)
        wb_ws = CloneWorkspace(run_ws)
        AddSampleLog(wb_ws, LogName="run_number", LogText="100", LogType="Number")
        # wb_ws=CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=10000)

        dgreduce.setup("MAR")
        par = {}
        par["ei_mon_spectra"] = [4, 5]
        par["abs_units_van_range"] = [-4000, 8000]
        # overwrite parameters, which are necessary from command line, but we want them to have test values
        dgreduce.getReducer().map_file = None
        dgreduce.getReducer().monovan_mapfile = None
        dgreduce.getReducer().mono_correction_factor = 1
        # abs_units(wb_for_run,sample_run,monovan_run,wb_for_monovanadium,samp_rmm,samp_mass,ei_guess,rebin,
        # map_file='default',monovan_mapfile='default',**kwargs):
        ws = dgreduce.abs_units("wb_ws", "run_ws", None, wb_ws, 10, 100, 8.8, [-10, 0.1, 7], None, None, **par)
        self.assertTrue(isinstance(ws, api.MatrixWorkspace))

    ##    tReducet.di
    def test_energy_to_TOF_range(self):
        ws = Load(Filename="MAR11001.raw", LoadMonitors="Include")

        en_range = [0.8 * 13, 13, 1.2 * 13]
        detIDs = [1, 2, 3, 10]
        red = DirectEnergyConversion()
        TRange = red.get_TOF_for_energies(ws, en_range, detIDs)
        for ind, detID in enumerate(detIDs):
            tof = TRange[ind]
            y = [1] * (len(tof) - 1)
            ind = ws.getIndexFromSpectrumNumber(detID)
            ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace="_ws_template", WorkspaceIndex=ind)
            CreateWorkspace(OutputWorkspace="TOF_WS", NSpec=1, DataX=tof, DataY=y, UnitX="TOF", ParentWorkspace="_ws_template")
            EnWs = ConvertUnits(InputWorkspace="TOF_WS", Target="Energy", EMode="Elastic")

            eni = EnWs.dataX(0)
            for samp, rez in zip(eni, en_range):
                self.assertAlmostEqual(samp, rez)

        # Now Test shifted:
        ei, mon1_peak, mon1_index, tzero = GetEi(InputWorkspace=ws, Monitor1Spec=int(2), Monitor2Spec=int(3), EnergyEstimate=13)
        ScaleX(InputWorkspace="ws", OutputWorkspace="ws", Operation="Add", Factor=-mon1_peak, InstrumentParameter="DelayTime", Combine=True)
        ws = mtd["ws"]

        mon1_det = ws.getDetector(1)
        mon1_pos = mon1_det.getPos()
        src_name = ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(
            Workspace="ws", ComponentName=src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False
        )

        # Does not work for monitor 2 as it has been moved to mon2 position and there all tof =0
        detIDs = [1, 3, 10]
        TRange1 = red.get_TOF_for_energies(ws, en_range, detIDs)

        for ind, detID in enumerate(detIDs):
            tof = TRange1[ind]
            y = [1] * (len(tof) - 1)
            ind = ws.getIndexFromSpectrumNumber(detID)
            ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace="_ws_template", WorkspaceIndex=ind)
            CreateWorkspace(OutputWorkspace="TOF_WS", NSpec=1, DataX=tof, DataY=y, UnitX="TOF", ParentWorkspace="_ws_template")
            EnWs = ConvertUnits(InputWorkspace="TOF_WS", Target="Energy", EMode="Elastic")

            eni = EnWs.dataX(0)
            for samp, rez in zip(eni, en_range):
                self.assertAlmostEqual(samp, rez)

    def test_late_rebinning(self):
        run_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=100000, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )
        LoadInstrument(run_monitors, InstrumentName="MARI", RewriteSpectraMap=True)
        ConvertUnits(InputWorkspace="run_monitors", OutputWorkspace="run_monitors", Target="TOF")
        run_monitors = mtd["run_monitors"]
        tof = run_monitors.dataX(3)
        tMin = tof[0]
        tMax = tof[-1]
        run = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=8,
            BankPixelWidth=1,
            NumEvents=100000,
            XUnit="TOF",
            xMin=tMin,
            xMax=tMax,
        )
        LoadInstrument(run, InstrumentName="MARI", RewriteSpectraMap=True)

        run.setMonitorWorkspace(run_monitors)

        wb_ws = Rebin(run, Params=[tMin, 1, tMax], PreserveEvents=False)

        # References used to test against ordinary reduction
        ref_ws = Rebin(run, Params=[tMin, 1, tMax], PreserveEvents=False)
        ref_ws_monitors = CloneWorkspace("run_monitors")
        ref_ws.setMonitorWorkspace(ref_ws_monitors)
        # just in case, wb should work without clone too.
        wb_clone = CloneWorkspace(wb_ws)

        # Run Mono
        tReducer = DirectEnergyConversion(run.getInstrument())
        tReducer.energy_bins = [-20, 0.2, 60]
        ei_guess = 67.0
        mono_s = tReducer.mono_sample(run, ei_guess, wb_ws)

        #
        mono_ref = tReducer.mono_sample(ref_ws, ei_guess, wb_clone)

        rez = CompareWorkspaces(mono_s, mono_ref)
        self.assertTrue(rez[0])

    def test_tof_range(self):
        run = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=6, BankPixelWidth=1, NumEvents=10, XUnit="Energy", XMin=5, XMax=75, BinWidth=0.2
        )
        LoadInstrument(run, InstrumentName="MARI", RewriteSpectraMap=True)

        red = DirectEnergyConversion(run.getInstrument())

        red.prop_man.incident_energy = 26.2
        red.prop_man.energy_bins = [-20, 0.1, 20]
        red.prop_man.multirep_tof_specta_list = [4, 5, 6]
        MoveInstrumentComponent(Workspace="run", ComponentName="Detector", DetectorID=1102, Z=3)
        MoveInstrumentComponent(Workspace="run", ComponentName="Detector", DetectorID=1103, Z=6)

        run_tof = ConvertUnits(run, Target="TOF", EMode="Elastic")

        tof_range = red.find_tof_range_for_multirep(run_tof)

        self.assertEqual(len(tof_range), 3)

        x = run_tof.readX(3)
        xMin = min(x)
        x = run_tof.readX(5)
        xMax = max(x)

        self.assertGreater(tof_range[0], xMin)
        # self.assertAlmostEqual(tof_range[1],dt)
        self.assertLess(tof_range[2], xMax)

        # check another working mode
        red.prop_man.multirep_tof_specta_list = 4
        red.prop_man.incident_energy = 47.505
        red.prop_man.energy_bins = [-20, 0.1, 45]

        tof_range1 = red.find_tof_range_for_multirep(run_tof)

        self.assertGreater(tof_range1[0], xMin)
        self.assertLess(tof_range1[2], xMax)

        self.assertLess(tof_range1[2], tof_range[2])
        self.assertLess(tof_range1[0], tof_range[0])
        self.assertLess(tof_range1[1], tof_range[1])

    def test_multirep_mode(self):
        # create test workspace
        run_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=100000, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )
        LoadInstrument(run_monitors, InstrumentName="MARI", RewriteSpectraMap=True)
        ConvertUnits(InputWorkspace="run_monitors", OutputWorkspace="run_monitors", Target="TOF")
        run_monitors = mtd["run_monitors"]
        tof = run_monitors.dataX(3)
        tMin = tof[0]
        tMax = tof[-1]
        run = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=8,
            BankPixelWidth=1,
            NumEvents=100000,
            XUnit="TOF",
            xMin=tMin,
            xMax=tMax,
        )
        LoadInstrument(run, InstrumentName="MARI", RewriteSpectraMap=True)
        MoveInstrumentComponent(Workspace="run", ComponentName="Detector", DetectorID=1102, Z=1)
        # MoveInstrumentComponent(Workspace='run', ComponentName='Detector', DetectorID=1103,Z=4)
        # MoveInstrumentComponent(Workspace='run', ComponentName='Detector', DetectorID=1104,Z=5)

        # do second
        run2 = CloneWorkspace(run)
        CloneWorkspace(run_monitors, OutputWorkspace="run2_monitors")

        wb_ws = Rebin(run, Params=[tMin, 1, tMax], PreserveEvents=False)

        # Run multirep
        tReducer = DirectEnergyConversion(run.getInstrument())
        tReducer.prop_man.run_diagnostics = True
        tReducer.hard_mask_file = None
        tReducer.map_file = None
        tReducer.save_format = None
        tReducer.multirep_tof_specta_list = [4, 5]

        result = tReducer.convert_to_energy(wb_ws, run, [67.0, 122.0], [-2, 0.02, 0.8])

        self.assertEqual(len(result), 2)

        ws1 = result[0]
        self.assertEqual(ws1.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws1.readX(0)
        self.assertAlmostEqual(x[0], -2 * 67.0)
        self.assertAlmostEqual(x[-1], 0.8 * 67.0)

        ws2 = result[1]
        self.assertEqual(ws2.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws2.readX(0)
        self.assertAlmostEqual(x[0], -2 * 122.0)
        self.assertAlmostEqual(x[-1], 0.8 * 122.0)

        # test another ws
        # rename samples from previous workspace to avoid deleting them on current run
        for ind, item in enumerate(result):
            result[ind] = RenameWorkspace(item, OutputWorkspace="SampleRez#" + str(ind))
        #
        result2 = tReducer.convert_to_energy(None, run2, [67.0, 122.0], [-2, 0.02, 0.8])

        rez = CompareWorkspaces(result[0], result2[0])
        self.assertTrue(rez[0])
        rez = CompareWorkspaces(result[1], result2[1])
        self.assertTrue(rez[0])

    def test_multirep_abs_units_mode(self):
        # create test workspace
        run_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=100000, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )
        LoadInstrument(run_monitors, InstrumentName="MARI", RewriteSpectraMap=True)
        ConvertUnits(InputWorkspace="run_monitors", OutputWorkspace="run_monitors", Target="TOF")
        run_monitors = mtd["run_monitors"]
        tof = run_monitors.dataX(3)
        tMin = tof[0]
        tMax = tof[-1]
        run = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=8,
            BankPixelWidth=1,
            NumEvents=100000,
            XUnit="TOF",
            xMin=tMin,
            xMax=tMax,
        )
        LoadInstrument(run, InstrumentName="MARI", RewriteSpectraMap=True)

        # build "monovanadium"
        mono = CloneWorkspace(run)
        CloneWorkspace(run_monitors, OutputWorkspace="mono_monitors")

        # build "White-beam"
        wb_ws = Rebin(run, Params=[tMin, 1, tMax], PreserveEvents=False)

        # build "second run" to ensure repeated execution
        run2 = CloneWorkspace(run)
        CloneWorkspace(run_monitors, OutputWorkspace="run2_monitors")

        # Run multirep
        tReducer = DirectEnergyConversion(run.getInstrument())
        tReducer.prop_man.run_diagnostics = True
        tReducer.hard_mask_file = None
        tReducer.map_file = None
        tReducer.prop_man.background_range = [0.99 * tMax, tMax]
        tReducer.prop_man.monovan_mapfile = None
        tReducer.save_format = None
        tReducer.prop_man.normalise_method = "monitor-1"
        tReducer.norm_mon_integration_range = [tMin, tMax]

        result = tReducer.convert_to_energy(wb_ws, run, [67.0, 122.0], [-2, 0.02, 0.8], None, mono)

        self.assertEqual(len(result), 2)

        ws1 = result[0]
        self.assertEqual(ws1.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws1.readX(0)
        self.assertAlmostEqual(x[0], -2 * 67.0)
        self.assertAlmostEqual(x[-1], 0.8 * 67.0)

        ws2 = result[1]
        self.assertEqual(ws2.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws2.readX(0)
        self.assertAlmostEqual(x[0], -2 * 122.0)
        self.assertAlmostEqual(x[-1], 0.8 * 122.0)

        # test another ws
        # rename samples from previous workspace to avoid deleting them on current run
        for ind, item in enumerate(result):
            result[ind] = RenameWorkspace(item, OutputWorkspace="SampleRez#" + str(ind))
        #
        result2 = tReducer.convert_to_energy(None, run2)

        rez = CompareWorkspaces(result[0], result2[0])
        self.assertTrue(rez[0])
        rez = CompareWorkspaces(result[1], result2[1])
        self.assertTrue(rez[0])

    def test_abs_multirep_with_bkg_and_bleed(self):
        # create test workspace
        run_monitors = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=100000, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )
        LoadInstrument(run_monitors, InstrumentName="MARI", RewriteSpectraMap=True)
        ConvertUnits(InputWorkspace="run_monitors", OutputWorkspace="run_monitors", Target="TOF")
        run_monitors = mtd["run_monitors"]
        tof = run_monitors.dataX(3)
        tMin = tof[0]
        tMax = tof[-1]
        run = CreateSampleWorkspace(
            Function="Multiple Peaks",
            WorkspaceType="Event",
            NumBanks=8,
            BankPixelWidth=1,
            NumEvents=100000,
            XUnit="TOF",
            xMin=tMin,
            xMax=tMax,
        )
        LoadInstrument(run, InstrumentName="MARI", RewriteSpectraMap=True)
        AddSampleLog(run, LogName="gd_prtn_chrg", LogText="1.", LogType="Number")
        run.setMonitorWorkspace(run_monitors)

        # build "monovanadium"
        mono = CloneWorkspace(run)
        mono_monitors = CloneWorkspace(run_monitors)
        mono.setMonitorWorkspace(mono_monitors)

        # build "White-beam"
        wb_ws = Rebin(run, Params=[tMin, 1, tMax], PreserveEvents=False)

        # build "second run" to ensure repeated execution
        run2 = CloneWorkspace(run)
        run2_monitors = CloneWorkspace(run_monitors)
        run2.setMonitorWorkspace(run2_monitors)

        # Run multirep
        tReducer = DirectEnergyConversion(run.getInstrument())
        tReducer.prop_man.run_diagnostics = True
        tReducer.hard_mask_file = None
        tReducer.map_file = None
        tReducer.prop_man.check_background = True
        tReducer.prop_man.background_range = [0.99 * tMax, tMax]
        tReducer.prop_man.monovan_mapfile = None
        tReducer.save_format = None
        tReducer.prop_man.normalise_method = "monitor-2"

        tReducer.prop_man.bleed = True
        tReducer.norm_mon_integration_range = [tMin, tMax]

        AddSampleLog(run, LogName="good_frames", LogText="1.", LogType="Number Series")
        result = tReducer.convert_to_energy(wb_ws, run, [67.0, 122.0], [-2, 0.02, 0.8], None, mono)

        self.assertEqual(len(result), 2)

        ws1 = result[0]
        self.assertEqual(ws1.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws1.readX(0)
        self.assertAlmostEqual(x[0], -2 * 67.0)
        self.assertAlmostEqual(x[-1], 0.8 * 67.0)

        ws2 = result[1]
        self.assertEqual(ws2.getAxis(0).getUnit().unitID(), "DeltaE")
        x = ws2.readX(0)
        self.assertAlmostEqual(x[0], -2 * 122.0)
        self.assertAlmostEqual(x[-1], 0.8 * 122.0)

        # test another ws
        # rename samples from previous workspace to avoid deleting them on current run
        for ind, item in enumerate(result):
            result[ind] = RenameWorkspace(item, OutputWorkspace="SampleRez#" + str(ind))
        #
        AddSampleLog(run2, LogName="goodfrm", LogText="1", LogType="Number")
        result2 = tReducer.convert_to_energy(None, run2)

        rez = CompareWorkspaces(result[0], result2[0])
        self.assertTrue(rez[0])
        rez = CompareWorkspaces(result[1], result2[1])
        self.assertTrue(rez[0])

    def test_sum_monitors(self):
        # create test workspace
        monitor_ws = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=6, BankPixelWidth=1, NumEvents=100000, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )
        ConvertUnits(InputWorkspace=monitor_ws, OutputWorkspace="monitor_ws", Target="TOF")

        # Rebin to "formally" make common bin boundaries as it is not considered as such
        # any more after converting units (Is this a bug?)
        xx = monitor_ws.readX(0)
        x_min = min(xx[0], xx[-1])
        x_max = max(xx[0], xx[-1])
        x_step = (x_max - x_min) / (len(xx) - 1)
        monitor_ws = Rebin(monitor_ws, Params=[x_min, x_step, x_max])
        #
        # keep this workspace for second test below -- clone and give
        # special name for RunDescriptor to recognize as monitor workspace for
        # fake data workspace we will provide.
        CloneWorkspace(monitor_ws, OutputWorkspace="_TMPmonitor_ws_monitors")

        # Estimate energy from two monitors
        ei, mon1_peak, mon1_index, tzero = GetEi(
            InputWorkspace=monitor_ws, Monitor1Spec=1, Monitor2Spec=4, EnergyEstimate=62.2, FixEi=False
        )
        self.assertAlmostEqual(ei, 62.1449, 3)

        # Provide instrument parameter, necessary to define
        # DirectEnergyConversion class properly
        SetInstrumentParameter(monitor_ws, ParameterName="fix_ei", ParameterType="Number", Value="0")
        SetInstrumentParameter(monitor_ws, DetectorList=[1, 2, 3, 6], ParameterName="DelayTime", ParameterType="Number", Value="0.5")
        SetInstrumentParameter(monitor_ws, ParameterName="mon2_norm_spec", ParameterType="Number", Value="1")

        # initiate test reducer
        tReducer = DirectEnergyConversion(monitor_ws.getInstrument())
        tReducer.prop_man.ei_mon_spectra = ([1, 2, 3], 6)
        tReducer.prop_man.normalise_method = "current"
        tReducer.prop_man.mon2_norm_spec = 2
        ei_mon_spectra = tReducer.prop_man.ei_mon_spectra
        ei_mon_spectra, monitor_ws = tReducer.sum_monitors_spectra(monitor_ws, ei_mon_spectra)
        #
        # Check GetEi with summed monitors. Try to run separately.
        ei1, mon1_peak, mon1_index, tzero = GetEi(
            InputWorkspace=monitor_ws, Monitor1Spec=1, Monitor2Spec=6, EnergyEstimate=62.2, FixEi=False
        )
        self.assertAlmostEqual(ei1, ei, 2)

        # Second test Check get_ei as part of the reduction
        tReducer.prop_man.ei_mon_spectra = ([1, 2, 3], [4, 5, 6])
        tReducer.prop_man.fix_ei = False
        # DataWorkspace == monitor_ws data workspace is not used anyway. The only thing we
        # use it for is to retrieve monitor workspace from Mantid using its name
        ei2, mon1_peak2 = tReducer.get_ei(monitor_ws, 62.2)
        self.assertAlmostEqual(ei2, 64.95, 2)

        ei2b, mon1_peak2 = tReducer.get_ei(monitor_ws, 62.2)
        self.assertAlmostEqual(ei2b, 64.95, 2)

    def test_remove_empty_bg(self):
        # create test workspace
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

        # Prepare reducer
        tReducer = DirectEnergyConversion("MAR")
        tReducer.prop_man.sample_run = wksp
        tReducer.prop_man.empty_bg_run = "bg_ws"

        tReducer.remove_empty_background()

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))

        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

    def test_remove_empty_bg_with_normalisation(self):
        # create test workspace
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
        AddSampleLog(Workspace=wksp, LogName="gd_prtn_chrg", LogText="10.", LogType="Number")
        AddSampleLog(Workspace="bg_ws", LogName="gd_prtn_chrg", LogText="100.", LogType="Number")
        wksp = NormaliseByCurrent(wksp, OutputWorkspace="wksp", RecalculatePCharge=False)
        AddSampleLog(Workspace="wksp", LogName="DirectInelasticReductionNormalisedBy", LogText="current", LogType="String")

        # Prepare reducer
        tReducer = DirectEnergyConversion("MAR")
        tReducer.prop_man.sample_run = wksp
        tReducer.prop_man.empty_bg_run = "bg_ws"

        tReducer.remove_empty_background()

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))

        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

    def test_remove_empty_bg_normalised_both(self):
        # create test workspace
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
        AddSampleLog(Workspace=wksp, LogName="gd_prtn_chrg", LogText="10.", LogType="Number")
        AddSampleLog(Workspace="bg_ws", LogName="gd_prtn_chrg", LogText="100.", LogType="Number")
        wksp = NormaliseByCurrent(wksp, OutputWorkspace="wksp", RecalculatePCharge=False)
        AddSampleLog(Workspace="wksp", LogName="DirectInelasticReductionNormalisedBy", LogText="current", LogType="String")
        NormaliseByCurrent("bg_ws", OutputWorkspace="bg_ws", RecalculatePCharge=False)
        AddSampleLog(Workspace="bg_ws", LogName="DirectInelasticReductionNormalisedBy", LogText="current", LogType="String")

        # Prepare reducer
        tReducer = DirectEnergyConversion("MAR")
        tReducer.prop_man.sample_run = wksp
        tReducer.prop_man.empty_bg_run = "bg_ws"

        tReducer.remove_empty_background()

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))

        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

    def test_remove_empty_bg_with_monovan(self):
        # create test workspace
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
        CloneWorkspace(wksp, OutputWorkspace="bg_ws4wb")
        CloneWorkspace(wksp, OutputWorkspace="wb_ws")
        CloneWorkspace(wksp, OutputWorkspace="monovan_ws")
        AddSampleLog(Workspace=wksp, LogName="gd_prtn_chrg", LogText="10", LogType="Number")
        AddSampleLog(Workspace="bg_ws", LogName="gd_prtn_chrg", LogText="100", LogType="Number")
        AddSampleLog(Workspace="bg_ws4wb", LogName="gd_prtn_chrg", LogText="100", LogType="Number")
        AddSampleLog(Workspace="wb_ws", LogName="gd_prtn_chrg", LogText="10", LogType="Number")
        AddSampleLog(Workspace="monovan_ws", LogName="gd_prtn_chrg", LogText="10", LogType="Number")

        # Prepare reducer
        tReducer = DirectEnergyConversion("MAR")
        tReducer.prop_man.sample_run = wksp
        tReducer.prop_man.empty_bg_run = "bg_ws"
        tReducer.prop_man.wb_run = "wb_ws"
        tReducer.prop_man.empty_bg_run_for_wb = "bg_ws4wb"
        tReducer.prop_man.monovan_run = "monovan_ws"

        tReducer.remove_empty_background()

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))

        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.wb_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.monovan_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.wb_for_monovan_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

    def test_remove_empty_bg_all_different(self):
        # create test workspace
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

        CloneWorkspace(wksp, OutputWorkspace="wb_ws")
        CloneWorkspace(wksp, OutputWorkspace="monovan_ws")
        CloneWorkspace(wksp, OutputWorkspace="wb4monovan_ws")
        CloneWorkspace("bg_ws", OutputWorkspace="bg_ws4wb")
        CloneWorkspace("bg_ws", OutputWorkspace="bg_for_monovan")
        CloneWorkspace("bg_ws", OutputWorkspace="bg_for_wb4monovan_ws")

        # Prepare reducer
        tReducer = DirectEnergyConversion("MAR")
        tReducer.prop_man.sample_run = wksp
        tReducer.prop_man.empty_bg_run = "bg_ws"
        tReducer.prop_man.wb_run = "wb_ws"
        tReducer.prop_man.empty_bg_run_for_wb = "bg_ws4wb"
        tReducer.prop_man.monovan_run = "monovan_ws"
        tReducer.prop_man.empty_bg_run_for_monovan = "bg_for_monovan"
        tReducer.wb_for_monovan_run = "wb4monovan_ws"
        tReducer.empty_bg_run_for_monoWb = "bg_for_wb4monovan_ws"

        tReducer.remove_empty_background()

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))

        resWs = 0.9 * wksp
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.wb_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.monovan_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)

        ws = PropertyManager.wb_for_monovan_run.get_workspace()
        self.assertTrue(ws.run().hasProperty("empty_bg_removed"))
        difr = CompareWorkspaces(resWs, ws)
        self.assertTrue(difr.Result)


if __name__ == "__main__":
    # test = DirectEnergyConversionTest('test_remove_empty_bg_normalised_both')
    # test.run()
    unittest.main()
