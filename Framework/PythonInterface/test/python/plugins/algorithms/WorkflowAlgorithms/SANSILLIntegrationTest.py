# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import SANSILLIntegration, SANSILLReduction, config, mtd


class SANSILLIntegrationTest(unittest.TestCase):

    _facility = None

    def setUp(self):
        self._facility = config['default.facility']
        config.appendDataSearchSubDir('ILL/D11/')
        config.appendDataSearchSubDir('ILL/D33/')
        config['default.facility'] = 'ILL'
        SANSILLReduction(Run='010569', ProcessAs='Sample', OutputWorkspace='sample')

    def tearDown(self):
        config['default.facility'] = self._facility
        mtd.clear()

    def test_monochromatic(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq')
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 88)
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_monochromatic_with_wedges(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', NumberOfWedges=2, WedgeWorkspace='wedges')
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 88)
        self.assertTrue(mtd['iq'].hasDx(0))
        self.assertTrue(mtd['wedges'])
        self.assertTrue(isinstance(mtd['wedges'], WorkspaceGroup))
        self.assertEquals(mtd['wedges'].getNumberOfEntries(), 2)
        for wedge in range(2):
            self._check_output(mtd['wedges'].getItem(wedge))
            self.assertEquals(mtd['wedges'].getItem(wedge).blocksize(), 88)
            self.assertTrue(mtd['iq'].hasDx(0))

    def test_monochromatic_cake(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputType='I(Phi,Q)', NumberOfWedges=36)
        self._check_output(mtd['iq'], 36)
        self.assertEquals(mtd['iq'].blocksize(), 88)
        azimuth_axis = mtd['iq'].getAxis(1)
        self.assertTrue(azimuth_axis.isNumeric())
        self.assertEquals(len(azimuth_axis),36)
        self.assertEqual(azimuth_axis.getUnit().unitID(), "Degrees")
        for phi in range(36):
            self.assertTrue(mtd['iq'].hasDx(phi))

    def test_monochromatic_2D(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputType='I(Qx,Qy)', MaxQxy=0.03, DeltaQ=0.001)
        self._check_output(mtd['iq'], 60)
        self.assertEquals(mtd['iq'].blocksize(), 60)
        qy_axis = mtd['iq'].getAxis(1)
        self.assertTrue(qy_axis.isNumeric())
        self.assertEquals(len(qy_axis),61)
        self.assertEqual(qy_axis.getUnit().unitID(), "MomentumTransfer")

    def test_with_bin_width(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputBinning=-0.1)
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 51)
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_with_bin_range(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputBinning=[0.001,0.03])
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 73)
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_with_bin_width_and_range(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputBinning=[0.001,-0.1,0.03])
        self._check_output(mtd['iq'])
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_custom_binning(self):
        binning = [0.001,0.005,0.006,0.01,0.016]
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', OutputBinning=binning)
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 2)
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_resolution_binning(self):
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', DefaultQBinning='ResolutionBased')
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 37)
        self.assertTrue(mtd['iq'].hasDx(0))

    def test_tof(self):
        # D33 VTOF
        SANSILLReduction(Run='093410', ProcessAs='Sample', OutputWorkspace='sample')
        # TOF resolution is not yet implemented
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', CalculateResolution='None')
        self._check_output(mtd['iq'])
        self.assertEquals(mtd['iq'].blocksize(), 217)

    def _check_output(self, ws, spectra = 1):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertTrue(ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getHistory())

if __name__ == '__main__':
    unittest.main()
