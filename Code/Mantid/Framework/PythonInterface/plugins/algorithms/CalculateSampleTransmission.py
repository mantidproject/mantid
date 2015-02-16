from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import math
import numpy as np


class CalculateSampleTransmission(PythonAlgorithm):

    def category(self):
        return 'Sample'


    def summary(self):
        return 'Calculates the scattering & transmission for a given sample material and size over a given wavelength range.'


    def PyInit(self):
        self.declareProperty(name='WavelengthRange', defaultValue='', validator=StringMandatoryValidator(),
                             doc='Wavelength range to calculate transmission for.')

        self.declareProperty(name='ChemicalFormula', defaultValue='', validator=StringMandatoryValidator(),
                             doc='Sample chemical formula')

        self.declareProperty(name='NumberDensity', defaultValue=0.1,
                             doc='Number denisty (atoms/Angstrom^3). Default=0.1')

        self.declareProperty(name='Thickness', defaultValue=0.1,
                             doc='Sample thickness (cm). Default=0.1')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Outputs the sample transmission over the wavelength range as a function of wavelength.')


    def validateInputs(self):
        issues = dict()

        density = self.getProperty('NumberDensity').value
        if(density < 0.0):
            issues['NumberDensity'] = 'NumberDensity must be positive'

        thickness = self.getProperty('Thickness').value
        if(thickness < 0.0):
            issues['Thickness'] = 'Thickness must be positive'

        return issues


    def PyExec(self):
        self._setup()

        # Create the workspace and set the sample material
        CreateWorkspace(OutputWorkspace=self._output_ws, NSpec=2, DataX=[0, 0], DataY=[0, 0])
        Rebin(InputWorkspace=self._output_ws, OutputWorkspace=self._output_ws, Params=self._bin_params)
        SetSampleMaterial(InputWorkspace=self._output_ws, ChemicalFormula=self._chamical_formula)
        ConvertToPointData(InputWorkspace=self._output_ws, OutputWorkspace=self._output_ws)

        ws = mtd[self._output_ws]
        wavelengths = ws.readX(0)
        transmission_data = np.zeros(len(wavelengths))
        scattering_data = np.zeros(len(wavelengths))

        # Calculate transmission and scattering for each wavelength point
        for idx in range(0, len(wavelengths)):
            transmission, scattering = self._calculate_at_wavelength(wavelengths[idx])
            transmission_data[idx] = transmission
            scattering_data[idx] = scattering

        ws.setY(0, transmission_data)
        ws.setY(1, scattering_data)

        self.setProperty('OutputWorkspace', self._output_ws)


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._bin_params = self.getPropertyValue('WavelengthRange')
        self._chamical_formula = self.getPropertyValue('ChemicalFormula')
        self._density = self.getProperty('NumberDensity').value
        self._thickness = self.getProperty('Thickness').value
        self._output_ws = self.getPropertyValue('OutputWorkspace')


    def _calculate_at_wavelength(self, wavelength):
        """
        Calculates transmission and scattering at a given wavelength.

        @param wavelength Wavelength at which to calculate (in Angstrom)
        @return Tuple of transmission and scattering percentages
        """

        material = mtd[self._output_ws].mutableSample().getMaterial()

        absorption_x_section = material.absorbXSection() * wavelength
        total_x_section = absorption_x_section + material.totalScatterXSection()

        transmission = math.exp(-self._density * total_x_section * self._thickness)
        scattering = 1.0 - math.exp(-self._density * material.totalScatterXSection() * self._thickness)

        return transmission, scattering



# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateSampleTransmission)
