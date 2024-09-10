# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from sys import platform
import numpy as np
from Direct.AbsorptionShapes import *
from Direct.PropertyManager import PropertyManager
from Direct.RunDescriptor import RunDescriptor


from mantid import api
from mantid.simpleapi import *


# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
class DirectPropertyManagerTest(unittest.TestCase):
    def __init__(self, methodName):
        self.prop_man = PropertyManager("MAR")
        return super(DirectPropertyManagerTest, self).__init__(methodName)

    def setUp(self):
        if self.prop_man is None or type(self.prop_man) is not type(PropertyManager):
            self.prop_man = PropertyManager("MAR")

    def tearDown(self):
        pass

    @staticmethod
    def getInstrument(InstrumentName="MAR"):
        """test method used to obtain default instrument for testing"""
        idf_file = api.ExperimentInfo.getInstrumentFilename(InstrumentName)
        tmp_ws_name = "__empty_" + InstrumentName
        if not mtd.doesExist(tmp_ws_name):
            LoadEmptyInstrument(Filename=idf_file, OutputWorkspace=tmp_ws_name)
        return mtd[tmp_ws_name].getInstrument()

    def test_init_reducer(self):
        propman = self.prop_man

        self.assertEqual(propman.deltaE_mode, "direct")

        self.assertRaises(KeyError, getattr, propman, "non_existing_property")

    def test_set_non_default_wrong_value(self):
        propman = self.prop_man
        # should do nothing as already initialized above

        # non-existing property can not be set!
        self.assertRaises(KeyError, setattr, propman, "non_existing_property", "Something_Meaningfull")
        # existing simple assignment works
        propman.load_monitors_with_workspace = False
        propman.load_monitors_with_workspace = True
        self.assertTrue(propman.load_monitors_with_workspace)

    def test_set_non_default_simple_value(self):
        propman = self.prop_man

        self.assertEqual(len(propman.getChangedProperties()), 0)
        propman.van_mass = 100

        self.assertAlmostEqual(propman.van_sig, 0.0, 7)

        propman.diag_van_sig = 1

        prop_changed = propman.getChangedProperties()

        self.assertEqual(len(prop_changed), 2)
        self.assertTrue("van_mass" in prop_changed)
        self.assertTrue("van_sig" in prop_changed)

    def test_overloaded_setters_getters(self):
        propman = self.prop_man

        changed_properties = propman.getChangedProperties()
        self.assertEqual(len(changed_properties), 0)

        self.assertAlmostEqual(propman.van_rmm, 50.9415, 9)
        self.assertRaises(AttributeError, setattr, propman, "van_rmm", 100)

        self.assertEqual(propman.det_cal_file, None)
        propman.det_cal_file = "a_data_file.dat"
        self.assertEqual(propman.det_cal_file, "a_data_file.dat")

        self.assertNotEqual(propman.map_file, None, "it is defined in IDF")
        propman.map_file = "a_map_file"
        self.assertEqual(propman.map_file, "a_map_file.map")

        self.assertFalse(propman.monovan_mapfile is None, " Monovan map file by default is defined")
        propman.monovan_mapfile = "a_monovan_map_file"
        self.assertEqual(propman.monovan_mapfile, "a_monovan_map_file.map")
        propman.monovan_mapfile = "the_monovan_map_file.rst"
        self.assertEqual(propman.monovan_mapfile, "the_monovan_map_file.rst")

        prop_changed = propman.getChangedProperties()
        self.assertEqual(len(prop_changed), 3)
        self.assertTrue("det_cal_file" in prop_changed)
        self.assertTrue("map_file" in prop_changed)
        self.assertTrue("monovan_mapfile" in prop_changed)

    def test_hartmask_plus_or_only(self):
        propman = self.prop_man

        self.assertEqual(propman.hard_mask_file, None)
        propman.hard_mask_file = "a_mask_file"
        self.assertEqual(propman.hard_mask_file, "a_mask_file.msk")

        prop_changed = propman.getChangedProperties()
        self.assertTrue("hard_mask_file" in prop_changed)

    def test_set_spectra_to_mon(self):
        propman = self.prop_man

        self.assertEqual(propman.spectra_to_monitors_list, None)

        propman.spectra_to_monitors_list = 35
        self.assertTrue(isinstance(propman.spectra_to_monitors_list, list))
        self.assertEqual(35, propman.spectra_to_monitors_list[0])

        propman.spectra_to_monitors_list = None
        self.assertEqual(propman.spectra_to_monitors_list, None)
        propman.spectra_to_monitors_list = "None"
        self.assertEqual(propman.spectra_to_monitors_list, None)
        propman.spectra_to_monitors_list = []
        self.assertEqual(propman.spectra_to_monitors_list, None)

        propman.spectra_to_monitors_list = "467"
        self.assertEqual(467, propman.spectra_to_monitors_list[0])

        propman.spectra_to_monitors_list = "467,444"
        self.assertEqual(467, propman.spectra_to_monitors_list[0])
        self.assertEqual(444, propman.spectra_to_monitors_list[1])

        propman.spectra_to_monitors_list = ["467", "444"]
        self.assertEqual(467, propman.spectra_to_monitors_list[0])
        self.assertEqual(444, propman.spectra_to_monitors_list[1])

        prop_changed = propman.getChangedProperties()
        self.assertEqual(len(prop_changed), 1)
        self.assertTrue("spectra_to_monitors_list" in prop_changed)

    def test_set_non_default_complex_value(self):
        propman = self.prop_man

        range = propman.norm_mon_integration_range
        self.assertAlmostEqual(
            range[0], 1000.0, 7, " Default integration min range on MARI should be as described in MARI_Parameters.xml " "file"
        )
        self.assertAlmostEqual(
            range[1], 2000.0, 7, " Default integration max range on MAPS should be as described in MARI_Parameters.xml " "file"
        )

        self.assertEqual(propman.ei_mon_spectra, (2, 3), " Default ei monitors on MARI should be as described in MARI_Parameters.xml file")

        propman.norm_mon_integration_range = [50, 1050]
        range = propman.norm_mon_integration_range
        self.assertAlmostEqual(range[0], 50.0, 7)
        self.assertAlmostEqual(range[1], 1050.0, 7)
        propman.ei_mon1_spec = 10
        mon_spectra = propman.ei_mon_spectra
        self.assertEqual(mon_spectra, (10, 3))
        self.assertEqual(propman.ei_mon1_spec, 10)

        prop_changed = propman.getChangedProperties()
        self.assertEqual(len(prop_changed), 2)
        self.assertTrue("norm_mon_integration_range" in prop_changed)

        self.assertTrue("norm_mon_integration_range" in prop_changed, "mon_norm_range should change")
        self.assertTrue("ei-mon1-spec" in prop_changed, "changing ei_mon_spectra should change ei-mon1-spec")

    def test_set_non_default_complex_value_synonims(self):
        propman = PropertyManager("MAP")
        propman.test_ei2_mon_spectra = 10000
        self.assertEqual(propman.ei_mon_spectra, (41475, 10000))

        prop_changed = propman.getChangedProperties()
        self.assertEqual(len(prop_changed), 1)

        self.assertTrue("ei-mon2-spec" in prop_changed, "changing test_ei2_mon_spectra should change ei-mon2-spectra")

        propman.test_mon_spectra_composite = [10000, 2000]
        self.assertEqual(propman.ei_mon_spectra, (10000, 2000))

        prop_changed = propman.getChangedProperties()
        self.assertEqual(len(prop_changed), 2)

        self.assertTrue("ei_mon_spectra" in prop_changed, "changing test_mon_spectra_composite should change ei_mon_spectra")

        ## weird way to prohibit this assignment
        # But it works and I was not able to find any other way!
        try:
            propman.ei_mon_spectra[1] = 100
            Success = True
        except TypeError:
            Success = False
        self.assertFalse(Success)
        self.assertEqual(10000, propman.ei_mon_spectra[0])
        self.assertEqual(2000, propman.ei_mon_spectra[1])

        # This works as should
        propman.ei_mon_spectra = (100, 200)
        self.assertEqual(100, propman.ei_mon_spectra[0])
        self.assertEqual(200, propman.ei_mon_spectra[1])

    def test_set_get_mono_range(self):
        # TODO : A lot of changes and tests here for mono_range
        propman = self.prop_man

        energy_incident = 100
        propman.incident_energy = energy_incident
        hi_frac = propman.monovan_hi_frac
        lo_frac = propman.monovan_lo_frac
        # propman.monovan_integr_range = None
        self.assertEqual(propman.monovan_integr_range, [lo_frac * energy_incident, hi_frac * energy_incident])

    def test_load_monitors_with_workspace(self):
        propman = self.prop_man

        self.assertTrue(propman.load_monitors_with_workspace, "MARI loads monitors with workspace by default")

        propman.load_monitors_with_workspace = True
        self.assertTrue(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace = 0
        self.assertFalse(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace = 10
        self.assertTrue(propman.load_monitors_with_workspace)

    def test_get_default_parameter_val(self):
        propman = self.prop_man
        param = propman.getDefaultParameterValue("map_file")
        self.assertTrue(isinstance(param, str))

        param = propman.getDefaultParameterValue("ei-mon1-spec")
        self.assertTrue(isinstance(param, int))

        param = propman.getDefaultParameterValue("check_background")
        self.assertTrue(isinstance(param, bool))

    def test_save_format(self):
        propman = self.prop_man

        formats = propman.save_format
        self.assertEqual(len(formats), 0)

        propman.save_format = "unknown"
        self.assertEqual(len(propman.save_format), 0)

        propman.save_format = ".spe"
        formats = propman.save_format
        self.assertTrue("spe" in formats)

        propman.save_format = "nxspe"
        formats = propman.save_format
        self.assertTrue("spe" in formats)
        self.assertTrue("nxspe" in formats)

        propman.save_format = ""
        self.assertEqual(len(propman.save_format), 0)

        propman.save_format = ["nxspe", ".nxs"]
        formats = propman.save_format
        self.assertTrue("nxs" in formats)
        self.assertTrue("nxspe" in formats)

        propman.save_format = None
        self.assertEqual(len(propman.save_format), 0)
        propman.save_format = "spe,.nxs"
        formats = propman.save_format
        self.assertEqual(len(propman.save_format), 2)
        self.assertTrue("nxs" in formats)
        self.assertTrue("spe" in formats)

        propman.save_format = "(spe,nxspe)"
        self.assertEqual(len(propman.save_format), 3)

        propman.save_format = "None"
        self.assertEqual(len(propman.save_format), 0)

        propman.save_format = ("spe", "nxspe")
        self.assertEqual(len(propman.save_format), 2)
        self.assertTrue("nxspe" in formats)
        self.assertTrue("spe" in formats)

    def test_allowed_values(self):
        propman = self.prop_man
        nm = propman.normalise_method
        self.assertEqual(nm, "monitor-1")
        propman.normalise_method = None
        self.assertEqual(propman.normalise_method, None)
        propman.normalise_method = "monitor-2"
        self.assertEqual(propman.normalise_method, "monitor-2")
        propman.normalise_method = "current"
        self.assertEqual(propman.normalise_method, "current")

        self.assertRaises(KeyError, setattr, propman, "normalise_method", "unsupported")

        # Only direct method is supported
        self.assertEqual(propman.deltaE_mode, "direct")
        self.assertRaises(KeyError, setattr, propman, "deltaE_mode", "unsupported")

    def test_ki_kf(self):
        propman = self.prop_man

        self.assertTrue(propman.apply_kikf_correction)

        propman.apply_kikf_correction = True
        self.assertTrue(propman.apply_kikf_correction)
        propman.apply_kikf_correction = False
        self.assertFalse(propman.apply_kikf_correction)

    def test_instr_name_and_psi(self):
        propman = self.prop_man

        psi = propman.psi
        self.assertTrue(np.isnan(psi))

        instr_name = propman.instr_name
        self.assertEqual(instr_name, "MARI")

        propman.psi = 10
        self.assertEqual(propman.psi, 10)

        logs = propman.motor_log_names
        self.assertTrue(isinstance(logs, list))
        self.assertEqual(len(logs), 2)
        self.assertEqual(logs[0], "wccr")
        self.assertEqual(logs[1], "Rot")

        self.assertEqual(propman.motor_offset, None)

        sample_ws = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=4, BankPixelWidth=1, NumEvents=10, XUnit="Energy", XMin=3, XMax=200, BinWidth=0.1
        )

        propman.motor_offset = 10
        psi = PropertyManager.psi.read_psi_from_workspace(sample_ws)
        self.assertTrue(np.isnan(psi))

        AddSampleLog(Workspace=sample_ws, LogName="Rot", LogText="20.", LogType="Number Series")
        propman.motor_offset = None
        psi = PropertyManager.psi.read_psi_from_workspace(sample_ws)
        self.assertTrue(np.isnan(psi))

        propman.psi = sample_ws
        self.assertTrue(np.isnan(propman.psi))

        propman.motor_offset = 10
        self.assertEqual(propman.motor_offset, 10)
        self.assertAlmostEqual(propman.psi, 30.0)
        psi = PropertyManager.psi.read_psi_from_workspace(sample_ws)
        self.assertAlmostEqual(psi, 30.0)

        AddSampleLog(Workspace=sample_ws, LogName="CCR_ROT", LogText="50.", LogType="Number Series")
        propman.motor_log_names = "Some_log"
        logs = propman.motor_log_names
        self.assertTrue(isinstance(logs, list))
        self.assertEqual(len(logs), 1)
        self.assertEqual(logs[0], "Some_log")

        self.assertTrue(np.isnan(propman.psi))
        propman.motor_log_names = "CCR_ROT"
        self.assertAlmostEqual(propman.psi, 60.0)

        psi = PropertyManager.psi.read_psi_from_workspace(sample_ws)
        self.assertAlmostEqual(psi, 60.0)

        api.AnalysisDataService.clear()

    def test_diag_spectra(self):
        propman = self.prop_man

        self.assertEqual(propman.diag_spectra, None)

        propman.diag_spectra = "(19,299);(399,500)"
        spectra = propman.diag_spectra
        self.assertEqual(spectra[0], (19, 299))
        self.assertEqual(spectra[1], (399, 500))

        propman = PropertyManager("MAP")
        spectra = propman.diag_spectra
        # (1,17280);(17281,18432);(18433,32256);(32257,41472)
        self.assertEqual(len(spectra), 4)
        self.assertEqual(spectra[0], (1, 17280))
        self.assertEqual(spectra[3], (32257, 41472))

    def test_get_diagnostics_parameters(self):
        propman = self.prop_man

        params = propman.get_diagnostics_parameters()
        self.assertEqual(len(params), 21)

        bkg_test_range0 = propman.background_test_range
        bkg_test_range = params["background_test_range"]
        bkg_range = propman.background_range
        self.assertEqual(bkg_range, bkg_test_range)
        self.assertEqual(bkg_range, bkg_test_range0)

        propman.background_test_range = [1000, 2000]
        bkg_test_range = propman.background_test_range
        self.assertEqual(bkg_test_range, (1000.0, 2000.0))

    def test_check_monovan_changed(self):
        propman = self.prop_man

        non_changed = propman._check_monovan_par_changed()
        # nothing have changed initially
        self.assertEqual(len(non_changed), 2)

        propman.monovan_run = 102
        propman.log_changed_values()

        propman.sample_mass = 1
        non_changed = propman._check_monovan_par_changed()
        self.assertEqual(len(non_changed), 1)
        propman.sample_rmm = 200
        non_changed = propman._check_monovan_par_changed()
        self.assertEqual(len(non_changed), 0)

        propman.log_changed_values()

    def test_set_defaults_from_instrument(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=100)

        SetInstrumentParameter(ws, ParameterName="TestParam1", Value="3.5", ParameterType="Number")
        SetInstrumentParameter(ws, ParameterName="TestParam2", Value="initial1", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="TestParam3", Value="initial2", ParameterType="String")

        instr = ws.getInstrument()
        propman = PropertyManager(instr)

        self.assertAlmostEqual(propman.TestParam1, 3.5)
        self.assertEqual(propman.TestParam2, "initial1")
        self.assertEqual(propman.TestParam3, "initial2")

        propman.TestParam2 = "gui_changed1"
        self.assertEqual(propman.TestParam2, "gui_changed1")

        SetInstrumentParameter(ws, ParameterName="TestParam2", Value="instr_changed1", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="TestParam3", Value="instr_changed2", ParameterType="String")

        self.assertAlmostEqual(propman.TestParam1, 3.5)
        self.assertEqual(propman.TestParam2, "gui_changed1")
        self.assertEqual(propman.TestParam3, "initial2")
        changes = propman.getChangedProperties()
        self.assertTrue("TestParam2" in changes)
        self.assertTrue("TestParam3" not in changes)

        changes = propman.update_defaults_from_instrument(ws.getInstrument())

        self.assertAlmostEqual(propman.TestParam1, 3.5)
        self.assertEqual(propman.TestParam2, "gui_changed1")
        self.assertEqual(propman.TestParam3, "instr_changed2")

        self.assertTrue("TestParam2" in changes)
        self.assertTrue("TestParam3" in changes)

    def test_set_complex_defaults_from_instrument(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)

        SetInstrumentParameter(ws, ParameterName="Param1", Value="BaseParam1:BaseParam2", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="BaseParam1", Value="Val1", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="BaseParam2", Value="Val2", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="synonims", Value="ParaPara=BaseParam2", ParameterType="String")

        instr = ws.getInstrument()
        propman = PropertyManager(instr)

        SampleResult = ["Val1", "Val2"]
        cVal = propman.Param1
        for test, sample in zip(cVal, SampleResult):
            self.assertEqual(test, sample)

        self.assertEqual(propman.ParaPara, "Val2")
        self.assertEqual(propman.BaseParam2, "Val2")

        propman.sample_run = ws
        # assume we get workspace with different instrument parameters
        SetInstrumentParameter(ws, ParameterName="Param1", Value="addParam1:addParam2", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="BaseParam1", Value="OtherVal1", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="BaseParam2", Value="OtherVal2", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="addParam1", Value="Ignore1", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="addParam2", Value="Ignore2", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="mask_run", Value="None", ParameterType="String")

        changed_prop = propman.update_defaults_from_instrument(ws.getInstrument())

        self.assertEqual(len(changed_prop), 4)
        # property have been changed from GUI and changes from instrument are
        # ignored
        SampleResult = ["OtherVal1", "OtherVal2"]
        cVal = propman.Param1
        for test, sample in zip(cVal, SampleResult):
            self.assertEqual(test, sample)

        self.assertEqual(propman.ParaPara, "OtherVal2")
        self.assertEqual(propman.BaseParam2, "OtherVal2")

        self.assertEqual(propman.BaseParam1, "OtherVal1")

    def test_set_all_defaults_from_instrument(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        # idf_dir = config.getString('instrumentDefinition.directory')
        idf_file = api.ExperimentInfo.getInstrumentFilename("LET", "2014-05-03 23:59:59")
        ws = LoadEmptyInstrument(Filename=idf_file, OutputWorkspace=ws)

        # Propman was defined for MARI but reduction parameters are all the
        # same, so testing on LET
        propman = self.prop_man
        self.assertEqual(propman.ei_mon1_spec, 2)

        ws = mtd["ws"]
        changed_prop = propman.update_defaults_from_instrument(ws.getInstrument(), False)
        self.assertTrue("ei-mon1-spec" in changed_prop)
        self.assertEqual(propman.ei_mon1_spec, 65542)

        self.assertFalse("ei_mon_spectra" in changed_prop)
        ei_spec = propman.ei_mon_spectra
        self.assertEqual(ei_spec[0], 65542)
        self.assertEqual(ei_spec[1], 5506)

    def test_set_energy_bins_and_ei(self):
        propman = self.prop_man

        propman.incident_energy = 20
        self.assertFalse(PropertyManager.incident_energy.multirep_mode())
        propman.energy_bins = "-30,3,10"

        bins = propman.energy_bins
        self.assertAlmostEqual(bins[0], -30)
        self.assertAlmostEqual(bins[1], 3)
        self.assertAlmostEqual(bins[2], 10)

        bins = PropertyManager.energy_bins.get_abs_range(propman)
        self.assertAlmostEqual(bins[0], -30)
        self.assertAlmostEqual(bins[1], 3)
        self.assertAlmostEqual(bins[2], 10)

        propman.incident_energy = 100.01
        propman.energy_bins = [-20, 4, 100]
        bins = propman.energy_bins
        self.assertAlmostEqual(bins[0], -20)
        self.assertAlmostEqual(bins[1], 4)
        self.assertAlmostEqual(bins[2], 100)
        bins = PropertyManager.energy_bins.get_abs_range(propman)
        self.assertAlmostEqual(bins[0], -20)
        self.assertAlmostEqual(bins[1], 4)
        self.assertAlmostEqual(bins[2], 100)

        propman.incident_energy = 10
        self.assertAlmostEqual(propman.incident_energy, 10)
        bins = PropertyManager.energy_bins.get_abs_range(propman)
        self.assertAlmostEqual(bins[0], -20 * 9.9999 / 100)
        self.assertAlmostEqual(bins[1], 4 * 9.9999 / 100)
        self.assertAlmostEqual(bins[2], 9.9999)

        ei = [20, 30]
        propman.incident_energy = ei
        got_ei = propman.incident_energy
        for ind, en in enumerate(got_ei):
            self.assertAlmostEqual(en, ei[ind])
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())
        bins = PropertyManager.energy_bins.get_abs_range(propman)

        self.assertAlmostEqual(bins[0], -20 * 20 * 0.99999 / 100)
        self.assertAlmostEqual(bins[1], 4 * 20 * 0.99999 / 100)
        self.assertAlmostEqual(bins[2], 20 * 0.99999)

        # check string work properly
        propman.incident_energy = "20"
        self.assertFalse(PropertyManager.incident_energy.multirep_mode())
        propman.energy_bins = [-2, 0.1, 0.8]

        bins = PropertyManager.energy_bins.get_abs_range(propman)

        self.assertAlmostEqual(bins[0], -2)
        self.assertAlmostEqual(bins[1], 0.1)
        self.assertAlmostEqual(bins[2], 0.8)

        propman.incident_energy = "[20]"
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())
        bins = PropertyManager.energy_bins.get_abs_range(propman)

        self.assertAlmostEqual(bins[0], -40)
        self.assertAlmostEqual(bins[1], 0.1 * 20)
        self.assertAlmostEqual(bins[2], 0.8 * 20)

        #
        propman.energy_bins = [-2, 0.1, 0.8]
        bins = propman.energy_bins
        self.assertAlmostEqual(bins[0], -2)
        self.assertAlmostEqual(bins[1], 0.1)
        self.assertAlmostEqual(bins[2], 0.8)

        bins = PropertyManager.energy_bins.get_abs_range(propman)
        self.assertAlmostEqual(bins[0], -20 * 2)
        self.assertAlmostEqual(bins[1], 20 * 0.1)
        self.assertAlmostEqual(bins[2], 20 * 0.8)

        propman.incident_energy = "20,30"
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())

        got_ei = propman.incident_energy
        for ind, en in enumerate(got_ei):
            self.assertAlmostEqual(en, ei[ind])

        propman.energy_bins = None
        self.assertFalse(propman.energy_bins)

    def test_multirep_ei_iterate_over(self):
        propman = self.prop_man
        propman.incident_energy = 20
        propman.energy_bins = [-2, 0.1, 0.8]
        self.assertFalse(PropertyManager.incident_energy.multirep_mode())

        AllEn = PropertyManager.incident_energy.getAllEiList()
        for ind, en in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ind)

            cen = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en, cen)

            self.assertAlmostEqual(en, 20)

            bins = propman.energy_bins
            self.assertAlmostEqual(bins[0], -2)
            self.assertAlmostEqual(bins[1], 0.1)
            self.assertAlmostEqual(bins[2], 0.8)

            bins = PropertyManager.energy_bins.get_abs_range(propman)
            self.assertAlmostEqual(bins[0], -2)
            self.assertAlmostEqual(bins[1], 0.1)
            self.assertAlmostEqual(bins[2], 0.8)

        propman.incident_energy = [20]
        propman.energy_bins = [-2, 0.1, 0.8]
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())

        AllEn = PropertyManager.incident_energy.getAllEiList()
        for ind, en in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ind)

            cen = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en, cen)

            self.assertAlmostEqual(en, 20)

            bins = propman.energy_bins
            self.assertAlmostEqual(bins[0], -2)
            self.assertAlmostEqual(bins[1], 0.1)
            self.assertAlmostEqual(bins[2], 0.8)

            bins = PropertyManager.energy_bins.get_abs_range(propman)

            self.assertAlmostEqual(bins[0], -2 * 20)
            self.assertAlmostEqual(bins[1], 0.1 * 20)
            self.assertAlmostEqual(bins[2], 0.8 * 20)

        eng = [20, 40, 60]
        propman.incident_energy = eng
        propman.energy_bins = [-2, 0.1, 0.8]

        self.assertTrue(PropertyManager.incident_energy.multirep_mode())

        AllEn = PropertyManager.incident_energy.getAllEiList()
        for ic, en in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ic)

            self.assertAlmostEqual(en, eng[ic])
            cen = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en, cen)

            bins = PropertyManager.energy_bins.get_abs_range(propman)
            self.assertAlmostEqual(bins[0], -2 * en)
            self.assertAlmostEqual(bins[1], 0.1 * en)
            self.assertAlmostEqual(bins[2], 0.8 * en)
        #
        AllEn = PropertyManager.incident_energy.getAllEiList()
        for ic, en in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ic)

            self.assertAlmostEqual(en, eng[ic])
            ei_stored = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en, ei_stored)

            cen = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en, cen)

            bins = PropertyManager.energy_bins.get_abs_range(propman)
            self.assertAlmostEqual(bins[0], -2 * eng[ic])
            self.assertAlmostEqual(bins[1], 0.1 * eng[ic])
            self.assertAlmostEqual(bins[2], 0.8 * eng[ic])

    def test_incident_energy_custom_enum(self):
        ##### Custom enum works in a peculiar way
        propman = self.prop_man
        en_source = [20, 40, 80]
        propman.incident_energy = en_source
        propman.energy_bins = [-2, 0.1, 0.8]
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())

        AllEn = PropertyManager.incident_energy.getAllEiList()
        for ic, en in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ic)

            # propagate current energy value to incident energy class
            self.assertAlmostEqual(en, en_source[ic])
            en_internal = PropertyManager.incident_energy.get_current()
            self.assertAlmostEqual(en_internal, en_source[ic])

    def test_auto_ei(self):
        propman = self.prop_man
        propman.incident_energy = "Auto"
        propman.energy_bins = [-2, 0.1, 0.8]
        self.assertTrue(PropertyManager.incident_energy.multirep_mode())

        # create test workspace
        wsEn = CreateSampleWorkspace(
            Function="Multiple Peaks", NumBanks=1, BankPixelWidth=2, NumEvents=10000, XUnit="Energy", XMin=10, XMax=200, BinWidth=0.1
        )
        # convert units to TOF to simulate real workspace obtained from experiment
        mon_ws = ConvertUnits(InputWorkspace=wsEn, Target="TOF")
        # find chopper log values would be present in real workspace
        l_chop = 7.5  # chopper position build into test workspace
        l_mon1 = 15.0  # monitor 1 position (detector 1), build into test workspace
        t_mon1 = 3100.0  # the time of flight defined by incident energy of the peak generated by
        # CreateSampelpWorkspace algorithm.
        t_chop = (l_chop / l_mon1) * t_mon1
        # Add these log values to simulated workspace to represent real sample logs
        AddTimeSeriesLog(mon_ws, Name="fermi_delay", Time="2010-01-01T00:00:00", Value=t_chop, DeleteExisting=True)
        AddTimeSeriesLog(mon_ws, Name="fermi_delay", Time="2010-01-01T00:30:00", Value=t_chop)
        AddTimeSeriesLog(mon_ws, Name="fermi_speed", Time="2010-01-01T00:00:00", Value=900, DeleteExisting=True)
        AddTimeSeriesLog(mon_ws, Name="fermi_speed", Time="2010-01-01T00:30:00", Value=900)
        AddTimeSeriesLog(mon_ws, Name="is_running", Time="2010-01-01T00:00:00", Value=1, DeleteExisting=True)
        AddTimeSeriesLog(mon_ws, Name="is_running", Time="2010-01-01T00:30:00", Value=1)

        valid = PropertyManager.incident_energy.validate(propman, PropertyManager)
        self.assertTrue(valid[0])
        valid = PropertyManager.energy_bins.validate(propman, PropertyManager)
        self.assertTrue(valid[0])

        # define monitors, used to calculate ei
        propman.ei_mon_spectra = (1, 2)
        PropertyManager.incident_energy.set_auto_Ei(mon_ws, propman)

        allEi = PropertyManager.incident_energy.getAllEiList()

        self.assertAlmostEqual(allEi[0], 8.8, 1)
        self.assertAlmostEqual(allEi[1], 15.8, 1)

    def test_ignore_complex_defailts_changes_fom_instrument(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)

        SetInstrumentParameter(ws, ParameterName="bkgd_range", Value="bkgd-range-min:bkgd-range-max", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="bkgd-range-min", Value="100.", ParameterType="Number")
        SetInstrumentParameter(ws, ParameterName="bkgd-range-max", Value="200.", ParameterType="Number")

        propman = self.prop_man

        propman.background_range = [20, 40]
        bkgd_range = propman.bkgd_range
        self.assertAlmostEqual(bkgd_range[0], 20)
        self.assertAlmostEqual(bkgd_range[1], 40)

        changed_prop = propman.update_defaults_from_instrument(ws.getInstrument())

        self.assertEqual(len(changed_prop), 3)
        bkgd_range = propman.bkgd_range
        self.assertAlmostEqual(bkgd_range[0], 20)
        self.assertAlmostEqual(bkgd_range[1], 40)

    def test_ignore_complex_defailts_single_fom_instrument(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)

        SetInstrumentParameter(ws, ParameterName="bkgd_range", Value="bkgd-range-min:bkgd-range-max", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="bkgd-range-min", Value="100.", ParameterType="Number")
        SetInstrumentParameter(ws, ParameterName="bkgd-range-max", Value="200.", ParameterType="Number")

        propman = self.prop_man
        mari_bkgd_range = propman.bkgd_range

        setattr(propman, "bkgd-range-max", 40)
        bkgd_range = propman.bkgd_range
        self.assertAlmostEqual(bkgd_range[0], mari_bkgd_range[0])
        self.assertAlmostEqual(bkgd_range[1], 40)

        changed_prop = propman.update_defaults_from_instrument(ws.getInstrument())

        self.assertEqual(len(changed_prop), 2)
        bkgd_range = propman.bkgd_range
        self.assertAlmostEqual(bkgd_range[0], 100)
        self.assertAlmostEqual(bkgd_range[1], 40)

    def test_monovan_integration_range(self):
        propman = self.prop_man

        propman.incident_energy = 10
        propman.monovan_lo_frac = -0.6
        propman.monovan_hi_frac = 0.7

        range = propman.abs_units_van_range
        self.assertAlmostEqual(range[0], -6.0)
        self.assertAlmostEqual(range[1], 7.0)

        range = propman.monovan_integr_range
        self.assertAlmostEqual(range[0], -6.0)
        self.assertAlmostEqual(range[1], 7.0)

        propman.monovan_lo_value = -10
        propman.monovan_hi_value = 10

        range = propman.abs_units_van_range
        self.assertAlmostEqual(range[0], -6.0)
        self.assertAlmostEqual(range[1], 7.0)

        propman.abs_units_van_range = [-40, 40]
        self.assertAlmostEqual(propman.monovan_lo_value, -40)
        self.assertAlmostEqual(propman.monovan_hi_value, 40)

        range = propman.monovan_integr_range
        self.assertAlmostEqual(range[0], -40)
        self.assertAlmostEqual(range[1], 40)

        propman.abs_units_van_range = None

        range = propman.monovan_integr_range
        self.assertAlmostEqual(range[0], -6.0)
        self.assertAlmostEqual(range[1], 7.0)
        #
        propman.monovan_lo_frac = -0.7
        range = propman.monovan_integr_range
        self.assertAlmostEqual(range[0], -7.0)

    def test_save_filename(self):
        propman = self.prop_man

        propman.incident_energy = 10
        propman.sample_run = 0
        propman.monovan_run = None

        name = propman.save_file_name
        self.assertEqual(name, "MAR00000Ei10d00meV")

    def test_log_to_Mantid(self):
        propman = self.prop_man
        self.assertFalse(propman.log_to_mantid)

        propman.log_to_mantid = True
        self.assertTrue(propman.log_to_mantid)

        propman.log_to_mantid = 0
        self.assertFalse(propman.log_to_mantid)

    def test_hadmask_options(self):
        propman = self.prop_man
        propman.hard_mask_file = "some_hard_mask_file"
        self.assertEqual(propman.hard_mask_file, "some_hard_mask_file.msk")

        propman.use_hard_mask_only = False
        self.assertFalse(propman.use_hard_mask_only)
        propman.use_hard_mask_only = True
        self.assertTrue(propman.use_hard_mask_only)

        propman.hardmaskPlus = "other_hard_mask_file.msk"

        self.assertFalse(propman.use_hard_mask_only)
        self.assertEqual(propman.hard_mask_file, "other_hard_mask_file.msk")
        self.assertTrue(propman.run_diagnostics)

        propman.hardmaskOnly = "more_hard_mask_file"
        self.assertTrue(propman.use_hard_mask_only)
        self.assertEqual(propman.hard_mask_file, "more_hard_mask_file.msk")
        self.assertTrue(propman.run_diagnostics)

        propman.hardmaskOnly = "None"
        self.assertFalse(propman.use_hard_mask_only)
        self.assertTrue(propman.run_diagnostics)
        self.assertEqual(propman.hard_mask_file, None)

    def test_hadmask_options_locked(self):
        #
        propman1 = self.prop_man
        propman1.setChangedProperties()
        propman1.hardmaskPlus = "a_hard_mask_file"
        self.assertFalse(propman1.use_hard_mask_only)
        self.assertEqual(propman1.hard_mask_file, "a_hard_mask_file.msk")
        self.assertTrue(propman1.run_diagnostics)
        changed_prop = propman1.getChangedProperties()
        self.assertEqual(len(changed_prop), 2)

        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)

        SetInstrumentParameter(ws, ParameterName="hard_mask_file", Value="different", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="run_diagnostics", Value="False", ParameterType="String")
        SetInstrumentParameter(ws, ParameterName="use_hard_mask_only", Value="True", ParameterType="String")

        # verify if changed properties list does not change anything
        changed_prop = propman1.update_defaults_from_instrument(ws.getInstrument())
        self.assertEqual(len(changed_prop), 2)
        self.assertFalse(propman1.use_hard_mask_only)
        self.assertEqual(propman1.hard_mask_file, "a_hard_mask_file.msk")
        self.assertTrue(propman1.run_diagnostics)

        propman1.setChangedProperties(set())
        propman1.hardmaskOnly = "more_hard_mask_file"

        # verify if changed properties list does not change anything
        changed_prop = propman1.update_defaults_from_instrument(ws.getInstrument())
        self.assertTrue(propman1.use_hard_mask_only)
        self.assertEqual(propman1.hard_mask_file, "more_hard_mask_file.msk")
        self.assertTrue(propman1.run_diagnostics)

    def test_sum_runs(self):
        propman = self.prop_man
        propman.sum_runs = True
        self.assertTrue(propman.sum_runs)
        propman.sum_runs = False
        self.assertFalse(propman.sum_runs)

        propman.sum_runs = 10  # TODO should we define number of runs to sum?
        self.assertTrue(propman.sum_runs)
        propman.sum_runs = 0
        self.assertFalse(propman.sum_runs)

    # def test_do_white(self) :
    #    tReducer = self.reducer
    #    monovan = 1000
    #    data = None
    #    name = tReducer.make_ckpt_name('do_white',monovan,data,'t1')
    #    self.assertEqual('do_white1000t1',name)

    def test_monitors_list(self):
        propman = self.prop_man
        mons = propman.get_used_monitors_list()
        self.assertEqual(len(mons), 3)

        propman.normalise_method = None
        mons = propman.get_used_monitors_list()
        self.assertEqual(len(mons), 2)

        propman.normalise_method = "monitor-2"
        mons = propman.get_used_monitors_list()
        self.assertEqual(len(mons), 2)

    def test_mon2_integration_range(self):
        propman = self.prop_man
        propman.incident_energy = 10
        range = propman.mon2_norm_energy_range

        # check defaults
        self.assertAlmostEqual(range[0], 8.0)
        self.assertAlmostEqual(range[1], 12.0)

        propman.mon2_norm_energy_range = [0.7, 1.3]
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 7.0)
        self.assertAlmostEqual(range[1], 13.0)

        propman.mon2_norm_energy_range = "[0.5,1.5]"
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 5.0)
        self.assertAlmostEqual(range[1], 15.0)

        propman.mon2_norm_energy_range = "0.6,1.4"
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 6.0)
        self.assertAlmostEqual(range[1], 14.0)

        self.assertRaises(KeyError, setattr, propman, "mon2_norm_energy_range", 10)
        self.assertRaises(KeyError, setattr, propman, "mon2_norm_energy_range", "[0.95,1.05,4]")
        self.assertRaises(KeyError, setattr, propman, "mon2_norm_energy_range", "[0.05,0.9]")

        propman.mon2_norm_energy_range = "0.95,1.05"
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 9.5)
        self.assertAlmostEqual(range[1], 10.5)

        # Test multirep mode
        propman.incident_energy = [10, 20, 30]
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 9.5)
        self.assertAlmostEqual(range[1], 10.5)

        PropertyManager.incident_energy.next()
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 2 * 9.5)
        self.assertAlmostEqual(range[1], 2 * 10.5)

        PropertyManager.incident_energy.next()
        range = propman.mon2_norm_energy_range
        self.assertAlmostEqual(range[0], 3 * 9.5)
        self.assertAlmostEqual(range[1], 3 * 10.5)

    def test_multirep_tof_specta_list(self):
        propman = self.prop_man
        sp = propman.multirep_tof_specta_list
        self.assertEqual(len(sp), 2)
        self.assertEqual(sp[0], 5)

        propman.multirep_tof_specta_list = "10"
        sp = propman.multirep_tof_specta_list
        self.assertEqual(len(sp), 1)
        self.assertEqual(sp[0], 10)

        propman.multirep_tof_specta_list = "10,11,13,15"
        sp = propman.multirep_tof_specta_list
        self.assertEqual(len(sp), 4)
        self.assertEqual(sp[3], 15)

    def test_mono_correction_factor(self):
        propman = self.prop_man
        propman.incident_energy = [10, 20]
        # propman.m

        PropertyManager.mono_correction_factor.set_cash_mono_run_number(11015)

        self.assertEqual(propman.mono_correction_factor, None)
        propman.mono_correction_factor = 66.0
        self.assertAlmostEqual(propman.mono_correction_factor, 66)

        self.assertEqual(PropertyManager.mono_correction_factor.get_val_from_cash(propman), None)
        PropertyManager.mono_correction_factor.set_val_to_cash(propman, 100)
        self.assertAlmostEqual(PropertyManager.mono_correction_factor.get_val_from_cash(propman), 100)

        PropertyManager.incident_energy.next()
        self.assertEqual(PropertyManager.mono_correction_factor.get_val_from_cash(propman), None)
        PropertyManager.mono_correction_factor.set_val_to_cash(propman, 50)
        self.assertAlmostEqual(PropertyManager.mono_correction_factor.get_val_from_cash(propman), 50)

        PropertyManager.mono_correction_factor.set_cash_mono_run_number(11060)
        self.assertEqual(PropertyManager.mono_correction_factor.get_val_from_cash(propman), None)

    def test_mono_file_properties(self):
        propman = self.prop_man
        propman.wb_run = 11001
        sw = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        propman.monovan_run = sw
        propman.mask_run = CloneWorkspace(sw, OutputWorkspace="mask_clone")
        propman.map_file = None
        propman.hard_mask_file = "testmasking.xml"
        propman.det_cal_file = 11001
        propman.monovan_mapfile = None

        file_prop = propman._get_properties_with_files()

        self.assertEqual(len(file_prop), 3)
        self.assertTrue("wb_run" in file_prop)
        self.assertFalse("monovan_run" in file_prop)
        self.assertFalse("mask_run" in file_prop)
        self.assertFalse("wb_for_monovan_run" in file_prop)

        self.assertTrue("hard_mask_file" in file_prop)
        self.assertTrue("det_cal_file" in file_prop)

        ok, fail_list = propman._check_file_properties()
        self.assertTrue(ok)
        if not ok:
            print("fail prop list: ", fail_list)

        api.AnalysisDataService.clear()

        propman.monovan_run = 11002
        propman.mask_run = None
        propman.wb_for_monovan_run = 11001
        propman.map_file = "some_missing_map"

        ok, fail_list = propman._check_file_properties()
        self.assertFalse(ok)
        self.assertEqual(len(fail_list), 2)
        self.assertTrue("monovan_run" in fail_list)
        self.assertTrue("map_file" in fail_list)

    def test_find_files2sum(self):
        propman = self.prop_man
        propman.sample_run = [11001, 11111]

        propman.sum_runs = False
        ok, not_found, found = propman.find_files_to_sum()
        self.assertTrue(ok)
        self.assertEqual(len(not_found), 0)
        self.assertEqual(len(found), 0)

        propman.sum_runs = True
        ok, not_found, found = propman.find_files_to_sum()
        self.assertFalse(ok)
        self.assertEqual(len(not_found), 1)
        self.assertEqual(len(found), 1)
        self.assertEqual(not_found[0], 11111)
        self.assertEqual(found[0], 11001)

        ok1, not_found1, found1 = propman.find_files_to_sum()
        self.assertEqual(len(not_found1), 1)
        self.assertEqual(len(found1), 1)

        ok, err_list = propman._check_file_properties()
        self.assertFalse(ok)
        self.assertEqual(len(err_list), 2)
        self.assertTrue("missing_runs_toSum" in err_list)
        self.assertEqual(err_list["missing_runs_toSum"], "[11111]")

    def test_custom_print(self):
        propman = self.prop_man
        propman.sample_run = 1000
        propman.incident_energy = 20.0

        def custom_print(propman):
            ei = propman.incident_energy
            run_n = PropertyManager.sample_run.run_number()
            name = "RUN{0}atEi{1:<4.1f}meV_One2One".format(run_n, ei)
            return name

        PropertyManager.save_file_name.set_custom_print(lambda: custom_print(self.prop_man))

        self.assertEqual(propman.save_file_name, "RUN1000atEi20.0meV_One2One")

        propman.sample_run = 2000
        self.assertEqual(propman.save_file_name, "RUN2000atEi20.0meV_One2One")
        # clean up
        PropertyManager.save_file_name.set_custom_print(None)

    def test_ei_mon_spectra(self):
        propman = self.prop_man
        # test default property assignment
        setattr(propman, "ei-mon1-spec", 101)
        setattr(propman, "ei-mon2-spec", 2020)
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], 101)
        self.assertEqual(spectra[1], 2020)

        propman.ei_mon_spectra = "ei-mon1-spec:ei-mon2-spec"

        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], 101)
        self.assertEqual(spectra[1], 2020)

        setattr(propman, "ei-mon1-spec", [101, 102, 103])
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], [101, 102, 103])
        self.assertEqual(spectra[1], 2020)
        self.assertTrue(PropertyManager.ei_mon_spectra.need_to_sum_monitors(propman))

        propman.ei_mon_spectra = (1, [3, 4, 5])
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], 1)
        self.assertEqual(spectra[1], [3, 4, 5])
        self.assertTrue(PropertyManager.ei_mon_spectra.need_to_sum_monitors(propman))

        propman.ei_mon_spectra = "3,4,5:1,2"
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], [3, 4, 5])
        self.assertEqual(spectra[1], [1, 2])
        self.assertTrue(PropertyManager.ei_mon_spectra.need_to_sum_monitors(propman))

        propman.ei_mon_spectra = "3:10"
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], 3)
        self.assertEqual(spectra[1], 10)
        self.assertFalse(PropertyManager.ei_mon_spectra.need_to_sum_monitors(propman))

        propman.ei_mon_spectra = "[4:11]"
        spectra = propman.ei_mon_spectra
        self.assertEqual(spectra[0], 4)
        self.assertEqual(spectra[1], 11)
        self.assertFalse(PropertyManager.ei_mon_spectra.need_to_sum_monitors(propman))

    def test_average_accuracy(self):
        #
        val = [0.0452, 0.0455, -0.045, -0.236, 1, 0.98, 1.02, 2.333, 2.356, 21.225, 21.5, 301.99, 305]
        exp_rez = [0.045, 0.046, -0.045, -0.24, 1, 0.98, 1.0, 2.3, 2.4, 21.0, 22.0, 302.0, 305]
        rez = PropertyManager.auto_accuracy.roundoff(val)

        for valExp, valReal in zip(exp_rez, rez):
            self.assertAlmostEqual(valExp, valReal)

    def test_abs_corr_info(self):
        propman = self.prop_man

        defaults = propman.abs_corr_info
        self.assertTrue(defaults["is_mc"])
        # the algorithm sets up the properties but does not verifis if the prperties
        # are acceptable by the corrections algorithm
        propman.abs_corr_info = "{is_mc: True, NumberOfWavelengthPoints: 200; MaxScatterPtAttempts=20, " "SparseInstrument=True}"

        propss = propman.abs_corr_info
        self.assertTrue(propss["is_mc"])
        self.assertEqual(propss["NumberOfWavelengthPoints"], 200)
        self.assertEqual(propss["MaxScatterPtAttempts"], 20)
        self.assertEqual(propss["SparseInstrument"], True)

        # Properties acceptable by MoneCarloAbsorption algorithm
        propman.abs_corr_info = {
            "is_mc": True,
            "NumberOfWavelengthPoints": 20,
            "EventsPerPoint": "200",
            "SeedValue": 31090,
            "Interpolation": "CSpline",
            "SparseInstrument": "True",
            "NumberOfDetectorRows": 20,
            "NumberOfDetectorColumns": "10",
            "MaxScatterPtAttempts": 200,
        }

        propss = propman.abs_corr_info
        self.assertTrue(propss["is_mc"])
        self.assertTrue(propss["SparseInstrument"])
        self.assertEqual(propss["Interpolation"], "CSpline")

        str_val = str(propman.abs_corr_info)

        propman.abs_corr_info = str_val

        rec_prop = propman.abs_corr_info

        self.assertDictEqual(propss, rec_prop)
        # Properties acceptable by AbsorptionCorrection algorithm
        ac_properties = {
            "ScatterFrom": "Sample",
            "NumberOfWavelengthPoints": 10,
            "ExpMethod": "FastApprox",
            "EMode": "Direct",
            "EFixed": 10,
            "ElementSize": 2,
        }
        ac_properties["is_fast"] = True
        propman.abs_corr_info = str(ac_properties)

        rec_prop = propman.abs_corr_info
        self.assertDictEqual(ac_properties, rec_prop)

        propman.abs_corr_info = None
        rec_prop = propman.abs_corr_info
        self.assertDictEqual(rec_prop, {"is_mc": True})

        self.assertRaises(KeyError, PropertyManager.abs_corr_info.__set__, propman.abs_corr_info, [1, 2, 3])
        self.assertRaises(KeyError, PropertyManager.abs_corr_info.__set__, propman.abs_corr_info, {"TheKeyNotRecognizedByAlgorithm": 10})

    #
    def test_abs_shapes_container(self):
        propman = self.prop_man

        cyl = Cylinder(["Al", 0.1], [10, 1])
        propman.correct_absorption_on = cyl

        got = propman.correct_absorption_on
        self.assertEqual(got, cyl)

        str_rep_cyl = str(propman.correct_absorption_on)
        is_in = "Shape" in str_rep_cyl
        self.assertTrue(is_in)
        is_in = "Cylinder" in str_rep_cyl
        self.assertTrue(is_in)

        propman.correct_absorption_on = str_rep_cyl

        got = propman.correct_absorption_on

        self.assertTrue(isinstance(got, Cylinder))
        self.assertEqual(got.material, cyl.material)
        self.assertEqual(got.shape, cyl.shape)

    #
    def test_lastrun_log_default(self):
        #
        if platform.startswith("linux"):
            self.assertEqual(PropertyManager.archive_upload_log_template, "/archive/NDX{0}/Instrument/logs/lastrun.txt")
            log_dir = "/archive/NDXMARI/Instrument/logs/"
        elif platform == "darwin":
            self.assertEqual(PropertyManager.archive_upload_log_template, "")
            log_dir = ""
        elif platform == "win32":
            self.assertEqual(PropertyManager.archive_upload_log_template, r"\\isis\inst$\NDX{0}\Instrument\logs\lastrun.txt")
            log_dir = r"\\isis\inst$\NDXMARI\Instrument\logs\\"

        propman = self.prop_man

        if os.path.isdir(log_dir):
            # in case test server or test machine is connected to the archive
            self.assertEqual(propman.archive_upload_log_file, os.path.normpath(log_dir + "lastrun.txt"))
        else:
            self.assertEqual(propman.archive_upload_log_file, "")

        test_dir = config.getString("defaultsave.directory")
        test_file = os.path.normpath(test_dir + "lastrun.txt")
        f = open(test_file, "w")
        f.write("aaaaaa")
        f.close()

        propman.archive_upload_log_file = test_file
        self.assertEqual(propman.archive_upload_log_file, test_file)

        os.remove(test_file)

    #
    def test_get_property_class(self):
        propman = self.prop_man

        p1 = propman.get_prop_class("empty_bg_run")

        self.assertTrue(isinstance(p1, RunDescriptor))
        self.assertEqual(p1._prop_name, "EBG_")
        self.assertTrue(p1.run_number() is None)

        propman.empty_bg_run = 1024
        self.assertEqual(p1.run_number(), 1024)

    #
    def test_set_psi_directly_works(self):
        propman = self.prop_man

        propman.psi = 10
        self.assertEqual(propman.psi, 10)

    def test_default_psi_is_nan(self):
        propman = self.prop_man

        self.assertTrue(np.isnan(propman.psi))

    def test_psi_set_to0_works(self):
        propman = self.prop_man

        propman.psi = 0
        self.assertEqual(propman.psi, 0)


if __name__ == "__main__":
    # tester = DirectPropertyManagerTest('test_get_property')
    # tester.run()
    unittest.main()
