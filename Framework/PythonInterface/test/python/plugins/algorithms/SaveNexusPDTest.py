#pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import unittest
import numpy as np
import mantid
from mantid.simpleapi import CreateWorkspace, SaveNexusPD
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os, json

class SaveNexusPDTest(unittest.TestCase):
    def saveFilePath(self, wkspname):
        dataDir=mantid.config.getString('defaultsave.directory')
        return os.path.join(dataDir, wkspname+'.h5')

    def cleanup(self, wkspname, filename):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(wkspname):
            mantid.api.AnalysisDataService.remove(wkspname)

    def test_saveOneSpectrumNoInstrument(self):
        """ Test to Save one spectrum
        """
        wkspname = "SaveNexusPDTest_onespectrumnoinstr"
        filename = self.saveFilePath(wkspname)
        self._createOneSpectrum(wkspname, False)

        try:
            SaveNexusPD(InputWorkspace=wkspname, OutputFilename=filename)

            # verify
        finally:
            # cleanup
            self.cleanup(filename, wkspname)

    def _createOneSpectrum(self, wkspname, withInstrument=True):
        """ Create data workspace
        """
        x = np.arange(300, 16667, 15.)
        y = np.random.random(len(x))
        e = np.sqrt(y)

        CreateWorkspace(OutputWorkspace=wkspname,
                        DataX=x, DataY=y, DataE=e, NSpec=1,
                        UnitX='TOF',#Distribution='1',
                        YUnitlabel="S(q)")

if __name__ == '__main__':
    unittest.main()
