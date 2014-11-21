from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
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

        self.assertEqual(len(propman.changed_properties),0);
        propman.van_mass = 100;

        self.assertAlmostEqual(propman.van_sig,0.,7)

        propman.diag_van_sig = 1;

        prop_changed = propman.changed_properties;

        self.assertEqual(len(prop_changed),2)
        self.assertTrue('van_mass' in prop_changed)
        self.assertTrue('van_sig'  in prop_changed)

    def test_overloaded_setters_getters(self):
        propman = self.prop_man

        self.assertEqual(len(propman.changed_properties),0);

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


        prop_changed = propman.changed_properties;
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

        prop_changed = propman.changed_properties;
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

        prop_changed = propman.changed_properties;
        self.assertEqual(len(prop_changed),2)
        self.assertTrue('norm_mon_integration_range' in prop_changed)

        self.assertTrue("norm_mon_integration_range" in prop_changed,"mon_norm_range should change")
        self.assertTrue("ei_mon1_spec" in prop_changed,"changing ei-mon1-spec should change ei_mon_spectra")

    def test_set_non_default_complex_value_synonims(self):
        propman = DirectPropertyManager("MAP");
        propman.test_ei2_mon_spectra = 10000;
        self.assertEqual(propman.ei_mon_spectra,[41474,10000])

        prop_changed = propman.changed_properties;
        self.assertEqual(len(prop_changed),1)

        self.assertTrue("ei_mon2_spec" in prop_changed,"changing test_ei2_mon_spectra should change ei_mon2_spectra")


        propman.test_mon_spectra_composite = [10000,2000]        
        self.assertEqual(propman.ei_mon_spectra,[10000,2000])

        prop_changed = propman.changed_properties;
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


    def test_set_energy_bins(self):
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

    def test_load_monitors_with_workspace(self):
        propman = self.prop_man

        self.assertTrue(propman.load_monitors_with_workspace,'MARI loads monitors with workspace by default')

        propman.load_monitors_with_workspace=True;
        self.assertTrue(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace=0;
        self.assertFalse(propman.load_monitors_with_workspace)
        propman.load_monitors_with_workspace=10;
        self.assertTrue(propman.load_monitors_with_workspace)


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

    def test_get_default_parameter_val(self):
        propman = self.prop_man
        param = propman.getDefaultParameterValue('map_file')
        self.assertTrue(isinstance(param,str))

        param = propman.getDefaultParameterValue('ei_mon1_spec')
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



    #def test_save_formats(self):
    #    tReducer = self.reducer;

    #    ws_name = '__empty_'+tReducer._instr_name

    #    pws = mtd[ws_name]
    #    self.assertEqual(pws.name(),ws_name);
    #    self.assertTrue(tReducer.save_format is None)
    #    # do nothing
    #    tReducer.save_results(pws,'test_path')
    #    tReducer.test_name='';
    #    def f_spe(workspace, filename):
    #            tReducer.test_name += (workspace.name()+'_file_spe_' + filename)
    #    def f_nxspe(workspace, filename):
    #            tReducer.test_name += (workspace.name()+'_file_nxspe_' + filename)
    #    def f_nxs(workspace, filename):
    #            tReducer.test_name += (workspace.name()+'_file_nxs_' + filename)


    #    # redefine test save methors to produce test ouptut
    #    tReducer._DirectEnergyConversion__save_formats['.spe']=lambda workspace,filename: f_spe(workspace,filename);
    #    tReducer._DirectEnergyConversion__save_formats['.nxspe']=lambda workspace,filename : f_nxspe(workspace,filename);
    #    tReducer._DirectEnergyConversion__save_formats['.nxs']=lambda workspace,filename : f_nxs(workspace,filename);



    #    # set non-exisiting format
    #    tReducer.save_format = 'non-existing-format'
    #    self.assertTrue(tReducer.save_format is None)

    #    tReducer.save_format = 'spe'
    #    self.assertTrue(tReducer.save_format is None)

    #    tReducer.save_format = '.spe'
    #    self.assertEqual(tReducer.save_format,['.spe'])

    #    tReducer.test_name='';
    #    tReducer.save_results(pws)
    #    self.assertEquals(ws_name+'_file_spe_'+ws_name+'.spe',tReducer.test_name)
    #    file_long_name = ws_name+'_file_spe_other_file_name.spe'

    #    tReducer.test_name='';
    #    tReducer.save_results(pws,'other_file_name')
    #    self.assertEquals(file_long_name,tReducer.test_name)

    #    file_long_name=ws_name+'_file_nxspe_ofn.nxspe'+ws_name+'_file_nxs_ofn.nxs'
    #    tReducer.test_name='';
    #    tReducer.save_results(pws,'ofn',['.nxspe','.nxs'])
    #    self.assertEquals(file_long_name,tReducer.test_name)

    #    #clear all previous default formats
    #    tReducer.save_format=[];
    #    self.assertTrue(tReducer.save_format is None)

    #    format_list = ['.nxspe','.nxs','.spe']
    #    file_long_name = '';
    #    tReducer.save_format = format_list;
    #    for i in xrange(len(format_list)):
    #        self.assertEqual(tReducer.save_format[i],format_list[i]);
    #        end = len(format_list[i]);
    #        file_long_name+=ws_name+'_file_'+format_list[i][1:end]+'_ofn'+format_list[i]

    #    tReducer.test_name='';
    #    tReducer.save_results(pws,'ofn')
    #    self.assertEquals(file_long_name,tReducer.test_name)



if __name__=="__main__":
        unittest.main()

