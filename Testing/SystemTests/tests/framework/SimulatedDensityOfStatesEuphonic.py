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
        from dos.load_euphonic import euphonic_available

        from euphonic.cli.utils import force_constants_from_file  # noqa: F401
        from euphonic.util import mp_grid  # noqa: F401

        assert euphonic_available()

        SimulatedDensityOfStates(
            ForceConstantsFile=find_file("phonopy-Al.yaml"), Function="Gaussian", SpectrumType="DOS", OutputWorkspace="phonopy-Al_DOS"
        )

    def validate(self):
        return ("phonopy-Al_DOS", "phonopy-Al_DOS.nxs")
