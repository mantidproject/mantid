from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import numpy as np
import os
import sys
from DirectPropertyManager import DirectPropertyManager


#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class DirectPropertyManagerTest(unittest.TestCase):

    def __init__(self, methodName):
        self.prop_man = DirectPropertyManager("MAR");
        return super(DirectPropertyManagerTest, self).__init__(methodName)

    def setUp(self):
        if self.prop_man == None or type(self.prop_man) != type(DirectPropertyManager):
            self.prop_man  = DirectPropertyManager("MAR");
    def tearDown(self):
        pass

    @staticmethod
    def getInstrument(InstrumentName='MAR'):
        """ test method used to obtain default instrument for testing """
        idf_dir = config.getString('instrumentDefinition.directory')
        idf_file=api.ExperimentInfo.getInstrumentFilename(InstrumentName)
        tmp_ws_name = '__empty_' + InstrumentName
        if not mtd.doesExist(tmp_ws_name):
               LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
        return mtd[tmp_ws_name].getInstrument();

 
    def test_init_reducer(self):
        
        propman=self.prop_man;

        self.assertEqual(propman.deltaE_mode,'direct')

        self.assertRaises(KeyError,getattr,propman,'non_existing_property')


    def test_set_non_default_wrong_value(self):
        propman = self.prop_man
        # should do nothing as already initialized above

        # non-existing property can not be set!
        self.assertRaises(KeyError,setattr,propman,'non_existing_property',"Something_Meaningfull")
        # existing simple assignment works 
        propman.load_monitors_with_workspace = False;
        propman.load_monitors_with_workspace = True;
        self.assertTrue(propman.load_monitors_with_workspace);

    def test_set_non_default_simple_value(self):
        propman = self.prop_man

        self.assertEqual(len(propman.getChangedProperties()),0);
        propman.van_mass = 100;

        self.assertAlmostEqual(propman.van_sig,0.,7)

        propman.diag_van_sig = 1;

        prop_changed = propman.getChangedProperties();

        self.assertEqual(len(prop_changed),2)
        self.assertTrue('van_mass' in prop_changed)
        self.assertTrue('van_sig'  in prop_changed)

    def test_overloaded_setters_getters(self):
        propman = self.prop_man

        changed_properties = propman.getChangedProperties()
        self.assertEqual(len(changed_properties),0);

        self.assertAlmostEqual(propman.van_rmm,50.9415,9);
        self.assertRaises(AttributeError,setattr,propman,'van_rmm',100);

        self.assertTrue(propman.det_cal_file is None);
        propman.det_cal_file = 'a_data_file.dat'
        self.assertEqual(propman.det_cal_file,'a_data_file.dat');

        self.assertTrue(propman.map_file is not None, 'it is defined in IDF');
        propman.map_file = 'a_map_file'
        self.assertEqual(propman.map_file,'a_map_file.map');

        self.assertTrue(propman.hard_mask_file is None);
        propman.hard_mask_file = 'a_mask_file'
        self.assertEqual(propman.hard_mask_file,'a_mask_file.msk');

        self.assertFalse(propman.monovan_mapfile is None," Monovan map file by default is defined");
        propman.monovan_mapfile = 'a_monovan_map_file'
        self.assertEqual(propman.monovan_mapfile,'a_monovan_map_file.map');
        propman.monovan_mapfile = 'the_monovan_map_file.rst'
        self.assertEqual(propman.monovan_mapfile,'the_monovan_map_file.rst');


        prop_changed =propman.getChangedProperties()
        self.assertEqual(len(prop_changed),4)
        self.assertTrue('det_cal_file' in prop_changed)
        self.assertTrue('hard_mask_file' in prop_changed)
        self.assertTrue('map_file' in prop_changed)
        self.assertTrue('monovan_mapfile' in prop_changed)


    def test_set_spectra_to_mon(self):
        propman = self.prop_man

        self.assertTrue(propman.spectra_to_monitors_list is None);

        propman.spectra_to_monitors_list = 35;
        self.assertTrue(isinstance(propman.spectra_to_monitors_list,list));
        self.assertEquals(35,propman.spectra_to_monitors_list[0]);

        propman.spectra_to_monitors_list = None;
        self.assertTrue(propman.spectra_to_monitors_list is None);
        propman.spectra_to_monitors_list = 'None';
        self.assertTrue(propman.spectra_to_monitors_list is None);
        propman.spectra_to_monitors_list = [];
        self.assertTrue(propman.spectra_to_monitors_list is None);

        propman.spectra_to_monitors_list = '467';
        self.assertEquals(467,propman.spectra_to_monitors_list[0]);

        propman.spectra_to_monitors_list = '467,444';
        self.assertEquals(467,propman.spectra_to_monitors_list[0]);
        self.assertEquals(444,propman.spectra_to_monitors_list[1]);

        propman.spectra_to_monitors_list = ['467','444'];
        self.assertEquals(467,propman.spectra_to_monitors_list[0]);
        self.assertEquals(444,propman.spectra_to_monitors_list[1]);

        prop_changed = propman.getChangedProperties();
        self.assertEqual(len(prop_changed),1)
        self.assertTrue('spectra_to_monitors_list' in prop_changed)




    def test_set_non_default_complex_value(self):
        propman = self.prop_man


        range = propman.norm_mon_integration_range
        self.assertAlmostEqual(range[0],1000.,7," Default integration min range on MARI should be as described in MARI_Parameters.xml file")
        self.assertAlmostEqual(range[1],2000.,7," Default integration max range on MAPS should be as described in MARI_Parameters.xml file")

        self.assertEqual(propman.ei_mon_spectra,[2,3]," Default ei monitors on MARI should be as described in MARI_Parameters.xml file")

 
        propman.norm_mon_integration_range = [50,1050]
        range=propman.norm_mon_integration_range
        self.assertAlmostEqual(range[0],50.,7)
        self.assertAlmostEqual(range[1],1050.,7)
        propman.ei_mon1_spec = 10
        mon_spectra = propman.ei_mon_spectra;
        self.assertEqual(mon_spectra,[10,3])
        self.assertEqual(propman.ei_mon1_spec,10)


        prop_changed = propman.getChangedProperties();
        self.assertEqual(len(prop_changed),2)
        self.assertTrue('norm_mon_integration_range' in prop_changed)

        self.assertTrue("norm_mon_integration_range" in prop_changed,"mon_norm_range should change")
        self.assertTrue("ei-mon1-spec" in prop_changed,"changing ei_mon_spectra should change ei-mon1-spec")

    def test_set_non_default_complex_value_synonims(self):
        propman = DirectPropertyManager("MAP");
        propman.test_ei2_mon_spectra = 10000;
        self.assertEqual(propman.ei_mon_spectra,[41474,10000])

        prop_changed = propman.getChangedProperties();
        self.assertEqual(len(prop_changed),1)

        self.assertTrue("ei-mon2-spec" in prop_changed,"changing test_ei2_mon_spectra should change ei-mon2-spectra")


        propman.test_mon_spectra_composite = [10000,2000]        
        self.assertEqual(propman.ei_mon_spectra,[10000,2000])

        prop_changed = propman.getChangedProperties();
        self.assertEqual(len(prop_changed),2)

        self.assertTrue("ei_mon_spectra" in prop_changed,"changing test_mon_spectra_composite should change ei_mon_spectra")

    ## HOW TO MAKE IT WORK? it fails silently
        propman.ei_mon_spectra[1]=100;
        self.assertEqual(10000,propman.ei_mon_spectra[0])
        self.assertEqual(2000,propman.ei_mon_spectra[1])


        propman.ei_mon_spectra=[100,200];
        self.assertEqual(100,propman.ei_mon_spectra[0])
        self.assertEqual(200,propman.ei_mon_spectra[1])




    def test_set_get_mono_range(self):
        # TODO : A lot of changes and tests here for mono_range
        propman = self.prop_man

        energy_incident = 100
        propman.incident_energy = energy_incident
        hi_frac = propman.monovan_hi_frac
        lo_frac = propman.monovan_lo_frac
        #propman.monovan_integr_range = None
        self.assertEqual(propman.monovan_integr_range,[lo_frac*energy_incident,hi_frac*energy_incident])


    def test_load_monitors_with_workspace(self):
        propman = self.prop_man

        self.assertTrue(propman.load_monitors_with_workspace,'MARI loads monitors with workspace by default')

        propman.load_monitors_with_workspace=True;
        self.assertTrue(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace=0;
        self.assertFalse(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace=10;
        self.assertTrue(propman.load_monitors_with_workspace)



    def test_get_default_parameter_val(self):
        propman = self.prop_man
        param = propman.getDefaultParameterValue('map_file')
        self.assertTrue(isinstance(param,str))

        param = propman.getDefaultParameterValue('ei-mon1-spec')
        self.assertTrue(isinstance(param,int))

        param = propman.getDefaultParameterValue('check_background')
        self.assertTrue(isinstance(param,bool))


    def test_save_format(self):
        propman = self.prop_man

        formats = propman.save_format;
        self.assertTrue(len(formats)==0)

        propman.save_format='unknown';
        self.assertTrue(len(propman.save_format)==0)

        propman.save_format = '.spe'
        formats = propman.save_format;
        self.assertTrue('spe' in formats)

        propman.save_format = 'nxspe'
        formats = propman.save_format;
        self.assertTrue('spe' in formats)
        self.assertTrue('nxspe' in formats)



        propman.save_format = ''
        self.assertTrue(len(propman.save_format)==0)

        propman.save_format = ['nxspe','.nxs']
        formats = propman.save_format;
        self.assertTrue('nxs' in formats)
        self.assertTrue('nxspe' in formats)

    def test_allowed_values(self):

        propman = self.prop_man
        nm = propman.normalise_method;
        self.assertEqual(nm, 'monitor-1')
        propman.normalise_method=None
        self.assertEqual(propman.normalise_method, None)
        propman.normalise_method='monitor-2'
        self.assertEqual(propman.normalise_method, 'monitor-2')
        propman.normalise_method='current'
        self.assertEqual(propman.normalise_method, 'current')

        self.assertRaises(KeyError,setattr,propman,'normalise_method','unsupported');

    def test_ki_kf(self):
        propman = self.prop_man

        self.assertTrue(propman.apply_kikf_correction)

        propman.apply_kikf_correction = True;
        self.assertTrue(propman.apply_kikf_correction)
        propman.apply_kikf_correction = False;
        self.assertFalse(propman.apply_kikf_correction)

    def test_instr_name_and_psi(self):
        propman = self.prop_man

        psi = propman.psi;
        self.assertTrue(np.isnan(psi))


        instr_name = propman.instr_name;
        self.assertEqual(instr_name,'MARI')

        propman.psi = 10
        self.assertEqual(propman.psi,10)
    def test_diag_spectra(self):
        propman = self.prop_man

        self.assertTrue(propman.diag_spectra is None)

        propman.diag_spectra ='(19,299);(399,500)'
        spectra = propman.diag_spectra
        self.assertEqual(spectra[0],(19,299));
        self.assertEqual(spectra[1],(399,500));

        propman  = DirectPropertyManager("MAP");
        spectra = propman.diag_spectra
        # (1,17280);(17281,18432);(18433,32256);(32257,41472)
        self.assertEqual(len(spectra),4)
        self.assertEqual(spectra[0],(1,17280));
        self.assertEqual(spectra[3],(32257,41472));
    def test_get_diagnostics_parameters(self):
        propman = self.prop_man

        params = propman.get_diagnostics_parameters();
        self.assertEqual(len(params),19);
        
        bkg_test_range0 = propman.background_test_range;
        bkg_test_range  = params['background_test_range'];
        bkg_range = propman.background_range;
        self.assertEqual(bkg_range,bkg_test_range)
        self.assertEqual(bkg_range,bkg_test_range0)

        propman.background_test_range = [1000,2000];
        bkg_test_range = propman.background_test_range;
        self.assertEqual(bkg_test_range,[1000,2000])

    def test_get_sample_ws_name(self):
        propman = self.prop_man

        # no workspace name if sample is not defined. 
        self.assertRaises(KeyError,propman.get_sample_ws_name)

        propman.sample_run = 0;
        ws_name = propman.get_sample_ws_name();
        self.assertEqual(ws_name,'MARI000000_spe')

        propman.sum_runs = 3
        ws_name = propman.get_sample_ws_name();
        self.assertEqual(ws_name,'MARI000000_spe-sum')


    def test_check_monovan_changed(self):
         propman = self.prop_man 
         
         non_changed = propman._check_monovan_par_changed()
         # nothing have changed initially 
         self.assertEqual(len(non_changed),3);

         propman.incident_energy = 10;
         propman.monovan_run = 139000

         non_changed = propman._check_monovan_par_changed()
         self.assertEqual(len(non_changed),2);
         propman.sample_mass = 1;
         propman.log_changed_values()

         non_changed = propman._check_monovan_par_changed()
         self.assertEqual(len(non_changed),1);
         propman.sample_rmm = 200;
         non_changed = propman._check_monovan_par_changed()
         self.assertEqual(len(non_changed),0);


         propman.log_changed_values()


    def test_set_energy_bins_and_ei(self):
        #TODO  modify and verify the energy bins for range of energies
        propman = self.prop_man

        propman.energy_bins='-30,3,10'
        bins = propman.energy_bins
        self.assertAlmostEqual(bins[0],-30)
        self.assertAlmostEqual(bins[1],3)
        self.assertAlmostEqual(bins[2],10)


        propman.energy_bins=[-20,4,100]
        bins = propman.energy_bins
        self.assertAlmostEqual(bins[0],-20)
        self.assertAlmostEqual(bins[1],4)
        self.assertAlmostEqual(bins[2],100)


        propman.incident_energy=10
        self.assertAlmostEqual(propman.incident_energy,10)
        ei = [20,30]
        propman.incident_energy=ei
        got_ei = propman.incident_energy
        for ind,en in enumerate(got_ei):
            self.assertAlmostEqual(en,ei[ind])

        propman.incident_energy='20,30'
        got_ei = propman.incident_energy
        for ind,en in enumerate(got_ei):
            self.assertAlmostEqual(en,ei[ind])


        #TODO: this one is not completed

    def test_set_defailts_from_instrument(self) :
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=100)

        SetInstrumentParameter(ws,ParameterName="TestParam1",Value="3.5",ParameterType="Number")
        SetInstrumentParameter(ws,ParameterName="TestParam2",Value="initial1",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="TestParam3",Value="initial2",ParameterType="String")

        instr = ws.getInstrument()
        propman = DirectPropertyManager(instr);

        self.assertAlmostEqual(propman.TestParam1,3.5);
        self.assertEquals(propman.TestParam2,"initial1");
        self.assertEquals(propman.TestParam3,"initial2");
        
        propman.TestParam2="gui_changed1"
        self.assertEquals(propman.TestParam2,"gui_changed1");

        SetInstrumentParameter(ws,ParameterName="TestParam2",Value="instr_changed1",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="TestParam3",Value="instr_changed2",ParameterType="String")

        self.assertAlmostEqual(propman.TestParam1,3.5);
        self.assertEquals(propman.TestParam2,"gui_changed1");
        self.assertEquals(propman.TestParam3,"initial2");
        changes = propman.getChangedProperties();
        self.assertTrue('TestParam2' in changes);
        self.assertTrue(not('TestParam3' in changes));

  

        changes = propman.update_defaults_from_instrument(ws.getInstrument());

        self.assertAlmostEqual(propman.TestParam1,3.5);
        self.assertEquals(propman.TestParam2,"gui_changed1");
        self.assertEquals(propman.TestParam3,"instr_changed2");

        self.assertTrue('TestParam2' in changes);
        self.assertTrue('TestParam3' in changes);

    def test_set_complex_defailts_from_instrument(self) :
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)

        SetInstrumentParameter(ws,ParameterName="Param1",Value="BaseParam1:BaseParam2",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="BaseParam1",Value="Val1",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="BaseParam2",Value="Val2",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="synonims",Value="ParaPara=BaseParam2",ParameterType="String")

        instr = ws.getInstrument()
        propman = DirectPropertyManager(instr);

        SampleResult = ['Val1','Val2']
        cVal = propman.Param1;
        for test,sample in zip(cVal,SampleResult):
            self.assertEqual(test,sample)

        self.assertEqual(propman.ParaPara,'Val2')
        self.assertEqual(propman.BaseParam2,'Val2')

        SetInstrumentParameter(ws,ParameterName="Param1",Value="addParam1:addParam2",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="BaseParam1",Value="OtherVal1",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="BaseParam2",Value="OtherVal2",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="addParam1",Value="Ignore1",ParameterType="String")
        SetInstrumentParameter(ws,ParameterName="addParam2",Value="Ignore2",ParameterType="String")


        changed_prop = propman.update_defaults_from_instrument(ws.getInstrument());

        #property have been changed from GUI and changes from instrument are ignored
        SampleResult = ['OtherVal1','OtherVal2']
        cVal = propman.Param1;
        for test,sample in zip(cVal,SampleResult):
            self.assertEqual(test,sample)

        self.assertEqual(propman.ParaPara,'OtherVal2')
        self.assertEqual(propman.BaseParam2,'OtherVal2')
        
        self.assertEquals(propman.BaseParam1,"OtherVal1")
       

    def test_set_all_defaults_from_instrument(self) :
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        #idf_dir = config.getString('instrumentDefinition.directory')
        idf_file=api.ExperimentInfo.getInstrumentFilename('LET','2014-05-03 23:59:59')
        ws = LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=ws)

        # Propman was defined for MARI but reduction parameters are all the same, so testing on LET
        propman = self.prop_man
        self.assertEqual(propman.ei_mon1_spec,2)

        ws = mtd['ws'];
        changed_prop=propman.update_defaults_from_instrument( ws.getInstrument(),False)
        self.assertFalse('ei-mon1-spec' in changed_prop)
        self.assertEqual(propman.ei_mon1_spec,65542)

        self.assertTrue('ei_mon_spectra' in changed_prop)
        ei_spec = propman.ei_mon_spectra
        self.assertEqual(ei_spec[0],65542)
        self.assertEqual(ei_spec[1],5506)





 #def test_default_warnings(self):
    #    tReducer = self.reducer

    #    keys_changed=['somethins_else1','sample_mass','sample_rmm','somethins_else2']

    #    self.assertEqual(0,tReducer.check_abs_norm_defaults_changed(keys_changed))

    #    keys_changed=['somethins_else1','sample_rmm','somethins_else2']
    #    self.assertEqual(1,tReducer.check_abs_norm_defaults_changed(keys_changed))

    #    keys_changed=['somethins_else1','somethins_else2']
    #    self.assertEqual(2,tReducer.check_abs_norm_defaults_changed(keys_changed))
    #def test_do_white(self) :
    #    tReducer = self.reducer
    #    monovan = 1000
    #    data = None
    #    name = tReducer.make_ckpt_name('do_white',monovan,data,'t1')
    #    self.assertEqual('do_white1000t1',name)




    

if __name__=="__main__":
        unittest.main()

