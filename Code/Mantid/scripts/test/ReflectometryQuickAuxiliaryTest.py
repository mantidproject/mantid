import unittest
from MantidFramework import mtd
mtd.initialise()
from mantid.simpleapi import *
from isis_reflgui import quick

class ReflectometryQuickAuxiliaryTest(unittest.TestCase):
    
    
    def test_cleanup(self):
        todump =CreateSingleValuedWorkspace(OutputWorkspace='_toremove', DataValue=1, ErrorValue=1)
        tokeep =CreateSingleValuedWorkspace(OutputWorkspace='tokeep', DataValue=1, ErrorValue=1)
        self.assertEqual(2, len(mtd.getObjectNames()))  
        # Should remove workspaces starting with _
        quick.cleanup()
        cleaned_object_names = mtd.getObjectNames()
        self.assertEqual(1, len(cleaned_object_names))
        self.assertEqual(True, ('tokeep' in cleaned_object_names))
        
    def test_coAdd_ws_in_ADS(self):
        inWS = CreateSingleValuedWorkspace(DataValue=1, ErrorValue=1)
        quick.coAdd('inWS', 'ProvidedName')
        outWS = mtd['_WProvidedName']
        result = CheckWorkspacesMatch(Workspace1=inWS, Workspace2=outWS)
        self.assertEquals("Success!", result)
        
    def test_coAdd_run_list(self):
        originalInstrument = config.getInstrument()
        try:
            # We have multiple runs from some MUSR files in AutoTest, lets use those.
            tempInstrument = "MUSR"
            config['default.instrument'] = tempInstrument
            runlist = '15189, 15190'
            
            # Run coAdd
            quick.coAdd(runlist, 'ProvidedName')
            
            # Get the output workspace and do some quick sanity checks
            outWS = mtd['_WProvidedName']
            self.assertEquals(outWS[0].getInstrument().getName(), tempInstrument)
            
            # Perform the addition of the two files manually
            a = LoadMuonNexus(Filename='15189')
            b = LoadMuonNexus(Filename='15190')
            c = Plus(LHSWorkspace=a[0], RHSWorkspace=b[0]) 
            
            #Check the expected calculated result against coAdd
            result = CheckWorkspacesMatch(Workspace1=c, Workspace2=outWS) 
            self.assertEquals("Success!", result)
        finally:
            config['default.instrument'] = originalInstrument.name()
        
        

if __name__ == '__main__':
    unittest.main()