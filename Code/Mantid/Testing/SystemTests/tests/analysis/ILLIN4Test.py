#pylint: disable=no-init
import stresstesting

from mantid.api import MatrixWorkspace, mtd
from mantid.simpleapi import LoadILL
from mantid.kernel import V3D

import unittest

DIFF_PLACES = 12

class ILLIN4Tests(unittest.TestCase):

    ws_name = "in4_ws"
    dataFile = "ILL/ILLIN4_074252.nxs"

    def tearDown(self):
        if self.ws_name in mtd:
            mtd.remove(self.ws_name)

    #================== Success cases ================================
    def test_load_file(self):
        self._run_load(self.dataFile)

        # Check some data
        wsOut = mtd[self.ws_name]
        self.assertEqual(wsOut.getNumberHistograms(), 397)

        # Check is the two detectors have the same theta
        samplePos = wsOut.getInstrument().getSample().getPos()
        beamDirection = V3D(0,0,1)
        det9 = wsOut.getDetector(9)
        det209 = wsOut.getDetector(209)
        self.assertEqual(det9.getTwoTheta(samplePos, beamDirection),
                         det209.getTwoTheta(samplePos, beamDirection))

        # Same mirror position
        self.assertEqual(det9.getPos().getX(),det209.getPos().getX())
        self.assertEqual(det9.getPos().getZ(),det209.getPos().getZ())
        self.assertEqual(det9.getPos().getY(),-det209.getPos().getY())

    #================== Failure cases ================================

    # TODO


    def _run_load(self, dataFile):
        """
        ILL Loader
        """
        LoadILL(Filename=dataFile,OutputWorkspace=self.ws_name)
        self._do_ads_check(self.ws_name)

    def _do_ads_check(self, name):
        self.assertTrue(name in mtd)
        self.assertTrue(type(mtd[name]) == MatrixWorkspace)

#====================================================================================

class LoadILLIN4Test(stresstesting.MantidStressTest):

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest( unittest.makeSuite(ILLIN4Tests, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success
