# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace
from mantid.simpleapi import SANSILLReduction, config, mtd


class SANSILLReductionTest(unittest.TestCase):

    _facility = None
    _instrument = None

    def setUp(self):
        config.appendDataSearchSubDir('ILL/D11/')
        config.appendDataSearchSubDir('ILL/D33/')

        self._facility = config['default.facility']
        self._instrument = config['default.instrument']

        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'

    def tearDown(self):
        if self._facility:
            config['default.facility'] = self._facility
        if self._instrument:
            config['default.instrument'] = self._instrument
        mtd.clear()

    def test_absorber(self):
        SANSILLReduction(Run='010462', ProcessAs='Absorber', OutputWorkspace='Cd')
        self._check_output(mtd['Cd'], True, 1, 128*128)
        self._check_process_flag(mtd['Cd'], 'Absorber')

    def test_beam(self):
        SANSILLReduction(Run='010414', ProcessAs='Beam', OutputWorkspace='Db', FluxOutputWorkspace='Fl')
        self._check_output(mtd['Db'], True, 1, 128*128)
        self._check_process_flag(mtd['Db'], 'Beam')
        run = mtd['Db'].getRun()
        self.assertAlmostEqual(run.getLogData('BeamCenterX').value, -0.0048, delta=1e-4)
        self.assertAlmostEqual(run.getLogData('BeamCenterY').value, -0.0027, delta=1e-4)
        self._check_output(mtd['Fl'], False, 1, 128*128)
        self._check_process_flag(mtd['Fl'], 'Beam')
        self.assertAlmostEqual(mtd['Fl'].readY(0)[0], 6628249, delta=1)
        self.assertAlmostEqual(mtd['Fl'].readE(0)[0], 8566, delta=1)

    def test_transmission(self):
        SANSILLReduction(Run='010414', ProcessAs='Beam', OutputWorkspace='Db')
        SANSILLReduction(Run='010585', ProcessAs='Transmission', BeamInputWorkspace='Db', OutputWorkspace='Tr')
        self.assertAlmostEqual(mtd['Tr'].readY(0)[0], 0.640, delta=1e-3)
        self.assertAlmostEqual(mtd['Tr'].readE(0)[0], 0.0019, delta=1e-4)
        self._check_process_flag(mtd['Tr'], 'Transmission')

    def test_container(self):
        SANSILLReduction(Run='010460', ProcessAs='Container', OutputWorkspace='can')
        self._check_output(mtd['can'], True, 1, 128*128)
        self._check_process_flag(mtd['can'], 'Container')

    def test_reference(self):
        SANSILLReduction(Run='010453', ProcessAs='Reference', SensitivityOutputWorkspace='sens', OutputWorkspace='water')
        self._check_output(mtd['water'], True, 1, 128*128)
        self._check_output(mtd['sens'], False, 1, 128*128)
        self._check_process_flag(mtd['water'], 'Reference')
        self._check_process_flag(mtd['sens'], 'Sensitivity')

    def test_sample(self):
        SANSILLReduction(Run='010569', ProcessAs='Sample', OutputWorkspace='sample')
        self._check_output(mtd['sample'], True, 1, 128*128)
        self._check_process_flag(mtd['sample'], 'Sample')

    def test_absorber_tof(self):
        # D33 VTOF
        # actually this is a container run, not an absorber, but is fine for this test
        SANSILLReduction(Run='093409', ProcessAs='Absorber', OutputWorkspace='absorber')
        self._check_output(mtd['absorber'], True, 30, 256*256)
        self._check_process_flag(mtd['absorber'], 'Absorber')

    def test_beam_tof(self):
        # D33 VTOF
        SANSILLReduction(Run='093406', ProcessAs='Beam', OutputWorkspace='beam', FluxOutputWorkspace='flux')
        self._check_output(mtd['beam'], True, 30, 256*256)
        self._check_process_flag(mtd['beam'], 'Beam')
        run = mtd['beam'].getRun()
        self.assertAlmostEqual(run.getLogData('BeamCenterX').value, -0.0025, delta=1e-4)
        self.assertAlmostEqual(run.getLogData('BeamCenterY').value, 0.0009, delta=1e-4)
        self._check_output(mtd['flux'], False, 30, 256*256)
        self._check_process_flag(mtd['flux'], 'Beam')

    def test_transmission_tof(self):
        # D33 VTOF
        SANSILLReduction(Run='093406', ProcessAs='Beam', OutputWorkspace='beam')
        SANSILLReduction(Run='093407', ProcessAs='Transmission', BeamInputWorkspace='beam', OutputWorkspace='ctr')
        self._check_output(mtd['ctr'], False, 75, 1)

    def test_container_tof(self):
        # D33 VTOF
        # this is actually a sample run, not water, but is fine for this test
        SANSILLReduction(Run='093410', ProcessAs='Reference', OutputWorkspace='ref')
        self._check_output(mtd['ref'], True, 30, 256*256)
        self._check_process_flag(mtd['ref'], 'Reference')

    def test_sample_tof(self):
        # D33 VTOF, Pluronic F127
        SANSILLReduction(Run='093410', ProcessAs='Sample', OutputWorkspace='sample')
        self._check_output(mtd['sample'], True, 30, 256*256)
        self._check_process_flag(mtd['sample'], 'Sample')

    def _check_process_flag(self, ws, value):
        self.assertTrue(ws.getRun().getLogData('ProcessedAs').value, value)

    def _check_output(self, ws, logs, blocksize, spectra):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertTrue(not ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getHistory())
        if logs:
            self.assertTrue(ws.getRun().hasProperty('qmin'))
            self.assertTrue(ws.getRun().hasProperty('qmax'))
            self.assertTrue(ws.getRun().hasProperty('l2'))
            self.assertTrue(ws.getRun().hasProperty('pixel_height'))
            self.assertTrue(ws.getRun().hasProperty('pixel_width'))
            self.assertTrue(ws.getRun().hasProperty('collimation.actual_position'))

if __name__ == '__main__':
    unittest.main()
