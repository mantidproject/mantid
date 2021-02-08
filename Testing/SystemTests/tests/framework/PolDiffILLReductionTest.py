# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import MatrixWorkspace, WorkspaceGroup, Run
from mantid.simpleapi import CloneWorkspace, config, D7AbsoluteCrossSections, Load, mtd, PolDiffILLReduction
from mantid.geometry import Instrument


class PolDiffILLReductionTest(systemtesting.MantidSystemTest):

    _pixels_per_bank = 44
    _sampleProperties = None

    def __init__(self):
        super(PolDiffILLReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D7'
        config.appendDataSearchSubDir('ILL/D7/')
        Load('numerical_attenuation.nxs', OutputWorkspace='numerical_attenuation_ws')

        self._sampleProperties = {'FormulaUnits': 1, 'SampleChemicalFormula': 'V', 'SampleMass': 8.54,
                                  'FormulaUnitMass': 50.94,'SampleInnerRadius': 2, 'SampleOuterRadius': 2.5,
                                  'SampleRadius':2.5, 'Height': 2,'SampleThickness':0.5, 'SampleDensity': 0.1,
                                  'SampleAngle':0, 'SampleWidth':2.5, 'BeamWidth': 3.0, 'BeamHeight': 3.0,
                                  'ContainerRadius': 2.7, 'ContainerInnerRadius':1.99, 'ContainerOuterRadius':2.7,
                                  'ContainerFrontThickness':0.2, 'ContainerBackThickness':0.2,
                                  'ContainerChemicalFormula': 'Al', 'ContainerDensity': 0.01,
                                  'EventsPerPoint':100, 'ElementSize':1.0, 'IncoherentCrossSection':0.1,
                                  'SampleSpin':1.5}

    def cleanup(self):
        mtd.clear()

    def d7_reduction_test_vanadium_individual(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_individual',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_individual'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_individual'], 'Vanadium')

    def d7_reduction_test_vanadium_sum(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_sum',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Sum')
        self._check_output(mtd['vanadium_sum'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_sum'], 'Vanadium')

    def d7_reduction_test_vanadium_full_reduction(self):
        PolDiffILLReduction(Run='396983', ProcessAs='EmptyBeam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396985', ProcessAs='Transmission', OutputWorkspace='quartz_transmission',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='396917,396918', ProcessAs='Container', OutputWorkspace='container_ws')
        PolDiffILLReduction(Run='396928,396929', ProcessAs='Absorber', OutputWorkspace='absorber_ws')
        PolDiffILLReduction(Run='396991', ProcessAs='BeamWithAbsorber', OutputWorkspace='cadmium_ws')
        PolDiffILLReduction(Run='396939,397000', ProcessAs='Quartz', OutputWorkspace='pol_corrections',
                            TransmissionInputWorkspace='quartz_transmission_1',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1', OutputTreatment='Average')
        PolDiffILLReduction(Run='396990', ProcessAs='Transmission', OutputWorkspace='vanadium_transmission',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_full',
                            AbsorberInputWorkspace='absorber_ws',
                            ContainerInputWorkspace='container_ws',
                            TransmissionInputWorkspace='vanadium_transmission_1',
                            QuartzInputWorkspace='pol_corrections',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Sum')
        self._check_output(mtd['vanadium_full'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_full'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_flat_plate_numerical(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_num_flat_plate',
                            SelfAttenuationMethod='Numerical', SampleGeometry="FlatPlate",
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_num_flat_plate'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_num_flat_plate'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_flat_plate_monte_carlo(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_mc_flat_plate',
                            SelfAttenuationMethod='MonteCarlo', SampleGeometry="FlatPlate",
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_mc_flat_plate'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_mc_flat_plate'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_cylinder_numerical(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_num_cylinder',
                            SelfAttenuationMethod='Numerical', SampleGeometry="Cylinder",
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_num_cylinder'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_num_cylinder'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_cylinder_monte_carlo(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_mc_cylinder',
                            SelfAttenuationMethod='MonteCarlo', SampleGeometry="Cylinder",
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_mc_cylinder'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_mc_cylinder'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_annulus_numerical(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_num_annulus',
                            SelfAttenuationMethod='Numerical', SampleGeometry="Annulus",
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_num_annulus'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_num_annulus'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_annulus_monte_carlo(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_mc_annulus',
                            SelfAttenuationMethod='MonteCarlo', SampleGeometry="Annulus",
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_mc_annulus'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_mc_annulus'], 'Vanadium')

    def d7_reduction_test_vanadium_individual_user(self):
        CloneWorkspace(InputWorkspace='numerical_attenuation_ws', OutputWorkspace='numerical_attenuation_ws_clone')
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_ind_cylinder',
                            SelfAttenuationMethod='User', SampleGeometry="Custom",
                            SampleSelfAttenuationFactors='numerical_attenuation_ws_clone',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['vanadium_ind_cylinder'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_ind_cylinder'], 'Vanadium')

    def d7_reduction_test_vanadium_sum_flat_plate(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_sum_num_flat_plate',
                            SelfAttenuationMethod='Numerical', SampleGeometry="FlatPlate",
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Sum')
        self._check_output(mtd['vanadium_sum_num_flat_plate'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_sum_num_flat_plate'], 'Vanadium')

    def d7_reduction_test_vanadium_sum_cylinder(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_sum_num_cylinder',
                            SelfAttenuationMethod='Numerical', SampleGeometry="Cylinder",
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Sum')
        self._check_output(mtd['vanadium_sum_num_cylinder'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_sum_num_cylinder'], 'Vanadium')

    def d7_reduction_test_vanadium_sum_annulus(self):
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_sum_mc_annulus',
                            SelfAttenuationMethod='MonteCarlo', SampleGeometry="Annulus",
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Sum')
        self._check_output(mtd['vanadium_sum_mc_annulus'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_sum_mc_annulus'], 'Vanadium')

    def d7_reduction_test_vanadium_sum_user(self):
        CloneWorkspace(InputWorkspace='numerical_attenuation_ws', OutputWorkspace='numerical_attenuation_ws_clone')
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_sum_user',
                            SelfAttenuationMethod='User', SampleGeometry="Custom",
                            SampleSelfAttenuationFactors='numerical_attenuation_ws_clone',
                            SampleAndEnvironmentProperties=self._sampleProperties, OutputTreatment='Sum')
        self._check_output(mtd['vanadium_sum_user'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['vanadium_sum_user'], 'Vanadium')

    def d7_reduction_test_sample_individual(self):
        PolDiffILLReduction(Run='397004,397005', ProcessAs='Sample', OutputWorkspace='sample_individual',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['sample_individual'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['sample_individual'], 'Sample')
        D7AbsoluteCrossSections(InputWorkspace='sample_individual', OutputWorkspace='sample_individual_not_normalised',
                                CrossSectionSeparationMethod='XYZ',
                                NormalisationMethod='None',
                                OutputTreatment='Sum',
                                OutputUnits='Q',
                                SampleAndEnvironmentProperties=self._sampleProperties)
        self._check_output(mtd['sample_individual_not_normalised'], 263, 1, 6, 'q', 'MomentumTransfer', 'Height',
                           'Label', post_processed=True)

    def d7_reduction_test_sample_individual_incoherent(self):
        PolDiffILLReduction(Run='397004,397005', ProcessAs='Sample', OutputWorkspace='sample_individual',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['sample_individual'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['sample_individual'], 'Sample')
        D7AbsoluteCrossSections(InputWorkspace='sample_individual', OutputWorkspace='sample_individual_incoherent',
                                CrossSectionSeparationMethod='XYZ',
                                NormalisationMethod='Incoherent',
                                OutputTreatment='Sum',
                                OutputUnits='TwoTheta',
                                SampleAndEnvironmentProperties=self._sampleProperties)
        self._check_output(mtd['sample_individual_incoherent'], 263, 1, 6, 'Scattering Angle', 'Label', 'Height',
                           'Label', post_processed=True)

    def d7_reduction_test_sample_individual_paramagnetic(self):
        PolDiffILLReduction(Run='397004,397005', ProcessAs='Sample', OutputWorkspace='sample_individual',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['sample_individual'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['sample_individual'], 'Sample')
        D7AbsoluteCrossSections(InputWorkspace='sample_individual', OutputWorkspace='sample_individual_paramagnetic',
                                CrossSectionSeparationMethod='XYZ',
                                NormalisationMethod='Paramagnetic',
                                OutputTreatment='Individual',
                                OutputUnits='Q',
                                SampleAndEnvironmentProperties=self._sampleProperties)
        self._check_output(mtd['sample_individual_paramagnetic'], 132, 1, 6, 'q', 'MomentumTransfer', 'Wavelength',
                           'Wavelength', post_processed=True, normalised_individually=True)

    def d7_reduction_test_sample_sum(self):
        PolDiffILLReduction(Run='397004,397005', ProcessAs='Sample', OutputWorkspace='sample_sum',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            ScatteringAngleBinSize = 1.0,
                            OutputTreatment='Sum')
        self._check_output(mtd['sample_sum'], 132, 1, 6, 'Scattering Angle', 'Label', 'Height', 'Label')
        self._check_process_flag(mtd['sample_sum'], 'Sample')

    def d7_reduction_test_sample_full_reduction(self):
        PolDiffILLReduction(Run='396983', ProcessAs='EmptyBeam', OutputWorkspace='beam_ws')
        PolDiffILLReduction(Run='396985', ProcessAs='Transmission', OutputWorkspace='quartz_transmission',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='396917,396918', ProcessAs='Container', OutputWorkspace='container_ws')
        PolDiffILLReduction(Run='396928,396929', ProcessAs='Absorber', OutputWorkspace='absorber_ws')
        PolDiffILLReduction(Run='396991', ProcessAs='BeamWithAbsorber', OutputWorkspace='cadmium_ws')
        PolDiffILLReduction(Run='396939,397000', ProcessAs='Quartz', OutputWorkspace='pol_corrections',
                            TransmissionInputWorkspace='quartz_transmission_1',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1', OutputTreatment='Average')
        PolDiffILLReduction(Run='396990', ProcessAs='Transmission', OutputWorkspace='vanadium_transmission',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='396993,396994', ProcessAs='Vanadium', OutputWorkspace='vanadium_full',
                            AbsorberInputWorkspace='absorber_ws',
                            ContainerInputWorkspace='container_ws',
                            TransmissionInputWorkspace='vanadium_transmission_1',
                            QuartzInputWorkspace='pol_corrections',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Sum')
        PolDiffILLReduction(Run='396986,396987', ProcessAs='Transmission', OutputWorkspace='sample_transmission',
                            AbsorberTransmissionInputWorkspace='cadmium_ws_1',
                            BeamInputWorkspace='beam_ws_1')
        PolDiffILLReduction(Run='397004,397005', ProcessAs='Sample', OutputWorkspace='sample_full',
                            AbsorberInputWorkspace='absorber_ws',
                            ContainerInputWorkspace='container_ws',
                            TransmissionInputWorkspace='vanadium_transmission_1',
                            QuartzInputWorkspace='pol_corrections',
                            SampleAndEnvironmentProperties=self._sampleProperties,
                            OutputTreatment='Individual')
        self._check_output(mtd['sample_full'], 1, 132, 6, 'Wavelength', 'Wavelength', 'Spectrum', 'Label')
        self._check_process_flag(mtd['sample_full'], 'Sample')

        D7AbsoluteCrossSections(InputWorkspace='sample_full', OutputWorkspace='sample_full_normalised',
                                CrossSectionSeparationMethod='XYZ',
                                VanadiumInputWorkspace='vanadium_full',
                                NormalisationMethod='Vanadium',
                                OutputTreatment='Sum',
                                OutputUnits='Q',
                                SampleAndEnvironmentProperties=self._sampleProperties,
                                AbsoluteUnitsNormalisation=True)
        self._check_output(mtd['sample_full_normalised'], 263, 1, 6, 'q', 'MomentumTransfer', 'Height', 'Label',
                           post_processed=True)

    def _check_process_flag(self, ws, value):
        self.assertTrue(ws[0].getRun().getLogData('ProcessedAs').value, value)

    def _check_output(self, ws, blocksize, spectra, nEntries, x_unit, x_unit_id, y_unit, y_unit_id, post_processed=False,
                      normalised_individually=False):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertTrue(ws.getNumberOfEntries(), nEntries)
        for entry in ws:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            if post_processed:
                self.assertTrue(entry.isDistribution())
            else:
                self.assertTrue(not entry.isDistribution())
            if normalised_individually:
                self.assertTrue(not entry.isHistogramData())
            else:
                self.assertTrue(entry.isHistogramData())
            self.assertEqual(entry.getAxis(0).getUnit().caption(), x_unit)
            self.assertEqual(entry.getAxis(0).getUnit().unitID(), x_unit_id)
            self.assertEqual(entry.getAxis(1).getUnit().caption(), y_unit)
            self.assertEqual(entry.getAxis(1).getUnit().unitID(), y_unit_id)
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(isinstance(entry.getInstrument(), Instrument))
            self.assertTrue(isinstance(entry.getRun(), Run))
            self.assertTrue(entry.getHistory())

    def runTest(self):
        self.d7_reduction_test_vanadium_individual()
        self.d7_reduction_test_vanadium_sum()
        self.d7_reduction_test_vanadium_full_reduction()
        self.d7_reduction_test_vanadium_individual_flat_plate_numerical()
        self.d7_reduction_test_vanadium_individual_flat_plate_monte_carlo()
        self.d7_reduction_test_vanadium_individual_cylinder_numerical()
        self.d7_reduction_test_vanadium_individual_cylinder_monte_carlo()
        self.d7_reduction_test_vanadium_individual_annulus_numerical()
        self.d7_reduction_test_vanadium_individual_annulus_monte_carlo()
        self.d7_reduction_test_vanadium_individual_user()
        self.d7_reduction_test_vanadium_sum_flat_plate()
        self.d7_reduction_test_vanadium_sum_cylinder()
        self.d7_reduction_test_vanadium_sum_annulus()
        self.d7_reduction_test_vanadium_sum_user()
        self.d7_reduction_test_sample_individual()
        self.d7_reduction_test_sample_individual_incoherent()
        self.d7_reduction_test_sample_individual_paramagnetic()
        self.d7_reduction_test_sample_sum()
        self.d7_reduction_test_sample_full_reduction()
