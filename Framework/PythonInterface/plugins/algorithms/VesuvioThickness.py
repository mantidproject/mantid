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
        i = 0

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        '''num_masses = len(self._masses)
        num_amplitudes = len(self._amplitudes)
        if num_masses != num_amplitudes:
            issues['Masses'] = ('The number of masses: %d, ' % num_masses \
                               + 'is not equal to the number of amplitudes: %d' % num_amplitudes)'''

        return issues


    def PyExec(self):
        Mass = self.getPropertyValue("Masses").split(',')
        Mass = np.asarray([float(mass) for mass in Mass])
        Amplitudes = self.getPropertyValue("Amplitudes").split(',')
        Amplitudes = np.asarray([float(amp) for amp in Amplitudes])
        trans = self.getProperty("TransmissionGuess").value
        d = self.getProperty("Thickness").value
        dens = self.getProperty("NumberDensity").value

        # initialise table workspaces
        density_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        density_guesses_tbl_ws.addColumn("str", "Iteration")
        density_guesses_tbl_ws.addColumn("double", "Density")
        trans_guesses_tbl_ws = ms.CreateEmptyTableWorkspace()
        trans_guesses_tbl_ws.addColumn("str","Iteration")
        trans_guesses_tbl_ws.addColumn("double","Transmission")



        d = d / 200.0

        FOUR_PI = 4.0 * math.pi
        AMP_OVER_FOUR_PI = np.divide(Amplitudes, FOUR_PI)
        b = np.sqrt(AMP_OVER_FOUR_PI)


        for i in range(10):
            TMass = Mass*1.66054e-24
            TMass = sum(TMass)
            ndens = dens/TMass*1e6
            xst = self.free_xst(Mass, b)
            mu = ndens*xst*1e-28

            dmur = 2*mu*d
            strans = math.exp(-dmur)
            dens = (1-trans)/(1-strans)*dens

            # Add guesses to workspaces
            density_guesses_tbl_ws.addRow([str(i+1), dens])
            trans_guesses_tbl_ws.addRow([str(i+1), strans])
            logger.warning("Iteration %d : dens = %f " % (i, dens))
            logger.warning("Iteration %d : strans = %f " % (i, strans))

        self.setProperty("DensityWorkspace", density_guesses_tbl_ws)
        self.setProperty("TransmissionWorkspace", trans_guesses_tbl_ws)


    def free_xst(self, Mass, b):
        """
        Analytic expression for integration of PDCS over E1 and solid angle
        """

        G = 1.00867/Mass
        FOUR_PI = 4*math.pi
        B_SQUARED = np.square(b)
        xs = np.divide((FOUR_PI * B_SQUARED),(np.square(G+1)))
        xs_sum = sum(xs)
        return xs_sum


AlgorithmFactory.subscribe(VesuvioThickness)
