from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import os, sys
from Direct.DirectEnergyConversion import DirectEnergyConversion


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
        # THIS DOES NOT WORK AND THE QUESTION IS WHY?
        used_parameters = """<?xml version="1.0" encoding="UTF-8" ?>
            <parameter-file instrument="BASIC_RECT" valid-from="2014-01-01 00:00:01">
            <component-link name = "BASIC_RECT">

            <parameter name="det_cal_file" type="string">
               <value val="None"/>
            </parameter>

            <parameter name="load_monitors_with_workspace"  type="bool">
                <value val="True"/>
            </parameter>

            </component-link>
            </parameter-file>"""
        #LoadParameterFile(Workspace=wb_ws,ParameterXML = used_parameters);
        LoadInstrument(wb_ws,InstrumentName='MARI')

        tReducer = DirectEnergyConversion(wb_ws.getInstrument())

        white_ws = tReducer.do_white(wb_ws, None, None,None)
        self.assertTrue(white_ws)

    def test_get_abs_normalization_factor(self) :
        mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15,BinWidth=0.1)
        LoadInstrument(mono_ws,InstrumentName='MARI')

        tReducer = DirectEnergyConversion(mono_ws.getInstrument())
        tReducer.prop_man.incident_energy = 5.

        (nf1,nf2,nf3,nf4) = tReducer.get_abs_normalization_factor(mono_ws.getName(),5.)        
        self.assertAlmostEqual(nf1,0.139349147,7)
        self.assertAlmostEqual(nf1,nf2)
        self.assertAlmostEqual(nf2,nf3)
        self.assertAlmostEqual(nf3,nf4)

        # check warning. WB spectra with 0 signal indicate troubles. 
        mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15,BinWidth=0.1)
        LoadInstrument(mono_ws,InstrumentName='MARI')
        sig = mono_ws.dataY(0);
        sig[:]=0;          

        (nf1,nf2,nf3,nf4) = tReducer.get_abs_normalization_factor(mono_ws.getName(),5.)
        self.assertAlmostEqual(nf1,0.139349147,7)
        self.assertAlmostEqual(nf1,nf2)
        self.assertAlmostEqual(nf2,nf3)
        self.assertAlmostEqual(nf3,nf4)


    ##def test_diag_call(self):
    ##    tReducer = self.reducer
    ##    # should do nothing as already initialized above, but if not will initiate the instrument
    ##    tReducer.initialise("MAP")

    ##    tReducet.di


if __name__=="__main__":
        unittest.main()
