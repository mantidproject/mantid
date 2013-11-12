import unittest
import numpy
from mantid.simpleapi import *
from isis_reflectometry import quick

class ReflectometryQuickAuxiliaryTest(unittest.TestCase):
    
    __wsName = None
    
    
    def __init__(self, methodName='runTest'):
        super(ReflectometryQuickAuxiliaryTest, self).__init__(methodName)
        self.__wsName = "TestWorkspace"
        LoadISISNexus(Filename='POLREF00004699', OutputWorkspace=self.__wsName)
    
    def __del__(self):
        DeleteWorkspace(mtd[self.__wsName])
        
    
    def test_cleanup(self):
        numObjectsOriginal = len(mtd.getObjectNames())
        todump =CreateSingleValuedWorkspace(OutputWorkspace='_toremove', DataValue=1, ErrorValue=1)
        tokeep =CreateSingleValuedWorkspace(OutputWorkspace='tokeep', DataValue=1, ErrorValue=1)
        self.assertEqual(numObjectsOriginal+2, len(mtd.getObjectNames()))  
        # Should remove workspaces starting with _
        quick.cleanup()
        cleaned_object_names = mtd.getObjectNames()
        self.assertEqual(numObjectsOriginal+1, len(cleaned_object_names))
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
        
        expectedInstrument = "POLREF"
        
        # Test with group workspace as input
        instrument = quick.groupGet(self.__wsName, 'inst')
        self.assertEquals(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws group")
        
        # Test with single workspace as input
        instrument = quick.groupGet(mtd[self.__wsName][0].name(), 'inst')
        self.assertEquals(expectedInstrument, instrument.getName(), "Did not fetch the instrument from ws")
        
    
    def test_groupGet_histogram_count(self):
        expectedNHistograms = mtd[self.__wsName][0].getNumberHistograms()
        
        # Test with group workspace as input
        nHistograms = quick.groupGet(self.__wsName, 'wksp')
        self.assertEquals(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws group")
        
        # Test with single workspace as input
        nHistograms = quick.groupGet(mtd[self.__wsName][0].name(), 'wksp')
        self.assertEquals(expectedNHistograms, nHistograms, "Did not fetch the n histograms from ws")
        
    
    def test_groupGet_log_single_value(self):
        
        expectedNPeriods = 2
        
        # Test with group workspace as input
        nPeriods = quick.groupGet(self.__wsName, 'samp', 'nperiods')
        self.assertEquals(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws group")
        
        # Test with single workspace as input
        nPeriods = quick.groupGet(mtd[self.__wsName][0].name(), 'samp', 'nperiods')
        self.assertEquals(expectedNPeriods, nPeriods, "Did not fetch the number of periods from ws")
        
    def test_groupGet_multi_value_log(self):

        # Expected start theta, taken from the last value of the time series log.
        expectedStartTheta = 0.4903 
        
        # Test with group workspace as input
        stheta = quick.groupGet(self.__wsName, 'samp', 'stheta')
        self.assertEquals(expectedStartTheta, round(float(stheta), 4))
        
        # Test with single workspace as input
        stheta = quick.groupGet(mtd[self.__wsName][0].name(), 'samp', 'stheta')
        self.assertEquals(expectedStartTheta, round(float(stheta), 4))
        
    def test_groupGet_unknown_log_error_code(self):
        errorCode = 0
        # Test with group workspace as input
        self.assertEquals(errorCode, quick.groupGet(self.__wsName, 'samp','MADE-UP-LOG-NAME'))
        
        # Test with group workspace as input
        self.assertEquals(errorCode, quick.groupGet(mtd[self.__wsName][0].name(), 'samp','MADE-UP-LOG-NAME'))
        

if __name__ == '__main__':
    unittest.main()
