# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid import logger
from mantid.api import ITableWorkspace
from mantid.simpleapi import SimulatedDensityOfStates, CompareWorkspaces, Scale


def scipy_not_available():
    """Check whether scipy is available on this platform"""
    try:
        import scipy

        return False
    except ImportError:
        logger.warning("Skipping SimulatedDensityOfStatesTest because scipy is unavailable.")
        return True


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied functon returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """

    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and "test" in attr:
                    delattr(cls, attr)
        return cls

    return decorate


@skip_if(scipy_not_available)
class SimulatedDensityOfStatesTest(unittest.TestCase):
    def setUp(self):
        self._phonon_file = "squaricn.phonon"
        self._castep_file = "squaricn.castep"
        self._isotope_phonon = "test_isotopes.phonon"

    def test_phonon_load(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        self.assertEqual(wks.getNumberHistograms(), 2)
        v_axis_values = wks.getAxis(1).extractValues()
        self.assertEqual(v_axis_values[0], "Gaussian")
        self.assertEqual(v_axis_values[1], "Stick")

    def test_castep_load(self):
        wks = SimulatedDensityOfStates(CASTEPFile=self._castep_file)
        self.assertEqual(wks.getNumberHistograms(), 2)
        v_axis_values = wks.getAxis(1).extractValues()
        self.assertEqual(v_axis_values[0], "Gaussian")
        self.assertEqual(v_axis_values[1], "Stick")

    def test_raman_active(self):
        spec_type = "Raman_Active"
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type)
        self.assertEqual(wks.getNumberHistograms(), 2)
        v_axis_values = wks.getAxis(1).extractValues()
        self.assertEqual(v_axis_values[0], "Gaussian")
        self.assertEqual(v_axis_values[1], "Stick")

    def test_ir_active(self):
        spec_type = "IR_Active"
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type)
        self.assertEqual(wks.getNumberHistograms(), 2)
        v_axis_values = wks.getAxis(1).extractValues()
        self.assertEqual(v_axis_values[0], "Gaussian")
        self.assertEqual(v_axis_values[1], "Stick")

    def test_lorentzian_function(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, Function="Lorentzian")
        self.assertEqual(wks.getNumberHistograms(), 2)
        v_axis_values = wks.getAxis(1).extractValues()
        self.assertEqual(v_axis_values[0], "Lorentzian")
        self.assertEqual(v_axis_values[1], "Stick")

    def test_peak_width(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, PeakWidth="0.3")
        self.assertEqual(wks.getNumberHistograms(), 2)

    def test_peak_width_function(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, PeakWidth="0.1*energy")
        self.assertEqual(wks.getNumberHistograms(), 2)

    def test_peak_width_function_error(self):
        """
        Using an invalid peak width function should raise RuntimeError.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Invalid peak width function",
            SimulatedDensityOfStates,
            PHONONFile=self._phonon_file,
            PeakWidth="10*",
            OutputWorkspace="wks",
        )

    def test_temperature(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, Temperature=50)
        self.assertEqual(wks.getNumberHistograms(), 2)

    def test_scale(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, Scale=10)
        ref = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        ref = Scale(ref, Factor=10)

        self.assertTrue(CompareWorkspaces(wks, ref)[0])

    def test_bin_width(self):
        import math

        ref = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, BinWidth=2)

        size = wks.blocksize()
        ref_size = ref.blocksize()

        self.assertEqual(size, math.ceil(ref_size / 2.0))

    def test_zero_threshold(self):
        import numpy as np

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, ZeroThreshold=20)

        x_data = wks.readX(0)
        y_data = wks.readY(0)

        mask = np.where(x_data < 20)
        self.assertEqual(sum(y_data[mask]), 0)

    def test_partial(self):
        spec_type = "DOS"

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type, Ions="H,C,O")

        workspaces = wks.getNames()
        self.assertEqual(len(workspaces), 3)

    def test_sum_partial_contributions(self):
        spec_type = "DOS"
        tolerance = 1e-10

        summed = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type, Ions="H,C,O", SumContributions=True)
        total = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type)

        self.assertTrue(CompareWorkspaces(summed, total, tolerance)[0])

    def test_partial_cross_section_scale(self):
        spec_type = "DOS"

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type, Ions="H,C,O", ScaleByCrossSection="Incoherent")

        workspaces = wks.getNames()
        self.assertEqual(len(workspaces), 3)

    def test_sum_partial_contributions_cross_section_scale(self):
        spec_type = "DOS"
        tolerance = 1e-10

        summed = SimulatedDensityOfStates(
            PHONONFile=self._phonon_file, SpectrumType=spec_type, Ions="H,C,O", SumContributions=True, ScaleByCrossSection="Incoherent"
        )
        total = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType=spec_type, ScaleByCrossSection="Incoherent")

        self.assertTrue(CompareWorkspaces(summed, total, tolerance)[0])

    def test_ion_table(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType="IonTable")

        self.assertTrue(isinstance(wks, ITableWorkspace))
        self.assertEqual(wks.columnCount(), 10)
        self.assertEqual(wks.rowCount(), 20)

        all_species = wks.column("Species")

        self.assertEqual(all_species.count("H"), 4)
        self.assertEqual(all_species.count("C"), 8)
        self.assertEqual(all_species.count("O"), 8)

    def test_ion_table_castep_error(self):
        """
        Creating an ion table from a castep file is not possible and should fail
        validation.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot produce ion table when only .castep file is provided",
            SimulatedDensityOfStates,
            CASTEPFile=self._castep_file,
            SpectrumType="IonTable",
            OutputWorkspace="wks",
        )

    def test_bond_table(self):
        wks = SimulatedDensityOfStates(CASTEPFile=self._castep_file, SpectrumType="BondTable")

        self.assertTrue(isinstance(wks, ITableWorkspace))
        self.assertEqual(wks.columnCount(), 6)
        self.assertEqual(wks.rowCount(), 74)

    def test_bond_table_phonon_error(self):
        """
        Creating a bond table from a phonon file is not possible and should
        fail validation.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Require a .castep file for bond table output",
            SimulatedDensityOfStates,
            PHONONFile=self._phonon_file,
            SpectrumType="BondTable",
            OutputWorkspace="wks",
        )

    def test_bin_ranges_are_correct(self):
        """
        Test that the bin ranges are correct when draw peak function is called
        """
        bin_width = 3
        wks_grp = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType="DOS", BinWidth=bin_width, Ions="H,C,O")
        expected_x_min = -0.051481
        for idx in range(wks_grp.getNumberOfEntries()):
            ws = wks_grp.getItem(idx)
            self.assertAlmostEqual(expected_x_min, ws.readX(0)[0])
            self.assertAlmostEqual(bin_width, (ws.readX(0)[1] - ws.readX(0)[0]))

    def test_isotopes_are_parsed_correctly(self):
        """
        Test that isotopes in the file in the format `element:P` are valid
        """
        wks_grp = SimulatedDensityOfStates(PHONONFile=self._isotope_phonon, SpectrumType="DOS", Ions="H:P,C:P,O")
        self.assertEqual(3, wks_grp.size())
        self.assertTrue(_is_name_material_in_group(wks_group=wks_grp, name_to_find="wks_grp_C(13)", material_to_find="(C13)"))

    def test_non_isotpe_element_indices_loading(self):
        """
        Test that the individual indices for each element can be loaded seperately
        Tests parse_chemical_and_ws_name for non-isotope + indices
        """
        wks_grp = SimulatedDensityOfStates(PHONONFile=self._phonon_file, SpectrumType="DOS", CalculateIonIndices=True, Ions="H,C,O")
        self.assertEqual(20, len(wks_grp))
        self.assertTrue(_is_name_in_group(wks_group=wks_grp, name_to_find="wks_grp_H_0"))
        self.assertTrue(_is_name_in_group(wks_group=wks_grp, name_to_find="wks_grp_C_4"))
        self.assertTrue(_is_name_in_group(wks_group=wks_grp, name_to_find="wks_grp_O_12"))

    def test_isotope_element_indices_loading(self):
        """ """
        wks_grp = SimulatedDensityOfStates(PHONONFile=self._isotope_phonon, SpectrumType="DOS", CalculateIonIndices=True, Ions="H:P,C:P")
        self.assertEqual(2, len(wks_grp))
        self.assertTrue(_is_name_in_group(wks_group=wks_grp, name_to_find="wks_grp_C(13)_0"))
        self.assertTrue(_is_name_in_group(wks_group=wks_grp, name_to_find="wks_grp_H(2)_1"))


def _is_name_material_in_group(wks_group, name_to_find, material_to_find):
    """
    Checks all elements in a group as the order is not guaranteed in Python 3
    that both the name exist and the associated material is correct
    :param wks_group: The group to check all elements of
    :param name_to_find: The name to find in that group
    :param material_to_find: The correct material for that name
    :return: True is both parts are correct, else false
    """

    found_ws = _perform_group_name_search(wks_group, name_to_find)
    if found_ws is None:
        return False

    return found_ws.sample().getMaterial().name() == material_to_find


def _is_name_in_group(wks_group, name_to_find):
    """
    Checks that the name specified exists in the group specified.
    :param wks_group: The group to search for the name
    :param name_to_find: The name to search for
    :return: True if the name exists in the group, else false
    """

    found_ws = _perform_group_name_search(wks_group, name_to_find)
    return found_ws is not None


def _perform_group_name_search(wks_group, name_to_find):
    """
    Performs the search for a name in a group and returns
    the element if found
    :param wks_group: The group to search in
    :param name_to_find: The name to find
    :return: The first element with that name
    """

    for workspace in wks_group:
        if workspace.name() == name_to_find:
            return workspace


if __name__ == "__main__":
    unittest.main()
