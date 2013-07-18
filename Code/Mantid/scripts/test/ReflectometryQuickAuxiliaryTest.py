import unittest
import numpy
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
        
        DeleteWorkspace(tokeep)
        
    def test_coAdd_ws_in_ADS(self):
        inWS = CreateSingleValuedWorkspace(DataValue=1, ErrorValue=1)
        quick.coAdd('inWS', 'ProvidedName')
        outWS = mtd['_WProvidedName']
        result = CheckWorkspacesMatch(Workspace1=inWS, Workspace2=outWS)
        self.assertEquals("Success!", result)
        DeleteWorkspace(outWS)
        
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
            DeleteWorkspace(a[0])
            DeleteWorkspace(b[0])
            DeleteWorkspace(c)
            DeleteWorkspace(outWS)
        
    def test_groupGet_instrument(self):
        wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=wsName)
        
        expectedInstrument = "POLREF"
        
        # Test with group workspace as input
        instrument = quick.groupGet(wsName, 'inst')
        self.assertEquals(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws group")
        
        # Test with single workspace as input
        instrument = quick.groupGet(mtd[wsName][0].name(), 'inst')
        self.assertEquals(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws")
        
        DeleteWorkspace(mtd[wsName])
    
    def test_groupGet_histogram_count(self):
        wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=wsName)
        expectedNHistograms = mtd[wsName][0].getNumberHistograms()
        
        # Test with group workspace as input
        nHistograms = quick.groupGet(wsName, 'wksp')
        self.assertEquals(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws group")
        
        # Test with single workspace as input
        nHistograms = quick.groupGet(mtd[wsName][0].name(), 'wksp')
        self.assertEquals(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws")
        
        DeleteWorkspace(mtd[wsName])
    
    def test_groupGet_log_single_value(self):
        wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=wsName)
        
        expectedNPeriods = 2
        
        # Test with group workspace as input
        nPeriods = quick.groupGet(wsName, 'samp', 'nperiods')
        self.assertEquals(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws group")
        
        # Test with single workspace as input
        nPeriods = quick.groupGet(mtd[wsName][0].name(), 'samp', 'nperiods')
        self.assertEquals(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws")
        
        DeleteWorkspace(mtd[wsName])
        
    def test_groupGet_multi_value_log(self):
        wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=wsName)
        
        # Expected start theta, taken from the last value of the time series log.
        expectedStartTheta = 0.4903 
        
        # Test with group workspace as input
        stheta = quick.groupGet(wsName, 'samp', 'stheta')
        self.assertEquals(expectedStartTheta, round(float(stheta), 4))
        
        # Test with single workspace as input
        stheta = quick.groupGet(mtd[wsName][0].name(), 'samp', 'stheta')
        self.assertEquals(expectedStartTheta, round(float(stheta), 4))
        
        DeleteWorkspace(mtd[wsName])
        
    def test_groupGet_unknown_log_error_code(self):
        wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=wsName)
        
        errorCode = 0
        # Test with group workspace as input
        self.assertEquals(errorCode, quick.groupGet(wsName, 'samp','MADE-UP-LOG-NAME'))
        
        # Test with group workspace as input
        self.assertEquals(errorCode, quick.groupGet(mtd[wsName][0].name(), 'samp','MADE-UP-LOG-NAME'))
        

if __name__ == '__main__':
    unittest.main()