from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace
from mantid.simpleapi import ILLSANSReduction, config, mtd


class ILLSANSReductionTest(unittest.TestCase):

    _facility = None

    def setUp(self):
        self._facility = config['default.facility']
        config['default.facility'] = 'ILL'

    def tearDown(self):
        config['default.facility'] = self._facility
        mtd.clear()

    def test_absorber(self):
        ILLSANSReduction(Run='010462', ProcessAs='Absorber', OutputWorkspace='Cd')
        self._check_output(mtd['Cd'])

    def test_beam(self):
        beam = ILLSANSReduction(Run='010414', ProcessAs='Beam', OutputWorkspace='Db', ReturnAll=True)
        self._check_output(mtd['Db'])
        self.assertAlmostEqual(beam.BeamCenterX, -0.0048, delta=1e-4)
        self.assertAlmostEqual(beam.BeamCenterY, -0.0027, delta=1e-4)
        self.assertAlmostEqual(beam.BeamFluxValue, 6618861, delta=1)
        self.assertAlmostEqual(beam.BeamFluxError, 8554, delta=1)

    def test_transmission(self):
        beam = ILLSANSReduction(Run='010414', ProcessAs='Beam', OutputWorkspace='Db', ReturnAll=True)
        transmission = ILLSANSReduction(Run='010585', ProcessAs='Transmission', BeamInputWorkspace='Db', ReturnAll=True)
        self.assertAlmostEqual(transmission.TransmissionValue, 0.640, delta=1e-3)
        self.assertAlmostEqual(transmission.TransmissionError, 0.0019, delta=1e-4)

    def test_container(self):
        container = ILLSANSReduction(Run='010460', ProcessAs='Container', OutputWorkspace='can')
        self._check_output(mtd['can'])

    def test_reference(self):
        ILLSANSReduction(Run='010453', ProcessAs='Reference', SensitivityOutputWorkspace='sens', OutputWorkspace='water')
        self._check_output(mtd['water'])
        self._check_output(mtd['sens'], logs=False)

    def test_sample(self):
        sample = ILLSANSReduction(Run='010569', ProcessAs='Sample')
        self._check_output(mtd['sample'])

    def _check_output(self, ws, logs=True):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertEqual(ws.blocksize(), 1)
        self.assertEqual(ws.getNumberHistograms(), 128 * 128 + 2)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getSampleDetails())
        self.assertTrue(ws.getHistory())
        if logs:
            self.assertTrue(ws.getRun().hasProperty('qmin'))
            self.assertTrue(ws.getRun().hasProperty('qmax'))
            self.assertTrue(ws.getRun().hasProperty('l2'))
            self.assertTrue(ws.getRun().hasProperty('pixel_height'))
            self.assertTrue(ws.getRun().hasProperty('pixel_width'))

if __name__ == '__main__':
    unittest.main()
