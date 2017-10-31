from __future__ import (absolute_import, division, print_function)
import unittest
from mantid import logger
# noinspection PyUnresolvedReferences
from mantid.simpleapi import mtd, Abins, Scale, CompareWorkspaces, Load, DeleteWorkspace
from AbinsModules import AbinsConstants, AbinsTestHelpers
import numpy as np


class AbinsBasicTest(unittest.TestCase):

    _si2 = "Si2-sc_Abins"
    _squaricn = "squaricn_sum_Abins"
    _ab_initio_program = "CASTEP"
    _temperature = 10.0  # temperature 10 K
    _scale = 1.0
    _sample_form = "Powder"
    _instrument_name = "TOSCA"
    _atoms = ""  # if no atoms are specified then all atoms are taken into account
    _sum_contributions = True

    # this is a string; once it is read it is converted internally to  integer
    _quantum_order_events_number = str(AbinsConstants.FUNDAMENTALS)

    _cross_section_factor = "Incoherent"
    _workspace_name = "output_workspace"
    _tolerance = 0.0001

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["explicit",  "default", "total", "squaricn_sum_Abins",
                                                            "squaricn_scale", "benzene_exp", "benzene_Abins",
                                                            "experimental"])
        mtd.clear()

    def test_wrong_input(self):
        """Test if the correct behaviour of algorithm in case input is not valid"""

        #  invalid CASTEP file missing:  Number of branches     6 in the header file
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile="Si2-sc_wrong.phonon",
                          OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CASTEP
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile="Si2-sc.wrong_phonon",
                          OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CRYSTAL
        self.assertRaises(RuntimeError, Abins, AbInitioProgram="CRYSTAL", VibrationalOrPhononFile="MgO.wrong_out",
                          OutputWorkspace=self._workspace_name)

        # in case of molecular calculations AllKpointsGiven cannot be False
        self.assertRaises(RuntimeError, Abins, AbInitioProgram="CRYSTAL",
                          VibrationalOrPhononFile="toluene_molecule_BasicAbins.out",
                          AllKpointsGiven=False, OutputWorkspace=self._workspace_name)

        # no name for workspace
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._si2 + ".phonon",
                          TemperatureInKelvin=self._temperature)

        # keyword total in the name of the workspace
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._si2 + ".phonon",
                          TemperatureInKelvin=self._temperature, OutputWorkspace=self._workspace_name + "total")

        # negative temperature in K
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._si2 + ".phonon", TemperatureInKelvin=-1.0,
                          OutputWorkspace=self._workspace_name)

        # negative scale
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._si2 + ".phonon", Scale=-0.2,
                          OutputWorkspace=self._workspace_name)

    # test if intermediate results are consistent
    def test_non_unique_atoms(self):
        """Test scenario in which a user specifies non unique atoms (for example in squaricn that would be "C,C,H").
           In that case Abins should terminate and print a meaningful message.
        """
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._squaricn + ".phonon", Atoms="C,C,H",
                          OutputWorkspace=self._workspace_name)

    def test_non_existing_atoms(self):
        """Test scenario in which  a user requests to create workspaces for atoms which do not exist in the system.
           In that case Abins should terminate and give a user a meaningful message about wrong atoms to analyse.
        """
        # In _squaricn there is no C atoms
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._squaricn + ".phonon", Atoms="N",
                          OutputWorkspace=self._workspace_name)

    def test_scale(self):
        """
        Test if scaling is correct.
        @return:
        """
        wrk_ref = Abins(AbInitioProgram=self._ab_initio_program,
                        VibrationalOrPhononFile=self._squaricn + ".phonon",
                        TemperatureInKelvin=self._temperature,
                        SampleForm=self._sample_form,
                        Instrument=self._instrument_name,
                        Atoms=self._atoms,
                        Scale=self._scale,
                        SumContributions=self._sum_contributions,
                        QuantumOrderEventsNumber=self._quantum_order_events_number,
                        ScaleByCrossSection=self._cross_section_factor,
                        OutputWorkspace=self._squaricn + "_ref")

        wrk = Abins(AbInitioProgram=self._ab_initio_program,
                    VibrationalOrPhononFile=self._squaricn + ".phonon",
                    TemperatureInKelvin=self._temperature,
                    SampleForm=self._sample_form,
                    Instrument=self._instrument_name,
                    Atoms=self._atoms,
                    SumContributions=self._sum_contributions,
                    QuantumOrderEventsNumber=self._quantum_order_events_number,
                    Scale=10,
                    ScaleByCrossSection=self._cross_section_factor,
                    OutputWorkspace="squaricn_scale")

        ref = Scale(wrk_ref, Factor=10)

        (result, messages) = CompareWorkspaces(wrk, ref, Tolerance=self._tolerance)
        self.assertEqual(result, True)

    def test_exp(self):
        """
        Tests if experimental data is loaded correctly.
        @return:
        """
        Abins(AbInitioProgram=self._ab_initio_program,
              VibrationalOrPhononFile="benzene_Abins.phonon",
              ExperimentalFile="benzene_Abins.dat",
              TemperatureInKelvin=self._temperature,
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

        experimental_file = ""

        wrk_ref = Abins(AbInitioProgram=self._ab_initio_program,
                        VibrationalOrPhononFile=self._squaricn + ".phonon",
                        ExperimentalFile=experimental_file,
                        TemperatureInKelvin=self._temperature,
                        SampleForm=self._sample_form,
                        Instrument=self._instrument_name,
                        Atoms=self._atoms,
                        Scale=self._scale,
                        SumContributions=self._sum_contributions,
                        QuantumOrderEventsNumber=self._quantum_order_events_number,
                        ScaleByCrossSection=self._cross_section_factor,
                        OutputWorkspace=self._squaricn + "_ref")

        wks_all_atoms_explicitly = Abins(VibrationalOrPhononFile=self._squaricn + ".phonon",
                                         Atoms="H, C, O",
                                         SumContributions=self._sum_contributions,
                                         QuantumOrderEventsNumber=self._quantum_order_events_number,
                                         OutputWorkspace="explicit")

        wks_all_atoms_default = Abins(VibrationalOrPhononFile=self._squaricn + ".phonon",
                                      SumContributions=self._sum_contributions,
                                      QuantumOrderEventsNumber=self._quantum_order_events_number,
                                      OutputWorkspace="default")

        # Python 3 has no guarantee of dict order so the workspaces in the group may be in
        # a different order on Python 3
        self.assertEqual(wks_all_atoms_explicitly.size(), wks_all_atoms_default.size())
        explicit_names = wks_all_atoms_explicitly.getNames()
        for i in range(len(explicit_names)):
            explicit_name = explicit_names[i]
            default_name = "default" + explicit_name[8:]
            (result, messages) = CompareWorkspaces(explicit_name, default_name,
                                                   Tolerance=self._tolerance)
            self.assertEqual(result, True)

        self.assertEqual(wrk_ref.size(), wks_all_atoms_default.size())
        ref_names = wrk_ref.getNames()
        for i in range(len(ref_names)):
            ref_name = ref_names[i]
            default_name = "default" + ref_name[len(self._squaricn + "_ref"):]
            (result, messages) = CompareWorkspaces(ref_name, default_name,
                                                   Tolerance=self._tolerance)
            self.assertEqual(result, True)


if __name__ == "__main__":
    unittest.main()
