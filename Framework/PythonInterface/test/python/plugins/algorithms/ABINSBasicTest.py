import unittest
from mantid import logger
from mantid.simpleapi import mtd
from mantid.simpleapi import ABINS, Scale, CompareWorkspaces, Load, DeleteWorkspace
from AbinsModules import AbinsConstants, AbinsTestHelpers
import numpy as np


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSBasicTest because Python is too old.")

    is_numpy_old = AbinsTestHelpers.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping ABINSBasicTest because numpy is too old.")

    return is_python_old or is_numpy_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_modules)
class ABINSBasicTest(unittest.TestCase):

    _si2 = "Si2-sc_ABINS"
    _squaricn = "squaricn_sum_ABINS"

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["ABINS", "explicit", "restart",  "default", "total",
                                                            "no_scale", "exp"])

        # remove workspaces
        DeleteWorkspace(self._squaricn + "_ref")
        # DeleteWorkspace(self._si2 + "_ref")

    def setUp(self):

        # set all parameters for tests
        self._dft_program = "CASTEP"
        self._experimental_file = ""
        self._temperature = 10.0  # temperature 10 K
        self._scale = 1.0
        self._sample_form = "Powder"
        self._instrument_name = "TOSCA"
        self._atoms = ""  # if no atoms are specified then all atoms are taken into account
        self._sum_contributions = True

        # this is a string; once it is read it is converted internally to  integer
        self._quantum_order_events_number = str(AbinsConstants.FUNDAMENTALS)

        self._cross_section_factor = "Incoherent"
        self._workspace_name = "output_workspace"
        self._tolerance = 0.0001

        # produce reference data
        self._ref_wrk = {self._squaricn + "_abins": ABINS(DFTprogram=self._dft_program,
                                                          PhononFile=self._squaricn + ".phonon",
                                                          ExperimentalFile=self._experimental_file,
                                                          Temperature=self._temperature,
                                                          SampleForm=self._sample_form,
                                                          Instrument=self._instrument_name,
                                                          Atoms=self._atoms,
                                                          Scale=self._scale,
                                                          SumContributions=self._sum_contributions,
                                                          QuantumOrderEventsNumber=self._quantum_order_events_number,
                                                          ScaleByCrossSection=self._cross_section_factor,
                                                          OutputWorkspace=self._squaricn + "_ref")
                          }

    def test_wrong_input(self):
        """Test if the correct behaviour of algorithm in case input is not valid"""

        #  invalid CASTEP file missing:  Number of branches     6 in the header file
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc_wrong.phonon", OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CASTEP
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc.wrong_phonon", OutputWorkspace=self._workspace_name)

        # two dots in filename
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2.sc.phonon", OutputWorkspace=self._workspace_name)

        # no name for workspace
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._si2 + ".phonon", Temperature=self._temperature)

        # keyword total in the name of the workspace
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._si2 + ".phonon", Temperature=self._temperature,
                          OutputWorkspace=self._workspace_name + "total")

        # negative temperature in K
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._si2 + ".phonon", Temperature=-1.0,
                          OutputWorkspace=self._workspace_name)

        # negative scale
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._si2 + ".phonon", Scale=-0.2,
                          OutputWorkspace=self._workspace_name)

    # test if intermediate results are consistent
    def test_non_unique_atoms(self):
        """Test scenario in which a user specifies non unique atoms (for example in squaricn that would be "C,C,H").
           In that case ABINS should terminate and print a meaningful message.
        """
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._squaricn + ".phonon", Atoms="C,C,H",
                          OutputWorkspace=self._workspace_name)

    def test_non_existing_atoms(self):
        """Test scenario in which  a user requests to create workspaces for atoms which do not exist in the system.
           In that case ABINS should terminate and give a user a meaningful message about wrong atoms to analyse.
        """
        # In _si2 there is no C atoms
        self.assertRaises(RuntimeError, ABINS, PhononFile=self._squaricn + ".phonon", Atoms="N",
                          OutputWorkspace=self._workspace_name)

    def test_scale(self):
        """
        Test if scaling is correct.
        @return:
        """
        wrk = ABINS(DFTprogram=self._dft_program,
                    PhononFile=self._squaricn + ".phonon",
                    Temperature=self._temperature,
                    SampleForm=self._sample_form,
                    Instrument=self._instrument_name,
                    Atoms=self._atoms,
                    SumContributions=self._sum_contributions,
                    QuantumOrderEventsNumber=self._quantum_order_events_number,
                    Scale=10,
                    ScaleByCrossSection=self._cross_section_factor,
                    OutputWorkspace="squaricn_no_scale")

        ref = Scale(self._ref_wrk[self._squaricn + "_abins"], Factor=10)

        (result, messages) = CompareWorkspaces(wrk, ref, Tolerance=self._tolerance)
        self.assertEqual(result, True)

    def test_exp(self):
        """
        Tests if experimental data is loaded correctly.
        @return:
        """
        ABINS(DFTprogram=self._dft_program,
              PhononFile="benzene_ABINS.phonon",
              ExperimentalFile="benzene_ABINS.dat",
              Temperature=self._temperature,
              SampleForm=self._sample_form,
              Instrument=self._instrument_name,
              Atoms=self._atoms,
              Scale=self._scale,
              SumContributions=self._sum_contributions,
              QuantumOrderEventsNumber=self._quantum_order_events_number,
              ScaleByCrossSection=self._cross_section_factor,
              OutputWorkspace="benzene_exp")

        # load experimental data
        Load(Filename="benzene.dat", OutputWorkspace="benzene_only_exp")

        (result, messages) = CompareWorkspaces(Workspace1=mtd["experimental_wrk"],
                                               Workspace2=mtd["benzene_only_exp"],
                                               CheckAxes=False,
                                               Tolerance=self._tolerance)
        self.assertEqual(result, True)

    def test_partial(self):
        # By default workspaces for all atoms should be created. Test this default behaviour.
        wks_all_atoms_explicitly = ABINS(PhononFile=self._squaricn + ".phonon",
                                         Atoms="H, C, O",
                                         SumContributions=self._sum_contributions,
                                         QuantumOrderEventsNumber=self._quantum_order_events_number,
                                         OutputWorkspace="explicit")

        wsk_all_atoms_default = ABINS(PhononFile=self._squaricn + ".phonon",
                                      SumContributions=self._sum_contributions,
                                      QuantumOrderEventsNumber=self._quantum_order_events_number,
                                      OutputWorkspace="default")

        (result, messages) = CompareWorkspaces(wks_all_atoms_explicitly, wsk_all_atoms_default,
                                               Tolerance=self._tolerance)
        self.assertEqual(result, True)

        (result, messages) = CompareWorkspaces(self._ref_wrk[self._squaricn + "_abins"], wsk_all_atoms_default,
                                               Tolerance=self._tolerance)
        self.assertEqual(result, True)


if __name__ == "__main__":
    unittest.main()
