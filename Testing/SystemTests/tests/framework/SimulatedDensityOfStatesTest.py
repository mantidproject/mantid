from abins.test_helpers import find_file
from mantid.simpleapi import SimulatedDensityOfStates
from systemtesting import MantidSystemTest


class SimulatedDensityOfStatesTest(MantidSystemTest):
    """Test from castep .phonon file at precalculated q-points"""

    def runTest(self):
        SimulatedDensityOfStates(
            CASTEPFile=find_file("Na2SiF6_CASTEP.phonon"), Function="Gaussian", SpectrumType="DOS", OutputWorkspace="Na2SiF6_DOS"
        )

    def validate(self):
        return ("Na2SiF6_DOS", "Na2SiF6_DOS.nxs")


class SimulatedDensityOfStatesEuphonicTest(MantidSystemTest):
    """Interpolate phonon data from Phonopy yaml using Euphonic"""

    def runTest(self):
        SimulatedDensityOfStates(
            ForceConstantsFile=find_file("phonopy-Al.yaml"), Function="Gaussian", SpectrumType="DOS", OutputWorkspace="phonopy-Al_DOS"
        )

    def validate(self):
        return ("phonopy-Al_DOS", "phonopy-Al_DOS.nxs")
