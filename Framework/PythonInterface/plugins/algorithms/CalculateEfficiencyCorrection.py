# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
import numpy as np

from mantid.simpleapi import \
    CloneWorkspace, ConvertFromDistribution, ConvertToPointData, ConvertUnits, SetSampleMaterial
from mantid.api import \
    mtd, AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty
from mantid.kernel import \
    Direction, FloatBoundedValidator, MaterialBuilder, StringMandatoryValidator

PROPS_FOR_TRANS = ['DensityType']
TABULATED_WAVELENGTH = 1.7982


class CalculateEfficiencyCorrection(DataProcessorAlgorithm):
    _input_ws = None
    _output_ws = None

    def category(self):
        return 'CorrectionFunctions\\EfficiencyCorrections'

    def name(self):
        return 'CalculateEfficiencyCorrection'

    def summary(self):
        return 'Calculate an efficiency correction using various inputs. Can be used to determine \
                an incident spectrum after correcting a measured spectrum from beam monitors \
                or vanadium measurements.'

    def seeAlso(self):
        return [ "He3TubeEfficiency", "DetectorEfficiencyCor", "DetectorEfficiencyCorUser",
                 "CalculateEfficiency", "ComputeCalibrationCoefVan" ]

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input),
            doc='Input workspace with wavelength range to calculate for the correction.')

        self.declareProperty(
            MatrixWorkspaceProperty('OutputWorkspace', '',
                                    direction=Direction.Output),
            doc="Outputs the efficiency correction which can be applied by multiplying \
                 the OutputWorkspace to the workspace that requires the correction.")

        self.declareProperty(
            name='Density',
            defaultValue=0.0,
            doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3), :math:`\\rho`. \
                 Default=0.0')

        self.declareProperty(
            name='Thickness',
            defaultValue=1.0,
            doc='Sample thickness (cm), :math:`T`. Default value=1.0')

        self.declareProperty(
            name='MeasuredEfficiency',
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0, 1.0),
            doc="Directly input the efficiency, :math:`\\epsilon`, measured at \
                 MeasuredEfficiencyWavelength, :math:`\\lambda_{\\epsilon}`, to determine \
                 :math:`\\rho * T = - ln(1-\\epsilon) \\frac{\\lambda_{ref}}{\\lambda_{\\epsilon} \\sigma_{a}(\\lambda_{ref})}` term, \
                 where :math:`\\lambda_{ref} =` 1.7982. Default=0.0")

        self.declareProperty(
            name='MeasuredEfficiencyWavelength',
            defaultValue=1.7982,
            validator=FloatBoundedValidator(0.0),
            doc="The wavelength, :math:`\\lambda_{\\epsilon}`, at which the \
                 MeasuredEfficiency, :math:`\\epsilon`, was measured. Default=1.7982")

        self.declareProperty(
            name='Alpha',
            defaultValue=0.0,
            doc="Directly input :math:`\\alpha`, where \
                 :math:`\\alpha = \\rho * T * \\sigma_{a}(\\lambda_{ref}) / \\lambda_{ref}`, \
                 where :math:`\\lambda_{ref} =` 1.7982. Default=0.0")

        self.declareProperty(
            name='ChemicalFormula', defaultValue='None',
            validator=StringMandatoryValidator(),
            doc='Sample chemical formula used to determine :math:`\\sigma_{a}(\\lambda_{ref}) term.`')

        self.copyProperties('CalculateSampleTransmission', PROPS_FOR_TRANS)

    def validateInputs(self):
        issues = dict()
        density = self.getProperty('Density').value
        if density < 0.0:
            issues['Density'] = 'Density must be positive'

        thickness = self.getProperty('Thickness').value
        if thickness < 0.0:
            issues['Thickness'] = 'Thickness must be positive'

        if not self.getProperty('Density').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with Density"

        if not self.getProperty('MeasuredEfficiency').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with MeasuredEfficiency"

        if not self.getProperty('MeasuredEfficiency').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with MeasuredEfficiency"

        if not self.getProperty('MeasuredEfficiency').isDefault and not self.getProperty('Density'):
            issues['MeasuredEfficiency'] = "Cannot select both MeasuredEfficiency and Density as input"
            issues['Density'] = "Cannot select both MeasuredEfficiency and Density as input"

        if not self.getProperty('Alpha').isDefault and not self.getProperty('Density'):
            issues['Alpha'] = "Cannot select both Alpha and Density as input"
            issues['Density'] = "Cannot select both Alpha and Density as input"

        if not self.getProperty('MeasuredEfficiency').isDefault and not self.getProperty('Alpha'):
            issues['MeasuredEfficiency'] = "Cannot select both MeasuredEfficiency and Alpha as input"
            issues['Alpha'] = "Cannot select both MeasuredEfficiency and Alpha as input"

        return issues

    def _setup(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._efficiency = self.getProperty('MeasuredEfficiency').value
        self._efficiency_wavelength = self.getProperty('MeasuredEfficiencyWavelength').value
        self._chemical_formula = self.getPropertyValue('ChemicalFormula')
        self._density_type = self.getPropertyValue('DensityType')
        self._density = self.getProperty('Density').value
        self._thickness = self.getProperty('Thickness').value
        self._alpha = self.getProperty('Alpha').value
        self._output_ws = self.getProperty('OutputWorkspace').valueAsStr

    def PyExec(self):
        self._setup()
        CloneWorkspace(
            Inputworkspace=self._input_ws,
            OutputWorkspace=self._output_ws)

        if mtd[self._output_ws].isDistribution():
            ConvertFromDistribution(Workspace=self._output_ws)

        ConvertToPointData(
            InputWorkspace=self._output_ws,
            OutputWorkspace=self._output_ws)
        ConvertUnits(
            InputWorkspace=self._output_ws,
            OutputWorkspace=self._output_ws,
            Target='Wavelength',
            EMode='Elastic')

        if self.getProperty('Alpha').isDefault:
            SetSampleMaterial(
                InputWorkspace=self._output_ws,
                ChemicalFormula=self._chemical_formula,
                SampleNumberDensity=self._density)
            if self.getProperty('MeasuredEfficiency').isDefault:
                self._calculate_area_density_from_density()
            else:
                self._calculate_area_density_from_efficiency()
            self._calculate_alpha()

        ws = mtd[self._output_ws]
        wavelengths = ws.readX(0)
        efficiency = self._calculate_efficiency(wavelengths)
        for histo in range(ws.getNumberHistograms()):
            mtd[self._output_ws].setY(histo, efficiency)

        self.setProperty('OutputWorkspace', mtd[self._output_ws])

    def _calculate_area_density_from_efficiency(self):
        '''
        Calculates area density (atom/cm^2) using efficiency

        '''
        material = mtd[self._output_ws].sample().getMaterial()
        ref_absXS = material.absorbXSection()
        self._area_density = - np.log( 1.0 - self._efficiency) / ref_absXS
        self._area_density *= TABULATED_WAVELENGTH / self._efficiency_wavelength

    def _calculate_area_density_from_density(self):
        '''
        Calculates area density (atom/cm^2) using number density and thickness.

        '''
        if self._density_type == 'Mass Density':
            builder = MaterialBuilder().setFormula(self._chemical_formula)
            mat = builder.setMassDensity(self._density).build()
            self._density = mat.numberDensity
        self._area_density = self._density * self._thickness

    def _calculate_alpha(self):
        '''
        Calculates alpha = -area_density / absXS / 1.7982
        '''

        material = mtd[self._output_ws].sample().getMaterial()
        absorption_x_section = material.absorbXSection()  / TABULATED_WAVELENGTH
        self._alpha = self._area_density * absorption_x_section

    def _calculate_efficiency(self, wavelength):
        '''
        Calculates efficiency of a detector / monitor at a given wavelength.

        @param wavelength Wavelength at which to calculate (in Angstroms)
        @return efficiency
        '''
        efficiency = 1. / (1.0 - np.exp(-self._alpha * wavelength))
        return efficiency


AlgorithmFactory.subscribe(CalculateEfficiencyCorrection)
