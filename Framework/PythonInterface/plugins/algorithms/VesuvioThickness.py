from mantid.kernel import Direction, logger
from mantid.api import (PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, Progress)
import mantid.simpleapi as ms

import numpy as np
import math

def sq(list):
    out = []
    for i in range(len(list)):
        out.append(list[i]**2)
    return out

class VesuvioThickness(PythonAlgorithm):

    _masses = None
    _amplitudes = None
    _transmission_guess = None
    _thickness = None
    _number_density = None

    def summary(self):
        return "Produces the sample density for vesuvio based on sample transmission and composite masses"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        self.declareProperty("Masses", "",
                             doc="A String list of the masses that make up the sample")

        self.declareProperty("Amplitudes", "",
                             doc="A string list of the amplitude of the peaks")

        self.declareProperty("TransmissionGuess", 241,
                             doc="Initial guess for the transmission")

        self.declareProperty("Thickness", 0.5,
                             doc="The thickness of the sample")

        self.declareProperty("NumberDensity", 10.0,
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
        self._masses =self.getPropertyValue("Masses").split(',')
        self._masses = [float(mass) for mass in self._masses]
        self._amplitudes = self.getPropertyValue("Amplitudes").split(',')
        self._amplitudes = [float(amp) for amp in self._amplitudes]
        self._transmission_guess = self.getProperty("TransmissionGuess").value
        self._thickness = self.getProperty("Thickness").value
        self._number_density = self.getProperty("NumberDensity").value


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
        self._thickness /= 200.0 # conversion to meters and then (1/2)

        scatter_length = []
        for i in range(len(self._amplitudes)):
            scatter_length.append(math.sqrt((self._amplitudes[i]) / (4.0 * math.pi)))

        # initialise table workspaces
        density_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        density_guesses_tbl_ws.addColumn("str", "Iteration")
        density_guesses_tbl_ws.addColumn("double", "Density")
        trans_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        trans_guesses_tbl_ws.addColumn("str","Iteration")
        trans_guesses_tbl_ws.addColumn("double","Transmission")

        for i in range(10):
            logger.warning("************ITERATION == " + str(i) + "**********************")
            total_masses = [mass * 1.66054e-24 for mass in self._masses] # conversion of masses to (kg)
            total_mass = sum(total_masses)
            logger.warning("total_mass = " + str(total_mass))
            ndens = self._number_density / (total_mass * 1.0e6)
            logger.warning("_number_density = " + str(self._number_density))
            logger.warning("ndens = " + str(ndens))
            xst = self.free_xst(self._masses, scatter_length) # xst = cross-section_total
            logger.warning("xst = " + str(xst))
            attenuation_length = ndens * xst * 1.0e-28

            logger.warning("attenuation_length = " + str(attenuation_length))
            logger.warning("_thickness = " + str(self._thickness))
            dmur = 2.0 * attenuation_length * self._thickness
            logger.warning("dmur = " +str(dmur))
            strans = math.exp(-dmur)
            logger.warning("strans = " + str(strans))
            logger.warning("num_dens = " + str(self._number_density))


            self._number_density = (1.0-self._transmission_guess) / (1.0-strans) * self._number_density

            # Add guesses to workspaces
            density_guesses_tbl_ws.addRow([str(i+1), self._number_density])
            trans_guesses_tbl_ws.addRow([str(i+1), strans])

        self.setProperty("DensityWorkspace", density_guesses_tbl_ws)
        self.setProperty("TransmissionWorkspace", trans_guesses_tbl_ws)


    def free_xst(self, masses, scatter_length):
        """
        Analytic expression for integration of PDCS over E1 and solid angle
        """
        G = [1.00867 / mass for mass in masses]
        cross_section = [4 * math.pi * scat_len for scat_len in scatter_length]
        cross_sec_sq = sq(cross_section)
        G_plus_1 = [g + 1 for g in G]
        sq_G = sq(G_plus_1)
        xs = []
        for i in range(len(sq_G)):
            xs.append(cross_sec_sq[i]/sq_G[i])
        xs_sum = sum(xs)
        return xs_sum


AlgorithmFactory.subscribe(VesuvioThickness)
