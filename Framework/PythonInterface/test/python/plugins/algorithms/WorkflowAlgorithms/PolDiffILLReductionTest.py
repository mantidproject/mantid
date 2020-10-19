# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup, Run
from mantid.simpleapi import config, mtd, PolDiffILLReduction
from mantid.geometry import Instrument

class PolDiffILLReductionTest(unittest.TestCase):

    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')

    def setUp(self):
        self._facility = config['default.facility']
        self._instrument = config['default.instrument']

        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D7'

    def tearDown(self):
        if self._facility:
            config['default.facility'] = self._facility
        if self._instrument:
            config['default.instrument'] = self._instrument
        mtd.clear()

    def test_absorber_transmission(self):
        PolDiffILLReduction(Run='396991', ProcessAs='Beam', OutputWorkspace='cadmium_ws')
        self._check_output(mtd['cadmium_ws'], 1, 1, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['cadmium_ws'], 'Absorber')
        self.assertAlmostEqual(mtd['cadmium_ws_1'].readY(0)[0], 116, delta=1)

    def test_beam(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        self._check_output(mtd['beam_ws'], 1, 1, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['beam_ws'], 'Beam')
        self.assertAlmostEqual(mtd['beam_ws_1'].readY(0)[0], 10769, delta=1)

    def test_transmission(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396991', ProcessAs='Beam', OutputWorkspace='cadmium_ws')
        PolDiffILLReduction(Run='396985', ProcessAs='Transmission', OutputWorkspace='quartz_transmission',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1', BeamInputWorkspace='beam_ws_1',)
        self._check_output(mtd['quartz_transmission'], 1, 1, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self.assertAlmostEqual(mtd['quartz_transmission_1'].readY(0)[0], 0.692, delta=1e-3)
        self._check_process_flag(mtd['quartz_transmission'], 'Transmission')

    def test_absorber(self):
        PolDiffILLReduction(Run='396928', ProcessAs='Absorber', OutputWorkspace='absorber_ws')
        self._check_output(mtd['absorber_ws'], 1, 132, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['absorber_ws'], 'Absorber')

    def test_container(self):
        PolDiffILLReduction(Run='396917', ProcessAs='Container', OutputWorkspace='container_ws')
        self._check_output(mtd['container_ws'], 1, 132, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['container_ws'], 'Container')

    def test_quartz(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396991', ProcessAs='Beam', OutputWorkspace='cadmium_ws')
        PolDiffILLReduction(Run='396985', ProcessAs='Transmission', OutputWorkspace='quartz_transmission',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1', BeamInputWorkspace='beam_ws_1',)
        PolDiffILLReduction(Run='396939', ProcessAs='Quartz', TransmissionInputWorkspace='quartz_transmission_1',
                            OutputTreatment='AverageScans', OutputWorkspace='quartz')
        self._check_output(mtd['quartz'], 1, 132, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['quartz'], 'Quartz')
    
    def test_vanadium(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396990', ProcessAs='Transmission', OutputWorkspace='vanadium_transmission',
                            BeamInputWorkspace='beam_ws_1')
        samplePropertiesDictionary = {'Mass': 8.54, 'Density': 6.0, 'FormulaUnits': 50, 'ChemicalFormula': 'V',
                                      'Thickness': 2, 'Height': 2, 'Width': 2, 'BeamWidth': 2.5, 'BeamHeight': 2.5,
                                      'NumberDensity': 1.18, 'ContainerFormula': 'Al', 'ContainerDensity': 2.7,
                                      'ContainerFrontThickness': 0.02, 'ContainerBackThickness': 0.02}
        PolDiffILLReduction(Run='396993', ProcessAs='Vanadium', OutputWorkspace='vanadium',
                            TransmissionInputWorkspace='vanadium_transmission',
                            SampleAndEnvironmentPropertiesDictionary=samplePropertiesDictionary,
                            OutputTreatment='OutputIndividualScans')
        self._check_output(mtd['vanadium_1'], 1, 132, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_1'], 'Vanadium')

    def test_vanadium_sum_scans(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396990', ProcessAs='Transmission', OutputWorkspace='vanadium_transmission',
                            BeamInputWorkspace='beam_ws_1')
        samplePropertiesDictionary = {'Mass': 8.54, 'Density': 6.0, 'FormulaUnits': 50, 'ChemicalFormula': 'V',
                                      'Thickness': 2, 'Height': 2, 'Width': 2, 'BeamWidth': 2.5, 'BeamHeight': 2.5,
                                      'NumberDensity': 1.18, 'ContainerFormula': 'Al', 'ContainerDensity': 2.7,
                                      'ContainerFrontThickness': 0.02, 'ContainerBackThickness': 0.02}
        PolDiffILLReduction(Run='396993', ProcessAs='Vanadium', OutputWorkspace='vanadium',
                            TransmissionInputWorkspace='vanadium_transmission',
                            SampleAndEnvironmentPropertiesDictionary=samplePropertiesDictionary,
                            OutputTreatment='SumScans', OutputUnits='TwoTheta')
        self._check_output(mtd['vanadium_1'], 1, 132, 'Wavelength', 'Wavelength', 'Scattering angle', 'Degrees')
        self._check_process_flag(mtd['vanadium_1'], 'Vanadium')

    def test_sample(self):
        PolDiffILLReduction(Run='396983', ProcessAs='Beam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396990', ProcessAs='Transmission', OutputWorkspace='vanadium_transmission',
                            BeamInputWorkspace='beam_ws_1')
        vanadiumPropertiesDictionary = {'Mass': 8.54, 'Density': 6.0, 'FormulaUnits': 50, 'ChemicalFormula': 'V',
                                      'Thickness': 2, 'Height': 2, 'Width': 2, 'BeamWidth': 2.5, 'BeamHeight': 2.5,
                                      'NumberDensity': 1.18, 'ContainerFormula': 'Al', 'ContainerDensity': 2.7,
                                      'ContainerFrontThickness': 0.02, 'ContainerBackThickness': 0.02}
        PolDiffILLReduction(Run='396993', ProcessAs='Vanadium', OutputWorkspace='vanadium',
                            TransmissionInputWorkspace='vanadium_transmission',
                            SampleAndEnvironmentPropertiesDictionary=vanadiumPropertiesDictionary,
                            OutputTreatment='AverageScans')

        samplePropertiesDictionary = {'Mass': 2.932, 'Density': 2.0, 'FormulaUnits': 182.54,
                                      'ChemicalFormula': 'Mn0.5-Fe0.5-P-S3', 'Thickness': 2, 'Height': 2, 'Width': 2,
                                      'BeamWidth': 2.5, 'BeamHeight': 2.5, 'NumberDensity': 1.18,
                                      'ContainerFormula': 'Al', 'ContainerDensity': 2.7,
                                      'ContainerFrontThickness': 0.02, 'ContainerBackThickness': 0.02}
        PolDiffILLReduction(Run='396986', ProcessAs='Transmission', OutputWorkspace='sample_transmission',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='397004', ProcessAs='Sample', OutputWorkspace='sample',
                            ComponentSeparationMethod='None',
                            TransmissionInputWorkspace='sample_transmission_1',
                            VanadiumInputWorkspace='vanadium_1',
                            SampleAndEnvironmentPropertiesDictionary=samplePropertiesDictionary,
                            DetectorEfficiencyCalibration='Vanadium',
                            OutputTreatment='OutputIndividualScans')
        self._check_output(mtd['sample'], 1, 132, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['sample'], 'Sample')
    
    def _check_process_flag(self, ws, value):
        self.assertTrue(ws[0].getRun().getLogData('ProcessedAs').value, value)

    def _check_output(self, ws, blocksize, spectra, x_unit, x_unit_id, y_unit, y_unit_id):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        for entry in ws:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(entry.isHistogramData())
            self.assertTrue(not entry.isDistribution())
            self.assertEqual(entry.getAxis(0).getUnit().caption(), x_unit)
            self.assertEqual(entry.getAxis(0).getUnit().unitID(), x_unit_id)
            self.assertEqual(entry.getAxis(1).getUnit().caption(), y_unit)
            self.assertEqual(entry.getAxis(1).getUnit().unitID(), y_unit_id)
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(isinstance(entry.getInstrument(), Instrument))
            self.assertTrue(isinstance(entry.getRun(), Run))
            self.assertTrue(entry.getHistory())

if __name__ == '__main__':
    unittest.main()
