import unittest
import os

import mantid
from mantid.simpleapi import *


class LoadLiveDataTest(unittest.TestCase):
    """
    Test LoadLiveData when passing a python snippet to it.
    """

    def setUp(self):
        FrameworkManager.clearData()
        ConfigService.updateFacilities(os.path.join(ConfigService.getInstrumentDirectory(),"IDFs_for_UNIT_TESTING/UnitTestFacilities.xml"))
        ConfigService.setFacility("TEST")
        pass


    # --------------------------------------------------------------------------
    def doChunkTest(self, code):
        """ Test that whatever the code is, it rebins to 20 bins """
        LoadLiveData(Instrument='FakeEventDataListener', ProcessingScript=code,
                     OutputWorkspace='fake')

        ws = mtd['fake']
        # The rebin call in the code made 20 bins
        self.assertEquals( len(ws.readY(0)), 20 )
        # First bin is correct
        self.assertAlmostEqual(ws.readX(0)[0], 40e3, 3)


    # --------------------------------------------------------------------------
    def test_chunkProcessing(self):
        code = """Rebin(InputWorkspace=input,Params='40e3,1e3,60e3',OutputWorkspace=output)"""
        self.doChunkTest(code)

    # --------------------------------------------------------------------------
    def test_chunkProcessing_changing_outputVariable(self):
        code = """
Rebin(input,Params='40e3,1e3,60e3', OutputWorkspace='my_temp_name')
output = mtd['my_temp_name']
"""
        self.doChunkTest(code)

    # --------------------------------------------------------------------------
    def test_chunkProcessing_complexCode(self):
        code = """
import sys
def MyMethod(a, b):
    Rebin(a,Params='40e3,1e3,60e3', OutputWorkspace=b)

MyMethod(input, output)
"""
        self.doChunkTest(code)


    # --------------------------------------------------------------------------
    def test_PostProcessing(self):
        code = """Rebin(InputWorkspace=input,Params='40e3,1e3,60e3',OutputWorkspace=output)"""

        LoadLiveData(Instrument='FakeEventDataListener', PostProcessingScript=code,
                     AccumulationWorkspace='fake_accum', OutputWorkspace='fake')

        ws = mtd['fake']
        # The rebin call in the code made 20 bins
        self.assertEquals( len(ws.readY(0)), 20 )
        # First bin is correct
        self.assertAlmostEqual(ws.readX(0)[0], 40e3, 3)


if __name__ == '__main__':
    unittest.main()

