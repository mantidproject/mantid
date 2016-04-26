from mantid.kernel import Direction
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
        self.declareProperty("Masses", "",
                             doc="A comma seperated list of the masses that make up the sample")

        self.declareProperty("Amplitudes", "",
                             doc="A comma separated list of the amplitudes of the peaks")

        self.declareProperty("TransmissionGuess", 1.0,
                             doc="Initial guess for the transmission")

        self.declareProperty("Thickness", 5.0,
                             doc="The thickness of the sample")

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
        self._masses = self.getPropertyValue("Masses").split(',')
        self._amplitudes = self.getPropertyValue("Amplitudes").split(',')
        self._transmission_guess = self.getProperty("TransmissionGuess").value
        self._thickness = self.getProperty("Thickness").value
        self._number_density = self.getProperty("NumberDensity").value


    def validateInputs(self):
        self._get_properties()
        issues = dict()

        try:
            self._masses = np.asarray([float(mass) for mass in self._masses])
        except ValueError:
            issues['Masses'] = ('Could not interpret \"%s\" ' % self._masses \
                                    + 'as a list of numbers . Please ensure that ' \
                                    + 'all inputs are valid')
        try:
            self._amplitudes = np.asarray([float(amp) for amp in self._amplitudes])
        except ValueError:
            issues['Amplitudes'] = ('Could not interpret \"%s\" ' % self._amplitudes \
                                    + 'as a list of numbers . Please ensure that ' \
                                    + 'all inputs are valid')

        num_masses = len(self._masses)
        num_amplitudes = len(self._amplitudes)
        if num_masses != num_amplitudes:
            issues['Masses'] = ('The number of masses: %d, ' % num_masses \
                                + 'is not equal to the number of amplitudes: %d' % num_amplitudes)

        return issues


    def PyExec(self):
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

        # Unit conversions and scatter length calculation
        self._thickness /= 200.0
        scatter_length = np.sqrt(np.divide(self._amplitudes, self.FOUR_PI))
        total_mass = self._masses*1.66054e-24
        total_mass = sum(total_mass)

        for i in range(10):
            ndens = self._number_density/total_mass*1e6
            xst = self.free_xst(self._masses, scatter_length)
            attenuation_length = ndens*xst*1e-28

            dmur = 2*attenuation_length*self._thickness
            trans_guess = math.exp(-dmur)
            self._number_density = (1-self._transmission_guess)/(1-trans_guess)*self._number_density

            # Add guesses to output workspaces
            density_guesses_tbl_ws.addRow([str(i+1), self._number_density])
            trans_guesses_tbl_ws.addRow([str(i+1), trans_guess])

        self.setProperty("DensityWorkspace", density_guesses_tbl_ws)
        self.setProperty("TransmissionWorkspace", trans_guesses_tbl_ws)


    def free_xst(self, Mass, scatter_length):
        """
        Analytic expression for integration of PDCS over E1 and solid angle
        """

        xs_mass = 1.00867/Mass
        scatter_len_sq = np.square(scatter_length)
        cross_section = np.divide((self.FOUR_PI * scatter_len_sq),(np.square(xs_mass+1)))
        xs_sum = sum(cross_section)
        return xs_sum


AlgorithmFactory.subscribe(VesuvioThickness)
