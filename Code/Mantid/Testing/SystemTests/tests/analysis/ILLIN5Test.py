import stresstesting

from mantid.api import MatrixWorkspace, mtd
from mantid.simpleapi import LoadILL

import unittest

DIFF_PLACES = 12

class ILLIN5Tests(unittest.TestCase):
    
    wsData_name = "in5_ws_data"
    wsVana_name = "in5_ws_vana"
    dataDispersionFile = "ILL/ILLIN5_Sample_096003.nxs"
    vanadiumFile = "ILL/ILLIN5_Vana_095893.nxs"


    def tearDown(self):
        if self.wsData_name in mtd:
            mtd.remove(self.wsData_name)
        if self.wsVana_name in mtd:
            mtd.remove(self.wsVana_name)

    #================== Success cases ================================
    def test_load_single_file(self):
        self._run_load(self.dataDispersionFile)
        
        # Check some data
        wsOut = mtd[self.wsData_name]
        self.assertEqual(wsOut.getNumberHistograms(), 98305)
    
    def test_load_dispersion_file_and_vanadium_file(self):
        self._run_load(self.dataDispersionFile,self.vanadiumFile)
        
        # Check some data
        wsOut = mtd[self.wsData_name]
        self.assertEqual(wsOut.getNumberHistograms(), 98305)

    def test_load_dispersion_file_and_vanadium_workspace(self):

        self._run_load(self.vanadiumFile,outWSName=self.wsVana_name)
        # Check some data
        wsVana = mtd[self.wsVana_name]
        self.assertEqual(wsVana.getNumberHistograms(), 98305)


        self._run_load(self.dataDispersionFile,vanaFile=None,vanaWS=self.wsVana_name,outWSName=self.wsData_name)
        
        # Check some data
        wsData = mtd[self.wsData_name]
        self.assertEqual(wsData.getNumberHistograms(), 98305)
            
    #================== Failure cases ================================

    # TODO

    #================== Private methods ================================

    
    def _run_load(self, dataFile, vanaFile=None,vanaWS=None,outWSName=wsData_name):
        """
        ILL Loader
        """
        LoadILL(Filename=dataFile,FilenameVanadium=None,WorkspaceVanadium=None,OutputWorkspace=outWSName)
        self._do_ads_check(outWSName)

    def _do_ads_check(self, name):
        self.assertTrue(name in mtd)
        self.assertTrue(type(mtd[name]) == MatrixWorkspace)

#====================================================================================

class LoadILLIN5Test(stresstesting.MantidStressTest):

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest( unittest.makeSuite(ILLIN5Tests, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True 
        else:
            self._success = False

    def validate(self):
        return self._success
