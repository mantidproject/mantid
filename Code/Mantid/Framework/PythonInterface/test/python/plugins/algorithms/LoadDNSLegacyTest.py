from mantid.kernel import *
import mantid.simpleapi as api
import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from math import pi


class LoadDNSLegacyTest(unittest.TestCase):

    def test_LoadValidData(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test1"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename = filename, \
                OutputWorkspace = outputWorkspaceName)
                
        self.assertTrue(alg_test.isExecuted())
        
        #Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(31461, ws.readY(1))
        self.assertEqual(13340, ws.readY(23))
        # sample logs
        logs = ws.getRun().getLogData()
        self.assertEqual('deterota', logs[4].name)
        self.assertEqual(-8.54, logs[4].value)
        # check whether detector bank is rotated
        samplePos = ws.getInstrument().getSample().getPos()
        beamDirection = V3D(0,0,1)
        det = ws.getDetector(1)
        self.assertAlmostEqual(8.54, det.getTwoTheta(samplePos, beamDirection)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace = outputWorkspaceName)
        return

if __name__ == '__main__':
    unittest.main()
