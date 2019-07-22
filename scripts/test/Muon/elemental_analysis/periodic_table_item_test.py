from __future__ import print_function, absolute_import


import unittest

from mantid.py3compat import mock

import Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table as periodic_table
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem,\
    ColoredPeriodicTableItem, _ElementButton, PeriodicTable, PeriodicCombo, PeriodicList


class PeriodicTableItemTest(unittest.TestCase):
    def setUp(self):
        self.element = PeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")

    def test_that_list_elements_contains_all(self):
        for i in range(len(periodic_table._elements)-1):
            self.assertTrue(isinstance(periodic_table._elements[i][0], str))
            self.assertTrue(isinstance(periodic_table._elements[i][1], int))
            self.assertTrue(isinstance(periodic_table._elements[i][2], int))
            self.assertTrue(isinstance(periodic_table._elements[i][3], int))
            self.assertTrue(isinstance(periodic_table._elements[i][4], str))
            self.assertTrue(isinstance(periodic_table._elements[i][5], float)
                            or isinstance(periodic_table._elements[i][5], int))
            self.assertTrue(isinstance(periodic_table._elements[i][6], str))

        # Meitnerium is not confirmed to be a transition metal, hence it lacks the subcategory field (ie. field 6)
        i = len(periodic_table._elements)-1
        self.assertTrue(isinstance(periodic_table._elements[i][0], str))
        self.assertTrue(isinstance(periodic_table._elements[i][1], int))
        self.assertTrue(isinstance(periodic_table._elements[i][2], int))
        self.assertTrue(isinstance(periodic_table._elements[i][3], int))
        self.assertTrue(isinstance(periodic_table._elements[i][4], str))
        self.assertTrue(isinstance(periodic_table._elements[i][5], float)
                        or isinstance(periodic_table._elements[i][5], int))

    def test_that_get_method_works(self):
        expected = ["Ti", 22, 4, 4, "titanium", 47.9000]
        returned = [self.element[i] for i in range(6)]

        self.assertEqual(expected, returned)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table._logger.warning')
    def test_that_density_returns_a_warning(self, mock_warning):
        self.assertEqual(self.element[6], 0.0)
        self.assertEqual(mock_warning.call_count, 1)

    def test_that_length_gives_correct_value(self):
        self.assertEqual(self.element.__len__(), 6)


class ColoredPeriodicTableItemTest(unittest.TestCase):
    def setUp(self):
        self.coloured = ColoredPeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")
        self.custom = ColoredPeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal", '#ABCDEF')

    def test_that_automatic_color_is_initialized(self):
        self.assertEqual(self.coloured.bgcolor, "#FFA07A")

    def test_that_custom_colors_are_allowed(self):
        self.assertEqual(self.custom.bgcolor, '#ABCDEF')


if __name__ == '__main__':
    unittest.main()























