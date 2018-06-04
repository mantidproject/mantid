from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import SANSMask, SetInstrumentParameter, MaskDetectors, CreateSampleWorkspace, mtd


class SANSMaskTest(unittest.TestCase):

    def setUp(self):
        # create sample workspace
        ws1 = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        SetInstrumentParameter(Workspace=ws1, ParameterName='number-of-x-pixels', ParameterType='Number', Value='10.')
        SetInstrumentParameter(Workspace=ws1, ParameterName='number-of-y-pixels', ParameterType='Number', Value='10.')
        SetInstrumentParameter(Workspace=ws1, ParameterName='x-pixel-size', ParameterType='Number', Value='5.')
        SetInstrumentParameter(Workspace=ws1, ParameterName='y-pixel-size', ParameterType='Number', Value='5.')
        SetInstrumentParameter(Workspace=ws2, ParameterName='number-of-x-pixels', ParameterType='Number', Value='10.')
        SetInstrumentParameter(Workspace=ws2, ParameterName='number-of-y-pixels', ParameterType='Number', Value='10.')
        SetInstrumentParameter(Workspace=ws2, ParameterName='x-pixel-size', ParameterType='Number', Value='5.')
        SetInstrumentParameter(Workspace=ws2, ParameterName='y-pixel-size', ParameterType='Number', Value='5.')

    def tearDown(self):
        mtd.clear()

    def testMaskedWorkspace(self):
        MaskDetectors(Workspace='ws1', WorkspaceIndexList='7')
        SANSMask(Workspace='ws2', MaskedWorkspace='ws1')
        self.assertTrue(mtd['ws2'].spectrumInfo().isMasked(7))

if __name__ == "__main__":
    unittest.main()
