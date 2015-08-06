import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from math import pi
import os
from mantid.simpleapi import LoadDNSLegacy


class LoadDNSLegacyTest(unittest.TestCase):

    def test_LoadValidData(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test1"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename,
                                 OutputWorkspace=outputWorkspaceName, Polarisation='y')
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(31461, ws.readY(1))
        self.assertEqual(13340, ws.readY(23))
        # sample logs
        run = ws.getRun()
        self.assertEqual(-8.54, run.getProperty('deterota').value)
        self.assertEqual(8332872, run.getProperty('mon_sum').value)
        self.assertEqual('y', run.getProperty('polarisation').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def _createIncompleteFile(self, filename):
        """
        creates an incomplete data file
        """
        with open(filename, "w") as f:
            f.write("# DNS Data userid=sa,exp=961,file=988,sample=run2")
            f.write("#--------------------------------------------------------------------------")
            f.write("# 9")
            f.write("# User: Some User")
            f.close()
        return

    def test_LoadInvalidData(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test2"
        filename = "dns-incomplete.d_dat"
        self._createIncompleteFile(filename)
        self.assertRaises(RuntimeError, LoadDNSLegacy,  Filename=filename,
                          OutputWorkspace=outputWorkspaceName, Polarisation='y')
        os.remove(filename)


if __name__ == '__main__':
    unittest.main()
