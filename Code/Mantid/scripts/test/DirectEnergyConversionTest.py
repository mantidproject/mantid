from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import os, sys
from DirectEnergyConversion import DirectEnergyConversion


#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class DirectEnergyConversionTest(unittest.TestCase):
    def __init__(self, methodName):
        self.reducer = None
        return super(DirectEnergyConversionTest, self).__init__(methodName)

    def setUp(self):
        if self.reducer == None or type(self.reducer) != type(DirectEnergyConversion):
            self.reducer = DirectEnergyConversion("MAR");
    def tearDown(self):
        pass

   #def test_build_coupled_keys_dict_simple(self):
   #    params = ["];

    def test_init_reducer(self):
        tReducer = self.reducer
        self.assertFalse(tReducer.prop_man is None)

        prop_man = tReducer.prop_man;
        self.assertEqual(prop_man.instr_name,"MAR")


    def test_save_formats(self):
        tReducer = self.reducer;


        tws =CreateSampleWorkspace(Function='Flat background', NumBanks=1, BankPixelWidth=1, NumEvents=10, XUnit='DeltaE', XMin=-10, XMax=10, BinWidth=0.1)


        self.assertTrue(len(tReducer.prop_man.save_format) ==0)
        # do nothing
        tReducer.save_results(tws,'save_formats_test_file')
        #
        file = FileFinder.getFullPath('save_formats_test_file.spe');
        self.assertTrue(len(file)==0);
        file = FileFinder.getFullPath('save_formats_test_file.nxspe');
        self.assertTrue(len(file)==0);
        file = FileFinder.getFullPath('save_formats_test_file');
        self.assertTrue(len(file)==0);
        file = FileFinder.getFullPath('save_formats_test_file.nxs');
        self.assertTrue(len(file)==0);



        # redefine test save methors to produce test ouptut

        tReducer.prop_man.save_format=['spe','nxspe','nxs']

        tReducer.save_results(tws,'save_formats_test_file.tt');

        file = FileFinder.getFullPath('save_formats_test_file.spe');
        self.assertTrue(len(file)>0);
        os.remove(file);
        file = FileFinder.getFullPath('save_formats_test_file.nxspe');
        self.assertTrue(len(file)>0);
        os.remove(file);
        file = FileFinder.getFullPath('save_formats_test_file.nxs');
        self.assertTrue(len(file)>0);
        os.remove(file);

        tReducer.prop_man.save_format=None;
        # do nothing
        tReducer.save_results(tws,'save_formats_test_file.tt');
        file = FileFinder.getFullPath('save_formats_test_file.tt');
        self.assertTrue(len(file)==0);

        # save file with given extension on direct request:
        tReducer.save_results(tws,'save_formats_test_file.nxs');
        file = FileFinder.getFullPath('save_formats_test_file.nxs');
        self.assertTrue(len(file)>0);
        os.remove(file);

        tReducer.prop_man.save_format=[];
        # do nothing
        tReducer.save_results(tws,'save_formats_test_file');
        file = FileFinder.getFullPath('save_formats_test_file');
        self.assertTrue(len(file)==0);


        # save files with extensions on request
        tReducer.save_results(tws,'save_formats_test_file',['nxs','.nxspe']);
        file = FileFinder.getFullPath('save_formats_test_file.nxs');
        self.assertTrue(len(file)>0);
        os.remove(file);
        file = FileFinder.getFullPath('save_formats_test_file.nxspe');
        self.assertTrue(len(file)>0);
        os.remove(file);

        # this is strange feature.
        self.assertTrue(len(tReducer.prop_man.save_format) ==2)




    #def test_set_non_default_wrong_value(self):
    #    tReducer = self.reducer
    #    # should do nothing as already initialized above
    #    tReducer.initialise("MAP")

    #    # non-existing property can not be set!
    #    self.assertRaises(KeyError,tReducer.set_input_parameters,non_existing_property="Something_Meaningfull")

    #def test_set_non_default_simple_value(self):
    #    tReducer = self.reducer
    #    # should do nothing as already initialized above
    #    tReducer.initialise("MAP");

    #    prop_changed=tReducer.set_input_parameters(van_mass=100,det_cal_file='det4to1_1912.dat')
    #    self.assertTrue("van_mass" in prop_changed)
    #    self.assertTrue("det_cal_file" in prop_changed)

    #    self.assertEqual(tReducer.van_mass,100);
    #    self.assertEqual(tReducer.det_cal_file,'det4to1_1912.dat');

    #    self.assertAlmostEqual(tReducer.van_sig,0.,7)
    #    kw=dict();
    #    kw["vanadium-mass"]=200
    #    kw["diag_van_median_sigma"]=1
    #    kw["det_cal_file"]=None
    #    kw["save_format"]=''
    #    prop_changed=tReducer.set_input_parameters(**kw)

    #    self.assertTrue("van_mass" in prop_changed,"vanadium-mass should correspond to van_mass")
    #    self.assertTrue("van_sig" in prop_changed," diag_van_median_sigma should correspond to van_sig ")

    #    self.assertEqual(tReducer.van_mass,200);
    #    self.assertEqual(tReducer.det_cal_file,None);
    #    self.assertAlmostEqual(tReducer.van_sig,1.,7)



    #def test_set_non_default_complex_value(self):
    #    tReducer = self.reducer
    #    # should do nothing as already initialized above, but if not will initiate the instrument
    #    tReducer.initialise("MAP");

    #    range = tReducer.norm_mon_integration_range
    #    self.assertAlmostEqual(range[0],1000.,7," Default integration min range on MAPS should be as described in MAPS_Parameters.xml file")
    #    self.assertAlmostEqual(range[1],2000.,7," Default integration max range on MAPS should be as described in MAPS_Parameters.xml file")
    #    self.assertEqual(tReducer.ei_mon_spectra,[41474,41475]," Default ei monitors on MAPS should be as described in MAPS_Parameters.xml file")

    #    self.assertRaises(KeyError,tReducer.set_input_parameters,mon_norm_range=1)
    #    self.assertRaises(KeyError,tReducer.set_input_parameters,mon_norm_range=[10,100,100])

    #    kw=dict();
    #    kw["norm_mon_integration_range"]=[50,1050]
    #    kw["ei-mon1-spec"]=10
    #    prop_changed=tReducer.set_input_parameters(**kw)

    #    range=tReducer.norm_mon_integration_range
    #    self.assertAlmostEqual(range[0],50.,7)
    #    self.assertAlmostEqual(range[1],1050.,7)
    #    self.assertEqual(tReducer.ei_mon_spectra,[10,41475])

    #    self.assertTrue("norm_mon_integration_range" in prop_changed,"mon_norm_range should change")
    #    self.assertTrue("ei_mon_spectra" in prop_changed,"changing ei-mon1-spec should change ei_mon_spectra")

    #def test_set_non_default_complex_value_synonims(self):
    #    tReducer = self.reducer
    #    # should do nothing as already initialized above, but if not will initiate the instrument
    #    tReducer.initialise("MAP");
    #    #
    #    kw = dict();
    #    kw["test_ei2_mon_spectra"]=10000
    #    prop_changed=tReducer.set_input_parameters(**kw)

    #    self.assertEqual(tReducer.ei_mon_spectra,[41474,10000])
    #    self.assertTrue("ei_mon_spectra" in prop_changed,"changing test_ei2_mon_spectra should change ei_mon_spectra")

    #    prop_changed=tReducer.set_input_parameters(test_mon_spectra_composite=[10000,2000])

    #    self.assertEqual(tReducer.ei_mon_spectra,[10000,2000])
    #    self.assertTrue("ei_mon_spectra" in prop_changed,"changing test_mon_spectra_composite should change ei_mon_spectra")

    #def test_set_get_mono_range(self):
    #    tReducer = self.reducer
    #    # should do nothing as already initialized above, but if not will initiate the instrument
    #    tReducer.initialise("MAP");

    #    energy_incident = 100
    #    tReducer.incident_energy = energy_incident
    #    hi_frac = tReducer.monovan_hi_frac
    #    lo_frac = tReducer.monovan_lo_frac
    #    tReducer.monovan_integr_range = None
    #    self.assertEqual(tReducer.monovan_integr_range,[lo_frac*energy_incident,hi_frac*energy_incident])

    #def test_comlex_get(self):
    #    tReducer = self.reducer

    #    van_rmm = tReducer.van_rmm;
    #    self.assertEqual(50.9415,van_rmm)

    #def test_comlex_set(self):
    #    tReducer = self.reducer

    #    tReducer.energy_bins='-30,3,10'
    #    bins = tReducer.energy_bins
    #    self.assertAlmostEqual(bins[0],-30)
    #    self.assertAlmostEqual(bins[1],3)
    #    self.assertAlmostEqual(bins[2],10)


    #    tReducer.energy_bins=[-20,4,100]
    #    bins = tReducer.energy_bins
    #    self.assertAlmostEqual(bins[0],-20)
    #    self.assertAlmostEqual(bins[1],4)
    #    self.assertAlmostEqual(bins[2],100)

    #    tReducer.map_file = "some_map"
    #    self.assertEqual("some_map.map",tReducer.map_file)
    #    tReducer.monovan_mapfile = "other_map"
    #    self.assertEqual("other_map.map",tReducer.monovan_mapfile)

    #    tReducer.monovan_mapfile = "other_map.myExt"
    #    self.assertEqual("other_map.myExt",tReducer.monovan_mapfile)

    #    tReducer.save_format = 'unknown'
    #    self.assertTrue(tReducer.save_format is None)

    #    tReducer.save_format = '.spe'
    #    self.assertEqual(['.spe'],tReducer.save_format)

    #def test_set_format(self):
    #    tReducer = self.reducer

    #    tReducer.save_format = '';
    #    self.assertTrue(tReducer.save_format is None)

    #    #self.assertRaises(KeyError,tReducer.energy_bins=20,None)
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

    #def test_get_parameter(self):
    #    tReducer = self.reducer
    #    param = tReducer.get_default_parameter('map_file')
    #    self.assertTrue(isinstance(param,str))

    #    param = tReducer.get_default_parameter('ei-mon1-spec')
    #    self.assertTrue(isinstance(param,int))

    #    param = tReducer.get_default_parameter('check_background')
    #    self.assertTrue(isinstance(param,bool))

    #    print "Instr_type :",type(tReducer.instrument)




    #def test_set_spectra_to_mon(self):
    #    tReducer = self.reducer;

    #    self.assertTrue(tReducer.spectra_to_monitors_list is None);

    #    tReducer.spectra_to_monitors_list = 35;
    #    self.assertTrue(isinstance(tReducer.spectra_to_monitors_list,list));
    #    self.assertEquals(35,tReducer.spectra_to_monitors_list[0]);

    #    tReducer.spectra_to_monitors_list = None;
    #    self.assertTrue(tReducer.spectra_to_monitors_list is None);
    #    tReducer.spectra_to_monitors_list = 'None';
    #    self.assertTrue(tReducer.spectra_to_monitors_list is None);
    #    tReducer.spectra_to_monitors_list = [];
    #    self.assertTrue(tReducer.spectra_to_monitors_list is None);

    #    tReducer.spectra_to_monitors_list = '467';
    #    self.assertEquals(467,tReducer.spectra_to_monitors_list[0]);

    #    tReducer.spectra_to_monitors_list = '467,444';
    #    self.assertEquals(467,tReducer.spectra_to_monitors_list[0]);
    #    self.assertEquals(444,tReducer.spectra_to_monitors_list[1]);

    #    tReducer.spectra_to_monitors_list = ['467','444'];
    #    self.assertEquals(467,tReducer.spectra_to_monitors_list[0]);
    #    self.assertEquals(444,tReducer.spectra_to_monitors_list[1]);




    #def test_process_copy_spectra_to_monitors(self):
    #    pass
    #def test_set_get_ei_monitor(self):
    #    tReducer = self.reducer;

    #    self.assertEqual(41474,tReducer.ei_mon_spectra[0])
    #    self.assertEqual(41475,tReducer.ei_mon_spectra[1])

    ## HOW TO MAKE IT WORK? it fails silently
    ##    tReducer.ei_mon_spectra[1]=100;
    ##    self.assertEqual(41474,tReducer.ei_mon_spectra[0])
    ##    self.assertEqual(100,tReducer.ei_mon_spectra[1])


    #    tReducer.ei_mon_spectra=[100,200];
    #    self.assertEqual(100,tReducer.ei_mon_spectra[0])
    #    self.assertEqual(200,tReducer.ei_mon_spectra[1])

    #    tReducer.init_idf_params(True);
    #    self.assertEqual(41474,tReducer.ei_mon_spectra[0])
    #    self.assertEqual(41475,tReducer.ei_mon_spectra[1])

    #def test_load_monitors_with_workspacer(self):
    #    tReducer =self.reducer;

    #    self.assertFalse(tReducer.load_monitors_with_workspace)

    #    tReducer.load_monitors_with_workspace=True;
    #    self.assertTrue(tReducer.load_monitors_with_workspace)
    #    tReducer.load_monitors_with_workspace=0;
    #    self.assertFalse(tReducer.load_monitors_with_workspace)
    #    tReducer.load_monitors_with_workspace=10;
    #    self.assertTrue(tReducer.load_monitors_with_workspace)

    ##def test_diag_call(self):
    ##    tReducer = self.reducer
    ##    # should do nothing as already initialized above, but if not will initiate the instrument
    ##    tReducer.initialise("MAP")

    ##    tReducet.di


if __name__=="__main__":
        unittest.main()
