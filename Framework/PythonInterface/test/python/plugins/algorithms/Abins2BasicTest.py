# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import logging
import tempfile
import unittest
from unittest.mock import patch
from numpy.testing import assert_array_almost_equal

import abins
import abins.io
from abins.abins2 import Abins as Abins2
from abins.constants import ATOM_PREFIX

# Required to register Abins2 before importing any mantid modules
Abins2.subscribe()

from mantid.simpleapi import mtd, CompareWorkspaces, Abins  # noqa: E402


class AbinsBasicTest(unittest.TestCase):
    # Define typical values for parameters
    _si2 = "Si2-sc_Abins"
    _squaricn = "squaricn_sum_Abins"
    _ab_initio_program = "CASTEP"
    _temperature = 10.0  # temperature 10 K
    _instrument_name = "TOSCA"
    _atoms = ""  # if no atoms are specified then all atoms are taken into account
    _sum_contributions = True
    _quantum_order_events_number = "1"

    _cross_section_factor = "Incoherent"
    _workspace_name = "output_workspace"

    # Test-related parameters
    _tolerance = 0.0001
    _tmp_cache_dir = None

    def get_cache_dir(self):
        return self._tmp_cache_dir.name

    @classmethod
    def setUpClass(cls):
        cls._tmp_cache_dir = tempfile.TemporaryDirectory()

    @classmethod
    def tearDownClass(cls):
        cls._tmp_cache_dir.cleanup()

    def setUp(self):
        self.logger = logging.getLogger("abins2-basic-test")

    def tearDown(self):
        abins.test_helpers.remove_output_files(
            list_of_names=[
                "explicit",
                "default",
                "total",
                "squaricn_sum_Abins",
                "benzene_exp",
                "benzene_Abins",
                "numbered",
            ]
        )
        mtd.clear()

    def test_subscribe_warning(self):
        """Test that subscribing work-in-progress algorithm raises warning"""
        with self.assertLogs(logger=self.logger, level="WARNING") as log_context:
            Abins2.subscribe(logger=self.logger)

            self.assertIn("breaking changes", log_context.output[0])

    def test_wrong_input(self):
        """Test if the correct behaviour of algorithm in case input is not valid"""

        #  invalid CASTEP file missing:  Number of branches     6 in the header file

        with (
            self.assertRaisesRegex(RuntimeError, "The third line should include 'Number of branches'."),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(Version=2, VibrationalOrPhononFile="Si2-sc_wrong.phonon", OutputWorkspace=self._workspace_name)

        # wrong extension of phonon file in case of CASTEP
        with (
            self.assertRaisesRegex(
                RuntimeError,
                "The expected extension of file is .phonon.",
            ),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile="Si2-sc.wrong_phonon",
                OutputWorkspace=self._workspace_name,
            )

        # wrong extension of phonon file in case of CRYSTAL
        with (
            self.assertRaisesRegex(
                RuntimeError,
                "The expected extension of file is .out.",
            ),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                AbInitioProgram="CRYSTAL",
                VibrationalOrPhononFile="MgO.wrong_out",
                OutputWorkspace=self._workspace_name,
            )

        # no name for workspace
        with self.assertRaises(TypeError), patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            Abins(Version=2, VibrationalOrPhononFile=self._si2 + ".phonon", TemperatureInKelvin=self._temperature)

        # keyword total in the name of the workspace
        with (
            self.assertRaisesRegex(
                RuntimeError,
                "Keyword: total cannot be used in the name of workspace.",
            ),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._si2 + ".phonon",
                TemperatureInKelvin=self._temperature,
                OutputWorkspace=self._workspace_name + "total",
            )

        # non-existent parameter
        with (
            self.assertRaisesRegex(TypeError, "'SampleForm' is an invalid keyword argument"),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._si2 + ".phonon",
                OutputWorkspace=self._workspace_name,
                SampleForm="Powder",
            )

        # negative temperature in K
        with (
            self.assertRaisesRegex(
                RuntimeError,
                "Temperature must be positive.",
            ),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._si2 + ".phonon",
                TemperatureInKelvin=-1.0,
                OutputWorkspace=self._workspace_name,
            )

        # unknown instrument
        with (
            self.assertRaisesRegex(ValueError, 'The value "UnknownInstrument" is not in the list of allowed values'),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._si2 + ".phonon",
                Instrument="UnknownInstrument",
                OutputWorkspace=self._workspace_name,
            )

    # test if intermediate results are consistent
    def test_non_unique_elements(self):
        """Test scenario in which a user specifies non-unique elements (for example in squaricn that would be "C,C,H").
        In that case Abins should terminate and print a meaningful message.
        """
        with (
            self.assertRaisesRegex(RuntimeError, r"User atom selection \(by symbol\) contains repeated species."),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms="C,C,H",
                OutputWorkspace=self._workspace_name,
            )

    def test_non_unique_atoms(self):
        """Test scenario in which a user specifies non-unique atoms (for example "atom_1,atom_2,atom1").
        In that case Abins should terminate and print a meaningful message.
        """
        with (
            self.assertRaisesRegex(RuntimeError, r"User atom selection \(by number\) contains repeated atom."),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms="atom_1,atom_2,atom1",
                OutputWorkspace=self._workspace_name,
            )

    def test_non_existing_atoms(self):
        """Test scenario in which  a user requests to create workspaces for atoms which do not exist in the system.
        In that case Abins should terminate and give a user a meaningful message about wrong atoms to analyse.
        """
        # In _squaricn there are no N atoms
        with (
            self.assertRaisesRegex(RuntimeError, r"User defined atom selection \(by element\) 'N': not present in the system."),
            patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir),
        ):
            Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms="N",
                OutputWorkspace=self._workspace_name,
            )

    def test_atom_index_limits(self):
        """Individual atoms may be indexed (counting from 1); if the index falls outside number of atoms, Abins should
        terminate with a useful error message.
        """
        with patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            self.assertRaisesRegex(
                RuntimeError,
                r"Invalid user atom selection \(by number\)" + f" '{ATOM_PREFIX}0'",
                Abins,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=ATOM_PREFIX + "0",
                OutputWorkspace=self._workspace_name,
            )
            self.assertRaisesRegex(
                RuntimeError,
                r"Invalid user atom selection \(by number\)" + f" '{ATOM_PREFIX}61'",
                Abins,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=ATOM_PREFIX + "61",
                OutputWorkspace=self._workspace_name,
            )

    def test_atom_index_invalid(self):
        r"""If the atoms field includes an unmatched entry (i.e. containing the prefix but not matching the '\d+' regex,
        Abins should terminate with a useful error message.
        """
        with patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            self.assertRaisesRegex(
                RuntimeError,
                r"Not all user atom selections \('atoms' option\) were understood.",
                Abins,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=ATOM_PREFIX + "-3",
                OutputWorkspace=self._workspace_name,
            )
            self.assertRaisesRegex(
                RuntimeError,
                r"Not all user atom selections \('atoms' option\) were understood.",
                Abins,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=ATOM_PREFIX + "_#4",
                OutputWorkspace=self._workspace_name,
            )

    def test_lagrange_exists(self):
        with patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            Abins(
                Version=2,
                AbInitioProgram=self._ab_initio_program,
                VibrationalOrPhononFile=(self._squaricn + ".phonon"),
                TemperatureInKelvin=self._temperature,
                Instrument="Lagrange",
                Setting="Cu(331) (Lagrange)",
                Atoms=self._atoms,
                SumContributions=self._sum_contributions,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                ScaleByCrossSection=self._cross_section_factor,
                OutputWorkspace="squaricn-lagrange",
            )

    def test_partial_by_element(self):
        """Check results of INS spectrum resolved by elements: default should match explicit list of elements"""

        with patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            wrk_ref = Abins(
                Version=2,
                AbInitioProgram=self._ab_initio_program,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                TemperatureInKelvin=self._temperature,
                Instrument=self._instrument_name,
                Atoms=self._atoms,
                SumContributions=self._sum_contributions,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                ScaleByCrossSection=self._cross_section_factor,
                OutputWorkspace=self._squaricn + "_ref",
            )

            wks_all_atoms_explicitly = Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms="H, C, O",
                SumContributions=self._sum_contributions,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                OutputWorkspace="explicit",
            )

            wks_all_atoms_default = Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                SumContributions=self._sum_contributions,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                OutputWorkspace="default",
            )

        # Python 3 has no guarantee of dict order so the workspaces in the group may be in
        # a different order on Python 3
        self.assertEqual(wks_all_atoms_explicitly.size(), wks_all_atoms_default.size())
        explicit_names = wks_all_atoms_explicitly.getNames()
        for i in range(len(explicit_names)):
            explicit_name = explicit_names[i]
            default_name = "default" + explicit_name[8:]
            (result, messages) = CompareWorkspaces(explicit_name, default_name, Tolerance=self._tolerance)
            self.assertEqual(result, True)

        self.assertEqual(wrk_ref.size(), wks_all_atoms_default.size())
        ref_names = wrk_ref.getNames()
        for i in range(len(ref_names)):
            ref_name = ref_names[i]
            default_name = "default" + ref_name[len(self._squaricn + "_ref") :]
            (result, messages) = CompareWorkspaces(ref_name, default_name, Tolerance=self._tolerance)
            self.assertEqual(result, True)

    def test_partial_by_number(self):
        """Simulated INS spectrum can also be resolved by numbered atoms. Check consistency with element totals"""
        with patch.object(abins.io.IO, "get_save_dir_path", side_effect=self.get_cache_dir):
            wrk_ref = Abins(
                Version=2,
                AbInitioProgram=self._ab_initio_program,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=self._atoms,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                ScaleByCrossSection=self._cross_section_factor,
                OutputWorkspace=self._squaricn + "_ref",
            )

            numbered_workspace_name = "numbered"
            h_indices = ("1", "2", "3", "4")
            wks_numbered_atoms = Abins(
                Version=2,
                VibrationalOrPhononFile=self._squaricn + ".phonon",
                Atoms=", ".join([ATOM_PREFIX + s for s in h_indices]),
                SumContributions=self._sum_contributions,
                QuantumOrderEventsNumber=self._quantum_order_events_number,
                ScaleByCrossSection=self._cross_section_factor,
                OutputWorkspace=numbered_workspace_name,
            )

        wrk_ref_names = list(wrk_ref.getNames())
        wrk_h_total = wrk_ref[wrk_ref_names.index(self._squaricn + "_ref_H_total")]

        wks_numbered_atom_names = list(wks_numbered_atoms.getNames())
        wrk_atom_totals = [
            wks_numbered_atoms[wks_numbered_atom_names.index(name)]
            for name in ["_".join((numbered_workspace_name, ATOM_PREFIX, s, "total")) for s in h_indices]
        ]

        assert_array_almost_equal(wrk_h_total.extractX(), wrk_atom_totals[0].extractX())

        assert_array_almost_equal(wrk_h_total.extractY(), sum((wrk.extractY() for wrk in wrk_atom_totals)))


if __name__ == "__main__":
    unittest.main()
