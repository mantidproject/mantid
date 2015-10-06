import unittest
import numpy
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class SaveVulcanGSSTest(unittest.TestCase):

    def test_saveGSS(self):
        """ Test to Save a GSAS file to match V-drive
        """
        # Create a test data file and workspace
        binfilename = "testbin.dat"
        self._createBinFile(binfilename)

        datawsname = "TestInputWorkspace"
        self._createDataWorkspace(datawsname)

        # Execute
        alg_test = run_algorithm("SaveVulcanGSS", 
                InputWorkspace = datawsname,
                BinFilename = binfilename,
                OutputWorkspace = datawsname+"_rebinned",
                GSSFilename = "tempout.gda")

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve(datawsname+"_rebinned")
        #self.assertEqual(4, tablews.rowCount())

        # Delete the test hkl file
        os.remove(binfilename)
        AnalysisDataService.remove("InputWorkspace")
        AnalysisDataService.remove(datawsname+"_rebinned")

        return

    def _createBinFile(self, binfilename):
        """ Create a bin file
        """
        import math

        tof0 = 5000.
        delta = 0.001
        numpts = 100

        wbuf = ""
        tof = tof0
        for n in xrange(numpts):
            wbuf += "%.4f " % (math.log(tof)/math.log(10.))
            tof = tof * (1 + delta)
       
        ofile = open(binfilename, "w")
        ofile.write(wbuf)
        ofile.close()

        return

    def _createDataWorkspace(self, datawsname):
        """ Create data workspace
        """
        import math

        tof0 = 4900.
        delta = 0.001
        numpts = 200

        vecx = []
        vecy = []
        vece = []

        tof = tof0
        for n in xrange(numpts):
            vecx.append(tof)
            vecy.append(math.sin(tof0))
            vece.append(1.)

            tof = tof * (1+delta)
        # ENDFOR
        vecx.append(tof)

        dataws = api.CreateWorkspace(DataX = vecx, DataY = vecy, DataE = vece, NSpec = 1, 
                UnitX = "TOF")

        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)

        return dataws


if __name__ == '__main__':
    unittest.main()
