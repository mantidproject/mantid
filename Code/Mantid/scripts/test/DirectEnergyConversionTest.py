import os, sys
#os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
from Direct.DirectEnergyConversion import DirectEnergyConversion
from Direct.PropertyManager  import PropertyManager
import Direct.dgreduce as dgreduce


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
        self.assertEqual(prop_man.instr_name,"MARI")


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

    def test_diagnostics_wb(self):
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        LoadInstrument(wb_ws,InstrumentName='MARI')

        tReducer = DirectEnergyConversion(wb_ws.getInstrument())


        mask_workspace=tReducer.diagnose(wb_ws);
        self.assertTrue(mask_workspace)

    def test_do_white_wb(self) :
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        #LoadParameterFile(Workspace=wb_ws,ParameterXML = used_parameters);
        LoadInstrument(wb_ws,InstrumentName='MARI')

        tReducer = DirectEnergyConversion(wb_ws.getInstrument())

        white_ws = tReducer.do_white(wb_ws, None, None,None)
        self.assertTrue(white_ws)
 

    def test_get_set_attributes(self):
        tReducer = self.reducer

        # prohibit accessing non-existing property
        self.assertRaises(KeyError,getattr,tReducer,'non_existing_property')
        self.assertRaises(KeyError,setattr,tReducer,'non_existing_property',1000)
        self.assertRaises(KeyError,getattr,tReducer,'non_existing_property')

        # allow simple creation of a system property
        self.assertRaises(KeyError,getattr,tReducer,'_new_system_property')
        setattr(tReducer,'_new_system_property',True)
        self.assertTrue(tReducer._new_system_property)

        # direct and indirect access to prop_man properties
        tReducer.sample_run = None
        #sample run has not been defined
        self.assertTrue(getattr(tReducer,'sample_run') is None)
        prop_man = tReducer.prop_man
        self.assertTrue(getattr(prop_man ,'sample_run') is None)
        # define sample run
        tReducer.sample_run =10234
        self.assertEqual(tReducer.sample_run,10234)
        self.assertEqual(tReducer.prop_man.sample_run,10234)



    def test_get_abs_normalization_factor(self) :
        mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15,BinWidth=0.1,function='Flat background')
        LoadInstrument(mono_ws,InstrumentName='MARI')

        tReducer = DirectEnergyConversion(mono_ws.getInstrument())
        tReducer.prop_man.incident_energy = 5.
        tReducer.prop_man.monovan_integr_range=[-10,10]

        (nf1,nf2,nf3,nf4) = tReducer.get_abs_normalization_factor(mono_ws.getName(),5.)        
        self.assertAlmostEqual(nf1,0.58561121802167193,7)
        self.assertAlmostEqual(nf1,nf2)
        self.assertAlmostEqual(nf2,nf3)
        self.assertAlmostEqual(nf3,nf4)

        # check warning. WB spectra with 0 signal indicate troubles. 
        mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15,BinWidth=0.1,function='Flat background')
        LoadInstrument(mono_ws,InstrumentName='MARI')
        sig = mono_ws.dataY(0);
        sig[:]=0;          

        (nf1,nf2,nf3,nf4) = tReducer.get_abs_normalization_factor(mono_ws.getName(),5.)
        self.assertAlmostEqual(nf1,0.585611218022,7)
        self.assertAlmostEqual(nf1,nf2)
        self.assertAlmostEqual(nf2,nf3)
        self.assertAlmostEqual(nf3,nf4)

    def test_dgreduce_works(self):
        """ Test for old interface """
        run_ws = CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        LoadInstrument(run_ws,InstrumentName='MARI')
        #mono_ws = CloneWorkspace(run_ws);
        wb_ws   = CloneWorkspace(run_ws);
        #wb_ws=CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=10000)




        dgreduce.setup('MAR');
        par = {};
        par['ei_mon_spectra']=[4,5]
        par['abs_units_van_range']=[-4000,8000]
        # overwrite parameters, which are necessary from command line, but we want them to have test values
        dgreduce.getReducer().map_file=None
        dgreduce.getReducer().monovan_mapfile=None
        dgreduce.getReducer().mono_correction_factor = 1
        #abs_units(wb_for_run,sample_run,monovan_run,wb_for_monovanadium,samp_rmm,samp_mass,ei_guess,rebin,map_file='default',monovan_mapfile='default',**kwargs):
        ws = dgreduce.abs_units(wb_ws,run_ws,None,wb_ws,10,100,8.8,[-10,0.1,7],None,None,**par)
        self.assertTrue(isinstance(ws,api.MatrixWorkspace))
    


    ##def test_diag_call(self):
    ##    tReducer = self.reducer
    ##    # should do nothing as already initialized above, but if not will initiate the instrument
    ##    tReducer.initialise("MAP")

    ##    tReducet.di
    def test_energy_to_TOF_range(self):

        ws = Load(Filename='MAR11001.raw',LoadMonitors='Include')
        
        en_range = [0.8*13,13,1.2*13]
        detIDs=[1,2,3,10]
        red = DirectEnergyConversion()
        TRange = red.get_TOF_for_to_energies(ws,en_range,detIDs)
        for ind,detID in enumerate(detIDs):
            tof = TRange[ind]
            y = [1]*(len(tof)-1)
            ind = ws.getIndexFromSpectrumNumber(detID)
            ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace='_ws_template', WorkspaceIndex=ind)
            CreateWorkspace(OutputWorkspace='TOF_WS',NSpec = 1,DataX=tof,DataY=y,UnitX='TOF',ParentWorkspace='_ws_template')
            EnWs=ConvertUnits(InputWorkspace='TOF_WS',Target='Energy',EMode='Elastic')

            eni = EnWs.dataX(0)
            for samp,rez in zip(eni,en_range): self.assertAlmostEqual(samp,rez)

        # Now Test shifted:
        ei,mon1_peak,mon1_index,tzero = GetEi(InputWorkspace=ws, Monitor1Spec=int(2), Monitor2Spec=int(3),EnergyEstimate=13)
        ScaleX(InputWorkspace='ws',OutputWorkspace='ws',Operation="Add",Factor=-mon1_peak,InstrumentParameter="DelayTime",Combine=True)
        ws = mtd['ws']

        mon1_det = ws.getDetector(1)
        mon1_pos = mon1_det.getPos()
        src_name = ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(Workspace='ws',ComponentName= src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)

        # Does not work for monitor 2 as it has been moved to mon2 position and there all tof =0
        detIDs=[1,3,10]
        TRange1 = red.get_TOF_for_to_energies(ws,en_range,detIDs)

        for ind,detID in enumerate(detIDs):
            tof = TRange1[ind]
            y = [1]*(len(tof)-1)
            ind = ws.getIndexFromSpectrumNumber(detID)
            ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace='_ws_template', WorkspaceIndex=ind)
            CreateWorkspace(OutputWorkspace='TOF_WS',NSpec = 1,DataX=tof,DataY=y,UnitX='TOF',ParentWorkspace='_ws_template')
            EnWs=ConvertUnits(InputWorkspace='TOF_WS',Target='Energy',EMode='Elastic')

            eni = EnWs.dataX(0)
            for samp,rez in zip(eni,en_range): self.assertAlmostEqual(samp,rez)





      



if __name__=="__main__":
        unittest.main()
