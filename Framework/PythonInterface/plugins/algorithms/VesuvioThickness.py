from mantid.kernel import Direction
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress)

import numpy as np
import math


class VesuvioThickness(PythonAlgorithm):

    _masses = None
    _amplitudes = None
    _transmission_guess = None
    _thickness = None
    _number_density = None

    def summary(self):
        return "Produces the sample density for vesuvio " \
               + "based on sample transmission and composite masses"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        self.declareProperty("Masses", "",
                             doc="A String list of the masses that make up the sample")

        self.declareProperty("Amplitude", "",
                             doc="A string list of the amplitude of the peaks")

        self.declareProperty("TransmissionGuess", 241,
                             doc="Initial guess for the transmission")

        self.declareProperty("Thickness", 0.5,
                             doc="The thickness of the sample")

        self.declareProperty("NumberDensity", 10,
                             doc="The Number Density of the sample material")

        self.declareProperty(ITableWorkspaceProperty("DensityWorkspace", "",
                             direction=Direction.Output)
                             doc="Output Workspace containing the iterative " \
                             +"approximations for Sample Density. The final " \
                             +"Y value in the first spectrum will be the last iteration")
                             
        self.declareProperty(ITableWorkspaceProperty("TransmissionWorkspace", "",
                             direction=Direction.Output)
                             doc="Output Workspace containing the iterative " \
                             +"approximation for Transmission.")


    def _get_properties(self):
        self._masses =self.getPropertyValue("Masses").split(',')
        self._masses = [float(mass) for mass in self._masses]
        self._amplitudes = self.getPropertyValue("Amplitude").split(',')
        self._amplitudes = [float(amp) for amp in self._amplitudes]
        self._transmission_guess = self.getPropertyValue("TansmissionGuess")
        self._thickness = self.getPropertyValue("Thickness")
        self._number_density = self.getPropertyValue("NumberDensity")


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        num_masses = len(self._masses)
        num_amplitudes = len(self._amplitudes)
        if num_masses != num_amplitudes:
            issues['Masses'] = ('The number of masses: %d, ' % num_masses \
                               + 'is not equal to the number of amplitudes: %d' % num_amplitudes)

        return issues


    def PyExec(self):
        self._thickness /= 200 # conversion to meters and then (1/2)

        scatter_length = []
        for i in range(len(self._amplitudes)):
            scatter_length[i] = math.sqrt((self._amplitudes[i]) / (4 * math.pi))

        # initialise table workspaces
        density_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        density_guesses_tbl_ws.addColumn("str", "Iteration")
        density_guesses_tbl_ws.addColumn("double", "Density")
        trans_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        trans_guesses_tbl_ws.addColumn("str","Iteration")
        trans_guesses_tbl_ws.addColumn("double","Transmission")

        for i in range(10):
            total_masses = [mass * 1.66054e-24 for mass in self._masses] # conversion of masses to (kg)
            total_mass = sum(total_masses)
            ndens = self._number_density / total_mass * 1e6 # ndens mihgt be global??
            xst = free_xst(masses, scatter_length) # xst = cross-section_total
            attenuation_length = ndens * xst * 1e-28

            dmur = 2 * attenuation_length * self._thickness
            strans = exp(-dmur)
            dens = (1-self.transmission_guess) / (1-strans) * dens

            # Add guesses to workspaces
            density_guesses_tbl_ws.addRow([str(i+1), dens])
            trans_guesses_tbl_ws([str(i+1), strans])

        self.setProperty("DensityWorkspace", density_guesses_tbl_ws)
        self.setProperty("TransmissionWorkspace", trans_guesses_tbl_ws)


    def free_xst(masses, scatter_length):
        """
        Analytic expression for integration of PDCS over E1 and solid angle
        """
        G = [mass * 1.00867 for mass in masses]
        cross_section = 4 * math.pi * [scat_len * scat_len for scat_len in scatter_length]
        for i in range(len(G)):
            cross_section[i] /= ((1+G[i]) * G[i])
        return sum(cross_section)

AlgorithmFactory.subscribe(VesuvioThickness)
