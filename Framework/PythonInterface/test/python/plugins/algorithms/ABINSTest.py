import unittest
from mantid import logger
from mantid.simpleapi import mtd
from mantid.simpleapi import ABINS, Scale, CompareWorkspaces, Load

import os

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

    def remove_hdf_files(self):
        files = os.listdir(os.getcwd())
        for filename in files:
            if self.Si2 in filename or self.Squaricn in filename or "benzene" in filename:
                os.remove(filename)


    def tearDown(self):
        self.remove_hdf_files()


    def setUp(self):

        # set all parameters for tests
        self._dft_program = "CASTEP"
        self.Si2 = "Si2-sc"
        self.Squaricn = "squaricn_sum"
        self._experimental_file = ""
        self._temperature = 10.0  # temperature 10 K
        self._scale = 1.0
        self._sample_form = "Powder"
        self._instrument = "TOSCA"
        self._atoms = ""  # if no atoms are specified then all atoms are taken into account
        self._sum_contributions = True
        self._overtones = True
        self._cross_section_factor = "Incoherent"
        self._workspace_name = "output_workspace"
        self._tolerance = 0.0001

        # produce reference data
        self._ref_wrk = {self.Si2: ABINS(DFTprogram=self._dft_program,
                                         PhononFile=self.Si2 + ".phonon",
                                         ExperimentalFile=self._experimental_file,
                                         Temperature=self._temperature,
                                         SampleForm=self._sample_form,
                                         Instrument=self._instrument,
                                         Atoms=self._atoms,
                                         Scale=self._scale,
                                         SumContributions=self._sum_contributions,
                                         Overtones=self._overtones,
                                         ScaleByCrossSection=self._cross_section_factor,
                                         OutputWorkspace="benzene_ref"),

                         self.Squaricn: ABINS(DFTprogram=self._dft_program,
                                                   PhononFile=self.Squaricn + ".phonon",
                                                   ExperimentalFile=self._experimental_file,
                                                   Temperature=self._temperature,
                                                   SampleForm=self._sample_form,
                                                   Instrument=self._instrument,
                                                   Atoms=self._atoms,
                                                   Scale=self._scale,
                                                   SumContributions=self._sum_contributions,
                                                   Overtones=self._overtones,
                                                   ScaleByCrossSection=self._cross_section_factor,
                                                   OutputWorkspace="squaricn_ref")
                         }

        self.remove_hdf_files()


    def test_wrong_input(self):
        """Test if the correct behaviour of algorithm in case input is not valid"""

        #  invalid CASTEP file missing:  Number of branches     6 in the header file
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc_wrong.phonon", OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CASTEP
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2-sc.wrong_phonon", OutputWorkspace=self._workspace_name)

        # two dots in filename
        self.assertRaises(RuntimeError, ABINS, PhononFile="Si2.sc.phonon", OutputWorkspace=self._workspace_name)

        # no name for workspace
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Si2 + ".phonon", Temperature=self._temperature)

        # negative temperature in K
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Si2 + ".phonon", Temperature=-1.0, OutputWorkspace=self._workspace_name)

        # negative scale
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Si2 + ".phonon", Scale=-0.2, OutputWorkspace=self._workspace_name)

        # overtones cannot be set in case we have single crystal (don't know expansion of S in terms of overtones for SingleCrystal case)
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Si2 + ".phonon", SampleForm="SingleCrystal", Overtones=True, OutputWorkspace=self._workspace_name)

    # test if intermediate results are consistent
    def test_non_unique_atoms(self):
        """Test scenario in which a user specifies non unique atoms (for example in squaricn that would be "C,C,H").
           In that case ABINS should terminate and print a meaningful message.
        """
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Squaricn + ".phonon", Atoms="C,C,H", OutputWorkspace=self._workspace_name)


    def test_non_existing_atoms(self):
        """Test scenario in which  a user requests to create workspaces for atoms which do not exist in the system.
           In that case ABINS should terminate and give a user a meaningful message about wrong atoms to analyse.
        """
        # In Si2 there is no C atoms
        self.assertRaises(RuntimeError, ABINS, PhononFile=self.Si2 + ".phonon", Atoms="C", OutputWorkspace=self._workspace_name)


    def test_scale(self):
        """
        Test if scaling is correct.
        @return:
        """
        wrk = ABINS(DFTprogram=self._dft_program,
                    PhononFile=self.Squaricn + ".phonon",
                    Temperature=self._temperature,
                    SampleForm=self._sample_form,
                    Instrument=self._instrument,
                    Atoms=self._atoms,
                    SumContributions=self._sum_contributions,
                    Overtones=self._overtones,
                    Scale=10,
                    ScaleByCrossSection=self._cross_section_factor,
                    OutputWorkspace="squaricn_no_scale")

        ref = Scale(self._ref_wrk[self.Squaricn], Factor=10)

        (result, messages) = CompareWorkspaces(wrk, ref, Tolerance=self._tolerance)
        self.assertEqual(result, True)


    def test_exp(self):
        """
        Tests if experimental data is loaded correctly.
        @return:
        """
        ABINS(DFTprogram=self._dft_program,
              PhononFile="benzene.phonon",
              ExperimentalFile="benzene.dat",
              Temperature=self._temperature,
              SampleForm=self._sample_form,
              Instrument=self._instrument,
              Atoms=self._atoms,
              Scale=self._scale,
              SumContributions=self._sum_contributions,
              Overtones=self._overtones,
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
        wks_all_atoms_explicitly = ABINS(PhononFile=self.Squaricn + ".phonon",
                                         Atoms="H, C, O",
                                         SumContributions=self._sum_contributions,
                                         Overtones=self._overtones,
                                         OutputWorkspace="explicit")

        wsk_all_atoms_default = ABINS(PhononFile=self.Squaricn + ".phonon",
                                      SumContributions=self._sum_contributions,
                                      Overtones=self._overtones,
                                      OutputWorkspace="default")

        (result, messages) = CompareWorkspaces(wks_all_atoms_explicitly, wsk_all_atoms_default, Tolerance=self._tolerance)
        self.assertEqual(result, True)

        (result, messages) = CompareWorkspaces(self._ref_wrk[self.Squaricn], wsk_all_atoms_default, Tolerance=self._tolerance)
        self.assertEqual(result, True)

    # main tests
    def test_good_cases_from_scratch(self):
        """
        Test case when simulation is started from scratch
        @return:
        """
        self._good_case_from_scratch(self.Squaricn)
        self._good_case_from_scratch(self.Si2)


    def _good_case_from_scratch(self, filename):
        # calculate workspaces
        wrk_calculated = ABINS(DFTprogram=self._dft_program,
                               PhononFile=filename+ ".phonon",
                               Temperature=self._temperature,
                               SampleForm=self._sample_form,
                               Instrument=self._instrument,
                               Atoms=self._atoms,
                               SumContributions=self._sum_contributions,
                               Overtones=self._overtones,
                               ScaleByCrossSection=self._cross_section_factor,
                               OutputWorkspace=filename + "_scratch")

        (result, messages) = CompareWorkspaces(self._ref_wrk[filename], wrk_calculated, Tolerance=self._tolerance)
        self.assertEqual(True, result)


    def test_good_cases_restart(self):
        self._good_case_restart(self.Squaricn)
        self._good_case_restart(self.Si2)


    def _good_case_restart(self, filename):
        """
        Test case of restart. The considered testing scenario looks as follows. First the user performs the simulation
        for T=20K (first run). Then the user changes T to 10K (second run). For T=10K both DW factors and S have to be
        recalculated. After that the user performs simulation with the same parameters as initial simulation, e.g., T=10K
        (third run). In the third run all required data will be read from hdf file. It is checked if workspace for the
        initial run and third run is the same (should be the same). It is also checked if the workspace from the second
        run is valid. In this test it is checked if previously calculated data is read correctly from an hdf file.

        """

        # restart without any changes
        temperature_for_test = 20  # 20K
        wrk_name = filename

        wrk_initial = ABINS(DFTprogram=self._dft_program,
                            PhononFile=filename + ".phonon",
                            Temperature=temperature_for_test,
                            SampleForm=self._sample_form,
                            Instrument=self._instrument,
                            Atoms=self._atoms,
                            SumContributions=self._sum_contributions,
                            Overtones=self._overtones,
                            ScaleByCrossSection=self._cross_section_factor,
                            OutputWorkspace=wrk_name + "init")

        wrk_mod = ABINS(DFTprogram=self._dft_program,
                        PhononFile=filename + ".phonon",
                        Temperature=self._temperature,
                        SampleForm=self._sample_form,
                        Instrument=self._instrument,
                        Atoms=self._atoms,
                        SumContributions=self._sum_contributions,
                        Overtones=self._overtones,
                        ScaleByCrossSection=self._cross_section_factor,
                        OutputWorkspace=wrk_name + "_mod")

        wrk_restart = ABINS(DFTprogram=self._dft_program,
                            PhononFile=filename + ".phonon",
                            Temperature=temperature_for_test,
                            SampleForm=self._sample_form,
                            Instrument=self._instrument,
                            Atoms=self._atoms,
                            SumContributions=self._sum_contributions,
                            Overtones=self._overtones,
                            ScaleByCrossSection=self._cross_section_factor,
                            OutputWorkspace=wrk_name + "restart")


        (result, messages) = CompareWorkspaces(wrk_initial, wrk_restart, Tolerance=self._tolerance)
        self.assertEqual(True, result)

        (result, messages) = CompareWorkspaces(self._ref_wrk[filename], wrk_mod, Tolerance=self._tolerance)
        self.assertEqual(True, result)


if __name__ == "__main__":
    unittest.main()
