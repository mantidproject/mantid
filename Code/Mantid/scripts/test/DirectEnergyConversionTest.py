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

    def test_diagnostics(self):
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)

        tReducer = self.reducer;

        tReducer.diagnose(wb_ws);


    def test_do_white(self) :
        tReducer = self.reducer
        wb_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000)
        tReducer.initialise(wb_ws.getInstrument())
        tReducer.do_white(wb_ws, None, None,None)

        #self.assertEqual('do_white1000t1',name)

    #def test_get_parameter(self):
    #    tReducer = self.reducer
    #    param = tReducer.get_default_parameter('map_file')
    #    self.assertTrue(isinstance(param,str))

    #    param = tReducer.get_default_parameter('ei-mon1-spec')
    #    self.assertTrue(isinstance(param,int))

    #    param = tReducer.get_default_parameter('check_background')
    #    self.assertTrue(isinstance(param,bool))

    #    print "Instr_type :",type(tReducer.instrument)



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
