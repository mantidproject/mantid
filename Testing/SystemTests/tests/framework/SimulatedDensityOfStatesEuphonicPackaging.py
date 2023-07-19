from abins.test_helpers import find_file
from mantid.simpleapi import SimulatedDensityOfStates
from systemtesting import MantidSystemTest


class SimulatedDensityOfStatesTest(MantidSystemTest):
    """Make sure normal case will run regardless of Euphonic status"""

    def runTest(self):
        SimulatedDensityOfStates(
            CASTEPFile=find_file("Na2SiF6_CASTEP.phonon"), Function="Gaussian", SpectrumType="DOS", OutputWorkspace="Na2SiF6_DOS"
        )

    def validate(self):
        return ("Na2SiF6_DOS", "Na2SiF6_DOS.nxs")


class SimulatedDensityOfStatesEuphonicTest(MantidSystemTest):
    """ "Install Euphonic library to temporary prefix and check results"""

    def runTest(self):
        SimulatedDensityOfStates(
            ForceConstantsFile=find_file("phonopy-Al.yaml"), Function="Gaussian", SpectrumType="DOS", OutputWorkspace="phonopy-Al_DOS"
        )

    def validate(self):
        return ("phonopy-Al_DOS", "phonopy-Al_DOS.nxs")
