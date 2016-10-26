from __future__ import (absolute_import, division, print_function)

from mantid.kernel import Direction, FloatArrayProperty
from mantid.api import (PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty)

import numpy as np
import math


class VesuvioThickness(PythonAlgorithm):

    _masses = None
    _amplitudes = None
    _transmission_guess = None
    _thickness = None
    _number_density = None
    FOUR_PI = (math.pi * 4)

    def summary(self):
        return "Produces the sample density for vesuvio based on sample transmission and composite masses"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        self.declareProperty(FloatArrayProperty("Masses", direction=Direction.Input),
                             doc="The masses that make up the sample")

        self.declareProperty(FloatArrayProperty("Amplitudes", direction=Direction.Input),
                             doc="The amplitudes of the peaks")

        self.declareProperty("TransmissionGuess", 1.0,
                             doc="Initial guess for the transmission")

        self.declareProperty("Thickness", 5.0,
                             doc="The thickness of the sample in centimetres (cm)")

        self.declareProperty("NumberDensity", 1.0,
                             doc="The Number Density of the sample material")

        self.declareProperty(ITableWorkspaceProperty("DensityWorkspace", "",
                                                     direction=Direction.Output),
                             doc="Output Workspace containing the iterative "
                             +"approximations for Sample Density. The final "
                             +"Y value in the first spectrum will be the last iteration")

        self.declareProperty(ITableWorkspaceProperty("TransmissionWorkspace", "",
                                                     direction=Direction.Output),
                             doc="Output Workspace containing the iterative "
                             +"approximation for Transmission.")

    def _get_properties(self):
        self._masses = self.getProperty("Masses").value
        self._amplitudes = self.getProperty("Amplitudes").value
        self._transmission_guess = self.getProperty("TransmissionGuess").value
        self._thickness = self.getProperty("Thickness").value
        self._number_density = self.getProperty("NumberDensity").value

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        num_masses = len(self._masses)
        num_amplitudes = len(self._amplitudes)
        if num_masses == 0:
            issues['Masses'] = ('Must have 1 or more Masses defined')
        if num_amplitudes == 0:
            issues['Amplitudes'] = ('Must have 1 or more Amplitudes defined')

        if num_masses != num_amplitudes:
            issues['Masses'] = ('The number of masses: %d, ' % num_masses
                                + 'is not equal to the number of amplitudes: %d' % num_amplitudes)

        return issues

    def PyExec(self):
        # Create numpy arrays
        self._masses = np.asarray(self._masses)
        self._amplitudes = np.asarray(self._amplitudes)

        # Initialise output table workspaces
        create_tbl_alg = self.createChildAlgorithm("CreateEmptyTableWorkspace")

        create_tbl_alg.setProperty('OutputWorkspace', 'density_guesses')
        create_tbl_alg.execute()
        density_guesses_tbl_ws = create_tbl_alg.getProperty('OutputWorkspace').value
        density_guesses_tbl_ws.addColumn("str", "Iteration")
        density_guesses_tbl_ws.addColumn("double", "Density")
        density_guesses_tbl_ws.setPlotType(0, 1)

        create_tbl_alg.setProperty('OutputWorkspace', 'transmission_guesses')
        create_tbl_alg.execute()
        trans_guesses_tbl_ws = create_tbl_alg.getProperty('OutputWorkspace').value
        trans_guesses_tbl_ws.addColumn("str","Iteration")
        trans_guesses_tbl_ws.addColumn("double","Transmission")
        trans_guesses_tbl_ws.setPlotType(0, 1)

        ### Unit conversions and scatter length calculation ###
        #Thickness converted to m and halved
        self._thickness /= 200.0
        scatter_length = np.sqrt(np.divide(self._amplitudes, self.FOUR_PI))
        # Mass * atomic mass unit(g)
        total_mass = self._masses*1.66054e-24
        total_mass = sum(total_mass)

        for i in range(10):
            ndens = (self._number_density/total_mass)*1e6
            xst = self.free_xst(self._masses, scatter_length)
            attenuation_length = ndens*xst*1e-28

            dmur = 2*attenuation_length*self._thickness
            trans_guess = math.exp(-dmur)
            self._number_density = ((1-self._transmission_guess)/(1-trans_guess))*self._number_density
            # Add guesses to output workspaces
            density_guesses_tbl_ws.addRow([str(i+1), self._number_density])
            trans_guesses_tbl_ws.addRow([str(i+1), trans_guess])

        self.setProperty("DensityWorkspace", density_guesses_tbl_ws)
        self.setProperty("TransmissionWorkspace", trans_guesses_tbl_ws)

    def free_xst(self, Mass, scatter_length):
        """
        Analytic expression for integration of PDCS over E1 and solid angle
        """
        # Neutron rest mass(u) / Mass
        xs_masses = 1.00867/Mass
        scatter_len_sq = np.square(scatter_length)
        cross_section = np.divide((self.FOUR_PI * scatter_len_sq),(np.square(xs_masses+1)))
        xs_sum = sum(cross_section)
        return xs_sum


AlgorithmFactory.subscribe(VesuvioThickness)
