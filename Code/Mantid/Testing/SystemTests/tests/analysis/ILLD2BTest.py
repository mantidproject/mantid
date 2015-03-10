#pylint: disable=no-init
import stresstesting

from mantid.api import mtd, IMDEventWorkspace
from mantid.simpleapi import LoadILLAscii

import unittest

class ILLD2BLoadTest(unittest.TestCase):

    ws_name = "d2b_ws"
    prefix = "D2B"
    dataFile = "ILL/ILL_D2B_121459.txt"

    def tearDown(self):
        for wsName in mtd.getObjectNames():
            if wsName.startswith(self.prefix):
                mtd.remove(wsName)

    #================== Success cases ================================
    def test_load_single_file(self):
        self._run_load(self.dataFile)

        # Check some data
        wsOut = mtd[self.ws_name]
        self.assertEqual(wsOut.getNEvents(), 409600)



    def _run_load(self, dataFile):
        """
        ILL Loader
        """
        LoadILLAscii(Filename=dataFile,OutputWorkspace=self.ws_name)
        self._do_ads_check(self.ws_name)

    def _do_ads_check(self, name):
        self.assertTrue(name in mtd)
        self.assertTrue(type(mtd[name]) == IMDEventWorkspace)



#====================================================================================

class ILLD2BTest(stresstesting.MantidStressTest):

    def requiredMemoryMB(self):
        """Set a limit of 2.5Gb to avoid 32-bit environment"""
        return 2500

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest( unittest.makeSuite(ILLD2BLoadTest, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success
