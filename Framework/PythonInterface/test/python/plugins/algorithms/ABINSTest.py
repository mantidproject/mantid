import unittest
from mantid import logger
from mantid.api import ITableWorkspace
from mantid.simpleapi import ABINS, Scale, CompareWorkspaces

try:
    import scipy
except ImportError:
    logger.warning("Failure of  ABINSTest because scipy is unavailable.")
    exit(1)
try:
    import h5py
except ImportError:
    logger.warning("Failure of ABINSTest because h5py is unavailable.")
    exit(1)


class ABINSTest(unittest.TestCase):

    def setUp(self):
        self._dft_program = "CASTEP"
        self._benzene_phonon_file = "benzene.phonon"
        self._squaricn_phonon_file = "squaricn.phonon"
        self._experimental_file = "benzene.dat"
        self._temperature = 10.0 # temperature 10 K
        self._scale = 0.01
        self._sample_form = "Powder"
        self._instrument = "TOSCA"
        self._atoms = ""
        self._sum_contributions = True
        self._overtones = True
        self._cross_section_factor = "Total"
        self._workspace_name = "output_workspace"

    # test if algorithm react properly for wrong user input
    def test_wrong_input(self):
        """Test if the correct behaviour of algorithm in case input is not valid"""

        #  invalid CASTEP file missing:  Number of branches     6 in the header file
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc_wrong.phonon", OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CASTEP
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc.wrong_phonon", OutputWorkspace=self._workspace_name)

        # two dots in filename
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2.sc.phonon", OutputWorkspace=self._workspace_name)

        # no name for workspace
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._benzene_phonon_file, Temperature=self._temperature)

        # negative temperature in K
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._benzene_phonon_file, Temperature=-1.0, OutputWorkspace=self._workspace_name)

        # negative scale
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._benzene_phonon_file, Scale=-0.2, OutputWorkspace=self._workspace_name)

        # overtones cannot be set in case we have single crystal (don't know expansion of S in terms of overtones in case of the SingleCrystal)
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._benzene_phonon_file, SampleForm="SingleCrystal", Overtones=True, OutputWorkspace=self._workspace_name)


    # test if intermediate results are consistent
    def test_non_existing_atoms(self):
        """Test scenario in which  a user requests to create workspaces for atoms which do not exist in the system.
           In that case ABINS should terminate and give a user a meaningful message about wrong atoms to analyse.
        """
        # In benzene there is no O atoms
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._benzene_phonon_file, Atoms="O", OutputWorkspace=self._workspace_name)


    def test_scale(self):
        """
        Test if scaling is correct.
        @return:
        """
        wks = ABINS(PhononFile=self._squaricn_phonon_file, Scale=10, OutputWorkspace="scaled_workspace")
        ref = ABINS(PhononFile=self._squaricn_phonon_file, OutputWorkspace="non_scaled_workspace")
        ref = Scale(ref, Factor=10)

        (result, messages) = CompareWorkspaces(wks, ref)
        self.assertEqual(result, True)


    def test_partial(self):


        # By default workspaces for all atoms should be created
        wks_all_atoms_explicitly = ABINS(PhononFile=self._squaricn_phonon_file,
                                         Atoms="H, C, O",
                                         OutputWorkspace="explicit")

        wsk_all_atoms_default = ABINS(PhononFile=self._squaricn_phonon_file,
                                      OutputWorkspace="default")

        (result, messages) = CompareWorkspaces(wks_all_atoms_explicitly, wsk_all_atoms_default)
        self.assertEqual(result, True)

        workspaces = wks_all_atoms_explicitly.getNames()
        self.assertEquals(len(workspaces), 3)


    # main tests
    def test_good_case(self):
        pass



if __name__=="__main__":
    unittest.main()