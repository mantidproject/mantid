# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
import numpy as np

from mantid.simpleapi import \
    CloneWorkspace, ConvertFromDistribution, ConvertToPointData, \
    ConvertUnits, CreateWorkspace, Rebin, SetSampleMaterial
from mantid.api import \
    AlgorithmFactory, CommonBinsValidator, PropertyMode, PythonAlgorithm, \
    MatrixWorkspaceProperty
from mantid.kernel import \
    Direction, FloatBoundedValidator, MaterialBuilder, \
    StringListValidator, StringMandatoryValidator

TABULATED_WAVELENGTH = 1.7982


class CalculateEfficiencyCorrection(PythonAlgorithm):
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
        return [ "He3TubeEfficiency", "CalculateSampleTransmission",
                 "DetectorEfficiencyCor", "DetectorEfficiencyCorUser",
                 "CalculateEfficiency", "ComputeCalibrationCoefVan" ]

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input,
                                    optional=PropertyMode.Optional,
                                    validator=CommonBinsValidator()),
            doc='Input workspace with wavelength range to calculate for the correction.')

        self.declareProperty(name='WavelengthRange', defaultValue='',
                             doc='Wavelength range to calculate efficiency for.')

        self.declareProperty(
            MatrixWorkspaceProperty('OutputWorkspace', '',
                                    direction=Direction.Output),
            doc="Outputs the efficiency correction which can be applied by multiplying \
                 the OutputWorkspace to the workspace that requires the correction.")

        self.declareProperty(
            name='ChemicalFormula', defaultValue='None',
            validator=StringMandatoryValidator(),
            doc='Sample chemical formula used to determine :math:`\\sigma (\\lambda_{ref})` term.')

        self.declareProperty(
            name='DensityType', defaultValue = 'Mass Density',
            validator=StringListValidator(['Mass Density', 'Number Density']),
            doc = 'Use of Mass density or Number density')

        self.declareProperty(
            name='Density',
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3), :math:`\\rho`. \
                 Default=0.0')

        self.declareProperty(
            name='Thickness',
            defaultValue=1.0,
            validator=FloatBoundedValidator(0.0),
            doc='Sample thickness (cm), :math:`T`. Default value=1.0')

        self.declareProperty(
            name='MeasuredEfficiency',
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0, 1.0),
            doc="Directly input the efficiency, :math:`\\epsilon`, measured at \
                 MeasuredEfficiencyWavelength, :math:`\\lambda_{\\epsilon}`, to determine :math:`\\rho * T`, where \
                 :math:`\\rho * T = - ln(1-\\epsilon) \\frac{1}{ \\frac{\\lambda_{\\epsilon} \\sigma (\\lambda_{ref})}{\\lambda_{ref}}}` \
                 term for XSectionType == AttenuationXSection and \
                 :math:`\\rho * T = - ln(1-\\epsilon) \\frac{1} \
                    { \\sigma_s + \\frac{\\lambda_{\\epsilon} \\sigma (\\lambda_{ref})}{\\lambda_{ref}}}` \
                 for XSectionType == TotalXSection \
                 with :math:`\\lambda_{ref} =` 1.7982. Default=0.0")

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
                 :math:`\\alpha = \\rho * T * \\sigma (\\lambda_{ref}) / \\lambda_{ref}`, \
                 where :math:`\\lambda_{ref} =` 1.7982. XSectionType has no effect. Default=0.0")

        self.declareProperty(
            name='XSectionType',
            defaultValue="AttenuationXSection",
            validator=StringListValidator(['AttenuationXSection', 'TotalXSection']),
            doc = 'Use either the absorption cross section (for monitor-type correction) or, \
                   the total cross section (for transmission-type correction) in Alpha term. \
                   Default=AttenuationXSection')

    def validateInputs(self):
        issues = dict()

        if self.getProperty('InputWorkspace').isDefault and self.getProperty('WavelengthRange').isDefault:
            issues['InputWorkspace'] = "Must select either InputWorkspace and WavelengthRange as input"
            issues['WavelengthRange'] = "Must select either InputWorkspace and WavelengthRange as input"

        if not self.getProperty('InputWorkspace').isDefault and not self.getProperty('WavelengthRange').isDefault:
            issues['InputWorkspace'] = "Cannot select both InputWorkspace and WavelengthRange as input"
            issues['WavelengthRange'] = "Cannot select both InputWorkspace and WavelengthRange as input"

        if not self.getProperty('Density').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with Density"

        if not self.getProperty('MeasuredEfficiency').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with MeasuredEfficiency"

        if not self.getProperty('MeasuredEfficiency').isDefault:
            if self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = "Must specify the ChemicalFormula with MeasuredEfficiency"

        if not self.getProperty('MeasuredEfficiency').isDefault and not self.getProperty('Density').isDefault:
            issues['MeasuredEfficiency'] = "Cannot select both MeasuredEfficiency and Density as input"
            issues['Density'] = "Cannot select both MeasuredEfficiency and Density as input"

        if not self.getProperty('Alpha').isDefault and not self.getProperty('Density').isDefault:
            issues['Alpha'] = "Cannot select both Alpha and Density as input"
            issues['Density'] = "Cannot select both Alpha and Density as input"

        if not self.getProperty('MeasuredEfficiency').isDefault and not self.getProperty('Alpha').isDefault:
            issues['MeasuredEfficiency'] = "Cannot select both MeasuredEfficiency and Alpha as input"
            issues['Alpha'] = "Cannot select both MeasuredEfficiency and Alpha as input"

        return issues

    def _setup(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._bin_params = self.getPropertyValue('WavelengthRange')
        self._output_ws = self.getProperty('OutputWorkspace').valueAsStr
        self._chemical_formula = self.getPropertyValue('ChemicalFormula')
        self._density_type = self.getPropertyValue('DensityType')
        self._density = self.getProperty('Density').value
        self._thickness = self.getProperty('Thickness').value
        self._efficiency = self.getProperty('MeasuredEfficiency').value
        self._efficiency_wavelength = self.getProperty('MeasuredEfficiencyWavelength').value
        self._alpha_absXS = self.getProperty('Alpha').value
        self._alpha_scatXS = 0.0
        self._xsection_type = self.getProperty('XSectionType').value

    def PyExec(self):
        self._setup()
        if not self.getProperty("InputWorkspace").isDefault:
            self._output_ws = CloneWorkspace(Inputworkspace=self._input_ws,
                                             OutputWorkspace=self._output_ws,
                                             StoreInADS=False)
        else:
            self._output_ws = CreateWorkspace(NSpec=1, DataX=[0], DataY=[0],
                                              UnitX='Wavelength', Distribution=False,
                                              StoreInADS=False)
            self._output_ws = Rebin(InputWorkspace=self._output_ws,
                                    Params=self._bin_params,
                                    StoreInADS=False)

        if self._output_ws.isDistribution():
            ConvertFromDistribution(Workspace=self._output_ws,
                                    StoreInADS=False)

        self._output_ws = ConvertToPointData(InputWorkspace=self._output_ws,
                                             StoreInADS=False)
        self._output_ws = ConvertUnits(InputWorkspace=self._output_ws,
                                       Target='Wavelength',
                                       EMode='Elastic',
                                       StoreInADS=False)

        if self.getProperty('Alpha').isDefault:
            SetSampleMaterial(
                InputWorkspace=self._output_ws,
                ChemicalFormula=self._chemical_formula,
                SampleNumberDensity=self._density,
                StoreInADS=False)
            if self.getProperty('MeasuredEfficiency').isDefault:
                self._calculate_area_density_from_density()
            else:
                self._calculate_area_density_from_efficiency()
            self._calculate_alpha_absXS_term()
            if self._xsection_type == "TotalXSection":
                self._calculate_alpha_scatXS_term()

        wavelengths = self._output_ws.readX(0)
        efficiency = self._calculate_efficiency(wavelengths)
        for histo in range(self._output_ws.getNumberHistograms()):
            self._output_ws.setY(histo, efficiency)

        self.setProperty('OutputWorkspace', self._output_ws)

    def _calculate_area_density_from_efficiency(self):
        """Calculates area density (atom/cm^2) using efficiency"""
        material = self._output_ws.sample().getMaterial()
        ref_absXS = material.absorbXSection()
        xs_term = ref_absXS * self._efficiency_wavelength / TABULATED_WAVELENGTH
        if self._xsection_type == "TotalXSection":
            xs_term += material.totalScatterXSection()
        self._area_density = - np.log( 1.0 - self._efficiency) / xs_term

    def _calculate_area_density_from_density(self):
        """Calculates area density (atom/cm^2) using number density and thickness."""
        if self._density_type == 'Mass Density':
            builder = MaterialBuilder().setFormula(self._chemical_formula)
            mat = builder.setMassDensity(self._density).build()
            self._density = mat.numberDensity
        self._area_density = self._density * self._thickness

    def _calculate_alpha_absXS_term(self):
        """Calculates absorption XS alpha term = area_density * absXS / 1.7982"""
        material = self._output_ws.sample().getMaterial()
        absXS = material.absorbXSection()  / TABULATED_WAVELENGTH
        self._alpha_absXS = self._area_density * absXS

    def _calculate_alpha_scatXS_term(self):
        """Calculates scattering XS alpha term = area_density * scatXS"""
        material = self._output_ws.sample().getMaterial()
        scatXS = material.totalScatterXSection()
        self._alpha_scatXS = self._area_density * scatXS

    def _calculate_efficiency(self, wavelength):
        """
        Calculates efficiency of a detector / monitor at a given wavelength.
        If using just the absorption cross section, _alpha_scatXS is == 0.

        @param wavelength Wavelength at which to calculate (in Angstroms)
        @return efficiency
        """
        efficiency = 1. / (1.0 - np.exp(-(self._alpha_scatXS + self._alpha_absXS * wavelength)))
        return efficiency


AlgorithmFactory.subscribe(CalculateEfficiencyCorrection)
