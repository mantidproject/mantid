# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import math
import numpy as np


class CalculateSampleTransmission(PythonAlgorithm):
    _bin_params = None
    _chemical_formula = None
    _density_type = None
    _density = None
    _thickness = None
    _output_ws = None

    def category(self):
        return "Sample"

    def seeAlso(self):
        return ["SetSampleMaterial"]

    def summary(self):
        return "Calculates the scattering & transmission for a given sample material and size over a given wavelength range."

    def PyInit(self):
        self.declareProperty(
            name="WavelengthRange",
            defaultValue="",
            validator=StringMandatoryValidator(),
            doc="Wavelength range to calculate transmission for.",
        )

        self.declareProperty(name="ChemicalFormula", defaultValue="", validator=StringMandatoryValidator(), doc="Sample chemical formula")

        self.declareProperty(
            name="DensityType",
            defaultValue="Mass Density",
            validator=StringListValidator(["Mass Density", "Number Density"]),
            doc="Use of Mass density or Number density",
        )

        self.declareProperty(
            name="Density", defaultValue=0.1, doc="Mass density (g/cm^3) or Number density (atoms/Angstrom^3). Default=0.1"
        )

        self.declareProperty(name="Thickness", defaultValue=0.1, doc="Sample thickness (cm). Default=0.1")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            doc="Outputs the sample transmission over the wavelength range as a function of wavelength.",
        )

    def validateInputs(self):
        issues = dict()

        density = self.getProperty("Density").value
        if density < 0.0:
            issues["Density"] = "Density must be positive"

        thickness = self.getProperty("Thickness").value
        if thickness < 0.0:
            issues["Thickness"] = "Thickness must be positive"

        return issues

    def PyExec(self):
        self._setup()

        # Create the workspace and set the sample material
        CreateWorkspace(
            OutputWorkspace=self._output_ws,
            NSpec=2,
            DataX=[0, 1],
            DataY=[0, 0],
            VerticalAxisUnit="Text",
            VerticalAxisValues="Transmission,Scattering",
        )
        Rebin(InputWorkspace=self._output_ws, OutputWorkspace=self._output_ws, Params=self._bin_params)

        if self._density_type == "Mass Density":
            builder = MaterialBuilder()
            mat = builder.setFormula(self._chemical_formula).setMassDensity(self._density).build()
            self._density = mat.numberDensity
        SetSampleMaterial(InputWorkspace=self._output_ws, ChemicalFormula=self._chemical_formula, SampleNumberDensity=self._density)
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

        self.setProperty("OutputWorkspace", self._output_ws)

    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._bin_params = self.getPropertyValue("WavelengthRange")
        self._chemical_formula = self.getPropertyValue("ChemicalFormula")
        self._density_type = self.getPropertyValue("DensityType")
        self._density = self.getProperty("Density").value
        self._thickness = self.getProperty("Thickness").value
        self._output_ws = self.getPropertyValue("OutputWorkspace")

    def _calculate_at_wavelength(self, wavelength):
        """
        Calculates transmission and scattering at a given wavelength.

        @param wavelength Wavelength at which to calculate (in Angstroms)
        @return Tuple of transmission and scattering percentages
        """

        TABULATED_WAVELENGTH = 1.798

        material = mtd[self._output_ws].mutableSample().getMaterial()

        absorption_x_section = material.absorbXSection() * wavelength / TABULATED_WAVELENGTH
        total_x_section = absorption_x_section + material.totalScatterXSection()

        transmission = math.exp(-self._density * total_x_section * self._thickness)
        scattering = 1.0 - math.exp(-self._density * material.totalScatterXSection() * self._thickness)

        return transmission, scattering


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateSampleTransmission)
