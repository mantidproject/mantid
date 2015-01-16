import os
os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
from mantid.simpleapi import *
from mantid import api
import unittest
import inspect
import numpy as np
import sys
from Direct.PropertyManager import PropertyManager
from Direct.RunDescriptor   import RunDescriptor


#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorTest(unittest.TestCase):

    def __init__(self, methodName):
        self.prop_man = PropertyManager("MAR")
        return super(RunDescriptorTest, self).__init__(methodName)

    def setUp(self):
        if self.prop_man == None or type(self.prop_man) != type(PropertyManager):
            self.prop_man  = PropertyManager("MAR");
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
        return mtd[tmp_ws_name].getInstrument()

 
    def test_descr_basic(self):
        propman  = self.prop_man

        self.assertTrue(propman.sample_run is None)
        self.assertTrue(PropertyManager.sample_run.get_workspace() is None)
        
        propman.sample_run = 10;
        self.assertEqual(propman.sample_run,10)

        run_ws = CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=100)
        propman.sample_run = 'run_ws'
        self.assertEqual(PropertyManager.sample_run.run_number(),0)

        stor_ws = PropertyManager.sample_run.get_workspace()
        rez = CheckWorkspacesMatch(Workspace1=run_ws,Workspace2=stor_ws)

        self.assertEqual(rez,'Success!')

    def test_descr_dependend(self):
        propman  = self.prop_man
        propman.wb_run = 100
        self.assertEqual(propman.wb_run,100)
        self.assertEqual(propman.wb_for_monovan_run,100)

        propman.wb_for_monovan_run = 200
        self.assertEqual(propman.wb_for_monovan_run,200)
        self.assertEqual(propman.wb_run,100)

    def test_find_file(self):
        propman  = self.prop_man
        propman.sample_run = 11001

        file=PropertyManager.sample_run.find_file()
        self.assertTrue(len(file)>0)

        ext = PropertyManager.sample_run.get_file_ext()
        self.assertEqual(ext,'.raw')

        PropertyManager.sample_run.set_file_ext('nxs')
        ext = PropertyManager.sample_run.get_file_ext()
        self.assertEqual(ext,'.nxs')

        test_dir = config.getString('defaultsave.directory')

        testFile1=os.path.normpath(test_dir+'MAR101111.nxs')
        testFile2=os.path.normpath(test_dir+'MAR101111.raw')

        # create two test files to check search for appropriate extension
        f=open(testFile1,'w')
        f.write('aaaaaa');
        f.close()

        f=open(testFile2,'w')
        f.write('bbbb')
        f.close()


        propman.sample_run = 101111
        PropertyManager.sample_run.set_file_ext('nxs')

        file=PropertyManager.sample_run.find_file()
        self.assertEqual(testFile1,os.path.normpath(file))
        PropertyManager.sample_run.set_file_ext('.raw')
        file=PropertyManager.sample_run.find_file()
        self.assertEqual(testFile2,os.path.normpath(file))

        os.remove(testFile1)
        os.remove(testFile2)

    def test_load_workspace(self):
        propman  = self.prop_man

        # MARI run with number 11001 and extension raw must among unit test files
        propman.sample_run = 11001
        PropertyManager.sample_run.set_file_ext('raw')

        ws = PropertyManager.sample_run.get_workspace()

        self.assertTrue(isinstance(ws, api.Workspace))
        self.assertEqual(ws.name(), PropertyManager.sample_run.get_ws_name())

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(isinstance(mon_ws, api.Workspace))
        self.assertEqual(mon_ws.name(),ws.name()) 
    
    def test_copy_spectra2monitors(self):
        propman  = self.prop_man
        run_ws = CreateSampleWorkspace( Function='Multiple Peaks', WorkspaceType = 'Event',NumBanks=1, BankPixelWidth=5, NumEvents=100)
        run_ws_monitors = CreateSampleWorkspace( Function='Multiple Peaks', WorkspaceType = 'Histogram',NumBanks=1, BankPixelWidth=1, NumEvents=100)
        self.assertEqual(run_ws_monitors.getNumberHistograms(),1)


        propman.monovan_run = run_ws
        propman.spectra_to_monitors_list = 3

        mon_ws = PropertyManager.monovan_run.get_monitors_ws()
        self.assertTrue(isinstance(mon_ws, api.Workspace))
        self.assertEqual(mon_ws.getNumberHistograms(),2)
        self.assertEqual(mon_ws.getIndexFromSpectrumNumber(3),1)

    def test_ws_name(self):
        run_ws = CreateSampleWorkspace( Function='Multiple Peaks', NumBanks=1, BankPixelWidth=4, NumEvents=100)
        propman  = self.prop_man
        propman.sample_run = run_ws

        self.assertEqual(PropertyManager.sample_run.get_ws_name(),'SR_run_ws')
        ws = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws.name(),'SR_run_ws')

        propman.sample_run = ws
        self.assertEqual(PropertyManager.sample_run.get_ws_name(),'SR_run_ws')
        self.assertTrue('SR_run_ws' in mtd)

        propman.sample_run = 11001
        self.assertFalse('SR_run_ws' in mtd)

        propman.load_monitors_with_workspace = False
        ws = PropertyManager.sample_run.get_workspace()
        ws_name = ws.name()
        self.assertEqual(PropertyManager.sample_run.get_ws_name(),ws_name)
        self.assertTrue(ws_name in mtd)
        self.assertTrue(ws_name+'_monitors' in mtd)

        propman.sample_run = ws
        self.assertEqual(PropertyManager.sample_run.get_ws_name(),ws_name)
        self.assertTrue(ws_name in mtd)

        ws1 = PropertyManager.sample_run.get_workspace()
        self.assertEqual(ws1.name(),ws_name)

        # if no workspace is available, attempt to get workspace name fails
        DeleteWorkspace(ws_name)
        self.assertRaises(RuntimeError,PropertyManager.sample_run.get_ws_name)

        propman.sample_run = None
        self.assertFalse(ws_name+'_monitors' in mtd)
        # name of empty property workspace 
        self.assertEqual(PropertyManager.sample_run.get_ws_name(),'SR_')


        # TODO: implement sum
        #propman.sum_runs = 3
        #ws_name = propman.get_sample_ws_name();
        #self.assertEqual(ws_name,'MARI000000_spe-sum')


if __name__=="__main__":
    unittest.main()
