# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest,os
from mantid.simpleapi import CreateSampleWorkspace, CropWorkspaceForMDNorm

class CropWorkspaceForMDNormTest(unittest.TestCase):
    def test_simple(self):
        ws_in = CreateSampleWorkspace(WorkspaceType='Event',
                                      Function='Flat background',
                                      XUnit='Momentum',
                                      XMax=10,
                                      BinWidth=0.1)
        ws_out = CropWorkspaceForMDNorm(InputWorkspace=ws_in,
                                        XMin=1,
                                        XMax=6)
        self.assertEquals(ws_out.getNumberEvents(), ws_in.getNumberEvents()/2)                       
        self.assertLessEqual(ws_out.getSpectrum(1).getTofs().max(), 6.)
        self.assertGreaterEqual(ws_out.getSpectrum(1).getTofs().min(), 1.)
        self.assertTrue(ws_out.run().hasProperty('MDNorm_low'))
        self.assertTrue(ws_out.run().hasProperty('MDNorm_high'))
        self.assertTrue(ws_out.run().hasProperty('MDNorm_spectra_index'))
        self.assertEquals(ws_out.run().getProperty('MDNorm_low').value[0],1.)
        self.assertEquals(ws_out.run().getProperty('MDNorm_high').value[0],6.)
        self.assertEquals(ws_out.run().getProperty('MDNorm_spectra_index').value[-1],ws_out.getNumberHistograms()-1)

if __name__=="__main__":
    unittest.main()
