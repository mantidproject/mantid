# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, Run
from mantid.simpleapi import SANSILLReduction, config, mtd
from mantid.geometry import Instrument


class SANSILLReduction2Test(unittest.TestCase):

    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D11/')
        config.appendDataSearchSubDir('ILL/D11B/')
        config.appendDataSearchSubDir('ILL/D33/')

    def setUp(self):
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

    def test_dark_current(self):
        SANSILLReduction(Runs='010462', ProcessAs='DarkCurrent', OutputWorkspace='dc', NormaliseBy='Monitor')
        self._check_output(mtd['dc'], 1, 128*128+2)
        self._check_process_flag(mtd['dc'], 'DarkCurrent')

    def test_empty_beam(self):
        SANSILLReduction(Runs='010414', ProcessAs='EmptyBeam', OutputWorkspace='eb', OutputFluxWorkspace='fl', NormaliseBy='Monitor')
        self._check_output(mtd['eb'], 1, 128*128+2)
        self._check_process_flag(mtd['eb'], 'EmptyBeam')
        run = mtd['eb'].getRun()
        self.assertAlmostEqual(run.getLogData('BeamCenterX').value, 0.0048, delta=1e-4)
        self.assertAlmostEqual(run.getLogData('BeamCenterY').value, -0.0027, delta=1e-4)
        self._check_output(mtd['fl'], 1, 1)
        self._check_process_flag(mtd['fl'], 'EmptyBeam')
        self.assertAlmostEqual(mtd['fl'].readY(0)[0], 738.538, delta=1e-3)
        self.assertAlmostEqual(mtd['fl'].readE(0)[0], 0.957, delta=1e-3)

    def test_transmission(self):
        SANSILLReduction(Runs='010414', ProcessAs='EmptyBeam', OutputWorkspace='eb', OutputFluxWorkspace='fl', NormaliseBy='Monitor')
        SANSILLReduction(Runs='010585', ProcessAs='Transmission', FluxWorkspace='fl', OutputWorkspace='tr', NormaliseBy='Monitor')
        self.assertAlmostEqual(mtd['tr'].readY(0)[0], 0.642, delta=1e-3)
        self.assertAlmostEqual(mtd['tr'].readE(0)[0], 0.0019, delta=1e-4)
        self._check_process_flag(mtd['tr'], 'Transmission')
        self._check_output(mtd['tr'], 1, 1)

    def test_container(self):
        SANSILLReduction(Runs='010460', ProcessAs='EmptyContainer', OutputWorkspace='can', NormaliseBy='Monitor')
        self._check_output(mtd['can'], 1, 128*128+2)
        self._check_process_flag(mtd['can'], 'EmptyContainer')

    def test_water(self):
        SANSILLReduction(Runs='010453', ProcessAs='Water', OutputSensitivityWorkspace='sens', OutputWorkspace='water', NormaliseBy='Monitor')
        self._check_output(mtd['water'], 1, 128*128+2)
        self._check_output(mtd['sens'], 1, 128*128+2)
        self._check_process_flag(mtd['water'], 'Water')
        self._check_process_flag(mtd['sens'], 'Water')

    def test_solvent(self):
        SANSILLReduction(Runs='010569', ProcessAs='Solvent', OutputWorkspace='solvent',)
        self._check_output(mtd['solvent'], 1, 128*128+2)
        self._check_process_flag(mtd['solvent'], 'Solvent')

    def test_sample(self):
        SANSILLReduction(Runs='010569', ProcessAs='Sample', OutputWorkspace='sample', NormaliseBy='Monitor')
        self._check_output(mtd['sample'], 1, 128*128+2)
        self._check_process_flag(mtd['sample'], 'Sample')

    def test_sample_kinetic(self):
        SANSILLReduction(Runs='017251', ProcessAs='Sample', OutputWorkspace='sample', NormaliseBy='Monitor')
        self._check_output(mtd['sample'], 85, 256*256+2)
        self._check_process_flag(mtd['sample'], 'Sample')

    def test_kinetic_calibrants_not_allowed(self):
        calibrants = ["EmptyBeam", "DarkCurrent", "Water", "EmptyContainer", "Solvent"]
        for process in calibrants:
            self.assertRaises(RuntimeError, SANSILLReduction, OutputWorkspace="out",
                              SampleRunsD1="017251", ProcessAs=process)

    def test_sample_tof(self):
        SANSILLReduction(Runs='042610', ProcessAs='Sample', OutputWorkspace='sample', NormaliseBy='Time')
        self._check_output_tof(mtd['sample'], 200, 256*256+2)
        self._check_process_flag(mtd['sample'], 'Sample')

    def _check_process_flag(self, ws, value):
        self.assertTrue(ws.getRun().getLogData('ProcessedAs').value, value)

    def _check_output(self, ws, blocksize, spectra):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(not ws.isHistogramData())
        self.assertTrue(not ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Empty")
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(isinstance(ws.getInstrument(), Instrument))
        self.assertTrue(isinstance(ws.getRun(), Run))
        self.assertTrue(ws.getHistory())

    def _check_output_tof(self, ws, blocksize, spectra):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertTrue(not ws.isDistribution())
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(isinstance(ws.getInstrument(), Instrument))
        self.assertTrue(isinstance(ws.getRun(), Run))
        self.assertTrue(ws.getHistory())

if __name__ == '__main__':
    unittest.main()
