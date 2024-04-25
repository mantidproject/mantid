# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-public-methods,invalid-name,protected-access
import unittest
from testhelpers import assertRaisesNothing

from LoadCIF import SpaceGroupBuilder, UnitCellBuilder, AtomListBuilder, UBMatrixBuilder, CrystalStructureBuilder

from mantid.api import AlgorithmFactory
from mantid.geometry import UnitCell

import numpy as np
import sys


def merge_dicts(lhs, rhs):
    merged = lhs.copy()
    merged.update(rhs)

    return merged


class SpaceGroupBuilderTest(unittest.TestCase):
    def setUp(self):
        self.builder = SpaceGroupBuilder()

    def test_getSpaceGroupFromString_valid_no_exceptions(self):
        valid_new = {"_space_group_name_h-m_alt": "P m -3 m"}
        valid_old = {"_symmetry_space_group_name_h-m": "P m -3 m"}

        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=valid_old)
        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=valid_new)
        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=merge_dicts(valid_new, valid_old))

    def test_getSpaceGroupFromString_valid_correct_value(self):
        valid_new = {"_space_group_name_h-m_alt": "P m -3 m"}
        valid_old = {"_symmetry_space_group_name_h-m": "P m -3 m"}
        valid_old_different = {"_symmetry_space_group_name_h-m": "F d d d"}
        invalid_old = {"_symmetry_space_group_name_h-m": "invalid"}

        self.assertEqual(self.builder._getSpaceGroupFromString(valid_new), "P m -3 m")
        self.assertEqual(self.builder._getSpaceGroupFromString(valid_old), "P m -3 m")
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, valid_old)), "P m -3 m")
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, valid_old_different)), "P m -3 m")
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, invalid_old)), "P m -3 m")

    def test_getSpaceGroupFromString_valid_correct_add_inversion(self):
        self.assertEqual(self.builder._getSpaceGroupFromString({"_space_group_name_h-m_alt": "F m 3 m"}), "F m -3 m")

    def test_getSpaceGroupFromString_invalid(self):
        valid_old = {"_symmetry_space_group_name_h-m": "P m -3 m"}
        invalid_new = {"_space_group_name_h-m_alt": "invalid"}
        invalid_old = {"_symmetry_space_group_name_h-m": "invalid"}

        self.assertRaisesRegex(RuntimeError, "No space group symbol in CIF.", self.builder._getSpaceGroupFromString, cifData={})
        self.assertRaisesRegex(
            ValueError, "Space group with symbol 'invalid' is not registered.", self.builder._getSpaceGroupFromString, cifData=invalid_new
        )
        self.assertRaisesRegex(
            ValueError, "Space group with symbol 'invalid' is not registered.", self.builder._getSpaceGroupFromString, cifData=invalid_old
        )
        self.assertRaisesRegex(
            ValueError,
            "Space group with symbol 'invalid' is not registered.",
            self.builder._getSpaceGroupFromString,
            cifData=merge_dicts(invalid_new, valid_old),
        )

    def test_getCleanSpaceGroupSymbol(self):
        fn = self.builder._getCleanSpaceGroupSymbol

        self.assertEqual(fn("P m -3 m :1"), "P m -3 m")
        self.assertEqual(fn("P m -3 m :H"), "P m -3 m")
        self.assertEqual(fn("F d -3 m S"), "F d -3 m")
        self.assertEqual(fn("F d -3 m Z"), "F d -3 m :2")
        self.assertEqual(fn("R 3 H"), "R 3")
        self.assertEqual(fn("R 3 R"), "R 3 :r")

    def test_getSpaceGroupFromNumber_invalid(self):
        invalid_old = {"_symmetry_int_tables_number": "400"}
        invalid_new = {"_space_group_it_number": "400"}

        self.assertRaisesRegex(RuntimeError, "No space group symbol in CIF.", self.builder._getSpaceGroupFromNumber, cifData={})
        self.assertRaisesRegex(
            RuntimeError,
            r"Can not use space group number to determine space group for no. \[400\]",
            self.builder._getSpaceGroupFromNumber,
            cifData=invalid_old,
        )
        self.assertRaisesRegex(
            RuntimeError,
            r"Can not use space group number to determine space group for no. \[400\]",
            self.builder._getSpaceGroupFromNumber,
            cifData=invalid_new,
        )


class UnitCellBuilderTest(unittest.TestCase):
    def setUp(self):
        self.builder = UnitCellBuilder()

    def test_getUnitCell_invalid(self):
        invalid_no_a = {"_cell_length_b": "5.6"}
        self.assertRaisesRegex(
            RuntimeError, "The a-parameter of the unit cell is not specified in the supplied CIF.", self.builder._getUnitCell, cifData={}
        )
        self.assertRaisesRegex(
            RuntimeError,
            "The a-parameter of the unit cell is not specified in the supplied CIF.",
            self.builder._getUnitCell,
            cifData=invalid_no_a,
        )

    def test_getUnitCell_cubic(self):
        cell = {"_cell_length_a": "5.6"}

        self.assertEqual(self.builder._getUnitCell(cell), "5.6 5.6 5.6 90.0 90.0 90.0")

    def test_getUnitCell_tetragonal(self):
        cell = {"_cell_length_a": "5.6", "_cell_length_c": "2.3"}

        self.assertEqual(self.builder._getUnitCell(cell), "5.6 5.6 2.3 90.0 90.0 90.0")

    def test_getUnitCell_orthorhombic(self):
        cell = {"_cell_length_a": "5.6", "_cell_length_b": "1.6", "_cell_length_c": "2.3"}

        self.assertEqual(self.builder._getUnitCell(cell), "5.6 1.6 2.3 90.0 90.0 90.0")

    def test_getUnitCell_hexagonal(self):
        cell = {"_cell_length_a": "5.6", "_cell_length_c": "2.3", "_cell_angle_gamma": "120.0"}
        cell_errors = {"_cell_length_a": "5.6(1)", "_cell_length_c": "2.3(1)", "_cell_angle_gamma": "120.0"}

        self.assertEqual(self.builder._getUnitCell(cell), "5.6 5.6 2.3 90.0 90.0 120.0")
        self.assertEqual(self.builder._getUnitCell(cell_errors), "5.6 5.6 2.3 90.0 90.0 120.0")


class AtomListBuilderTest(unittest.TestCase):
    def setUp(self):
        self.builder = AtomListBuilder()
        self._baseData = dict(
            [
                ("_atom_site_fract_x", ["1/8", "0.34(1)"]),
                ("_atom_site_fract_y", ["1/8", "0.56(2)"]),
                ("_atom_site_fract_z", ["1/8", "0.23(2)"]),
            ]
        )

    def _getData(self, additionalData):
        data = self._baseData.copy()
        data.update(additionalData)

        return data

    def test_getAtoms_required_keys(self):
        mandatoryKeys = dict(
            [
                ("_atom_site_label", ["Si"]),
                ("_atom_site_fract_x", ["1/8"]),
                ("_atom_site_fract_y", ["1/8"]),
                ("_atom_site_fract_z", ["1/8"]),
            ]
        )
        expected_error_messages = {
            "_atom_site_label": "Cannot determine atom types",
            "_atom_site_fract_x": "Mandatory field _atom_site_fract_x not found in CIF-file.",
            "_atom_site_fract_y": "Mandatory field _atom_site_fract_y not found in CIF-file.",
            "_atom_site_fract_z": "Mandatory field _atom_site_fract_z not found in CIF-file.",
        }

        for key in mandatoryKeys:
            tmp = mandatoryKeys.copy()
            del tmp[key]
            self.assertRaisesRegex(RuntimeError, expected_error_messages[key], self.builder._getAtoms, cifData=tmp)

    def test_getAtoms_correct(self):
        data = self._getData(
            dict(
                [
                    ("_atom_site_label", ["Si", "Al"]),
                    ("_atom_site_occupancy", ["0.6", "0.4(0)"]),
                    ("_atom_site_u_iso_or_equiv", ["0.01", "0.02"]),
                ]
            )
        )

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 0.6 0.01;Al 0.34 0.56 0.23 0.4 0.02")

    def test_getAtoms_atom_type_symbol(self):
        data = self._getData(
            dict(
                [
                    ("_atom_site_label", ["Fake1", "Fake2"]),
                    ("_atom_site_occupancy", ["1.0", "1.0(0)"]),
                    ("_atom_site_u_iso_or_equiv", ["0.01", "0.02"]),
                ]
            )
        )

        self.assertEqual(self.builder._getAtoms(data), "Fake 1/8 1/8 1/8 1.0 0.01;Fake 0.34 0.56 0.23 1.0 0.02")

        del data["_atom_site_label"]
        data["_atom_site_type_symbol"] = ["Si", "Al"]

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02")

    def test_getAtoms_B_iso(self):
        data = self._getData(
            dict(
                [
                    ("_atom_site_label", ["Si", "Al"]),
                    ("_atom_site_occupancy", ["1.0", "1.0(0)"]),
                    ("_atom_site_b_iso_or_equiv", ["1.0", "2.0"]),
                ]
            )
        )

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0 0.012665147955292222;Al 0.34 0.56 0.23 1.0 0.025330295910584444")

    def test_getAtoms_no_occupancy(self):
        data = self._getData(dict([("_atom_site_label", ["Si", "Al"]), ("_atom_site_u_iso_or_equiv", ["0.01", "0.02"])]))

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02")

    def test_getAtoms_no_u_or_b(self):
        data = self._getData(dict([("_atom_site_label", ["Si", "Al"]), ("_atom_site_occupancy", ["1.0", "1.0(0)"])]))

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0;Al 0.34 0.56 0.23 1.0")

    def test_getAtoms_invalid_u(self):
        data = self._getData(
            dict(
                [
                    ("_atom_site_label", ["Si", "Al"]),
                    ("_atom_site_occupancy", ["1.0", "1.0(0)"]),
                    ("_atom_site_u_iso_or_equiv", ["0.01", "sdfsdfs"]),
                ]
            )
        )

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0")

    def test_getAtoms_invalid_b(self):
        data = self._getData(
            dict(
                [
                    ("_atom_site_label", ["Si", "Al"]),
                    ("_atom_site_occupancy", ["1.0", "1.0(0)"]),
                    ("_atom_site_b_iso_or_equiv", ["1.0", "sdfsdfs"]),
                ]
            )
        )

        self.assertEqual(self.builder._getAtoms(data), "Si 1/8 1/8 1/8 1.0 0.012665147955292222;Al 0.34 0.56 0.23 1.0")

    def test_getAtoms_aniso_u_orthogonal(self):
        uElements = {
            "11": ["0.01", "0.02"],
            "12": ["0.0", "0.0"],
            "13": ["0.0", "0.0"],
            "22": ["0.01", "0.02"],
            "23": ["0.0", "0.0"],
            "33": ["0.04", "0.05"],
        }

        uDict = dict([("_atom_site_aniso_u_{0}".format(key), value) for key, value in uElements.items()])
        uDict.update(dict([("_atom_site_label", ["Si", "Al"]), ("_atom_site_aniso_label", ["Si", "Al"])]))

        data = self._getData(uDict)
        cell = UnitCell(5.4, 4.3, 3.2)

        # u_equiv should be (0.01 + 0.01 + 0.04)/3 = 0.02 and (0.2 + 0.2 + 0.5)/3 = 0.03
        self.assertEqual(self.builder._getAtoms(data, cell), "Si 1/8 1/8 1/8 1.0 0.02;Al 0.34 0.56 0.23 1.0 0.03")

    def test_getAtoms_aniso_u_hexagonal(self):
        uElements = {
            "11": ["0.01", "0.02"],
            "12": ["0.01", "0.01"],
            "13": ["0.0", "0.0"],
            "22": ["0.01", "0.02"],
            "23": ["0.0", "0.0"],
            "33": ["0.04", "0.05"],
        }

        uDict = dict([("_atom_site_aniso_u_{0}".format(key), value) for key, value in uElements.items()])
        uDict.update(dict([("_atom_site_label", ["Si", "Al"]), ("_atom_site_aniso_label", ["Si", "Al"])]))

        data = self._getData(uDict)
        cell = UnitCell(5.4, 4.3, 3.2, 90, 90, 120)

        # u_equiv should be (4/3*(0.01 + 0.01 - 0.01) + 0.04)/3 = 0.0177... and (4/3*(0.02 + 0.02 - 0.01) + 0.05)/3 = 0.03
        self.assertEqual(self.builder._getAtoms(data, cell), "Si 1/8 1/8 1/8 1.0 0.01778;Al 0.34 0.56 0.23 1.0 0.03")

    def test_getAtoms_aniso_b_orthogonal(self):
        bElements = {
            "11": ["1.0", "2.0"],
            "12": ["0.0", "0.0"],
            "13": ["0.0", "0.0"],
            "22": ["1.0", "2.0"],
            "23": ["0.0", "0.0"],
            "33": ["4.0", "5.0"],
        }

        bDict = dict([("_atom_site_aniso_b_{0}".format(key), value) for key, value in bElements.items()])
        bDict.update(dict([("_atom_site_label", ["Si", "Al"]), ("_atom_site_aniso_label", ["Si", "Al"])]))

        data = self._getData(bDict)
        cell = UnitCell(5.4, 4.3, 3.2)

        self.assertEqual(self.builder._getAtoms(data, cell), "Si 1/8 1/8 1/8 1.0 0.02533;Al 0.34 0.56 0.23 1.0 0.038")

    def test_getAtoms_aniso_iso_mixed(self):
        uElements = {"11": ["0.01"], "12": ["0.0"], "13": ["0.0"], "22": ["0.01"], "23": ["0.0"], "33": ["0.04"]}

        uDict = dict([("_atom_site_aniso_u_{0}".format(key), value) for key, value in uElements.items()])
        uDict.update(
            dict(
                [("_atom_site_label", ["Si", "Al"]), ("_atom_site_aniso_label", ["Al"]), ("_atom_site_u_iso_or_equiv", ["0.01", "invalid"])]
            )
        )

        data = self._getData(uDict)
        cell = UnitCell(5.4, 4.3, 3.2)

        self.assertEqual(self.builder._getAtoms(data, cell), "Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02")

    def test_getAtoms_iso_preferred(self):
        uElements = {
            "11": ["0.01", "0.02"],
            "12": ["0.0", "0.0"],
            "13": ["0.0", "0.0"],
            "22": ["0.01", "0.02"],
            "23": ["0.0", "0.0"],
            "33": ["0.04", "0.05"],
        }

        uDict = dict([("_atom_site_aniso_u_{0}".format(key), value) for key, value in uElements.items()])
        uDict.update(
            dict(
                [
                    ("_atom_site_label", ["Si", "Al"]),
                    ("_atom_site_aniso_label", ["Si", "Al"]),
                    ("_atom_site_u_iso_or_equiv", ["0.01", "0.02"]),
                ]
            )
        )

        data = self._getData(uDict)
        cell = UnitCell(5.4, 4.3, 3.2)

        self.assertEqual(self.builder._getAtoms(data, cell), "Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02")

    def test_getReciprocalLengthMatrix(self):
        cell = UnitCell(1, 2, 3)

        matrix = self.builder._getReciprocalLengthSquaredMatrix(cell)

        expected = np.array(
            [
                [(1.0 / 1.0 * 1.0 / 1.0), (1.0 / 1.0 * 1.0 / 2.0), (1.0 / 1.0 * 1.0 / 3.0)],
                [(1.0 / 2.0 * 1.0 / 1.0), (1.0 / 2.0 * 1.0 / 2.0), (1.0 / 2.0 * 1.0 / 3.0)],
                [(1.0 / 3.0 * 1.0 / 1.0), (1.0 / 3.0 * 1.0 / 2.0), (1.0 / 3.0 * 1.0 / 3.0)],
            ]
        )

        self.assertTrue(np.all(matrix == expected))

    def test_getSumWeights_orthorhombic(self):
        cell = UnitCell(1, 2, 3, 90, 90, 90)

        matrix = self.builder._getMetricDependentWeights(cell)
        expected = np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])

        self.assertTrue(np.all(np.abs(matrix - expected) < 1.0e-9))

    def test_getSumWeights_hexagonal(self):
        cell = UnitCell(2, 2, 3, 90, 90, 120)

        matrix = self.builder._getMetricDependentWeights(cell)
        expected = np.array([[4.0 / 3.0, -2.0 / 3.0, 0.0], [-2.0 / 3.0, 4.0 / 3.0, 0.0], [0.0, 0.0, 1.0]])

        self.assertTrue(np.all(np.abs(matrix - expected) < 1.0e-9))


class UBMatrixBuilderTest(unittest.TestCase):
    def setUp(self):
        self.builder = UBMatrixBuilder()
        self.valid_matrix = {
            "_diffrn_orient_matrix_ub_11": "-0.03",
            "_diffrn_orient_matrix_ub_12": "0.13",
            "_diffrn_orient_matrix_ub_13": "0.31",
            "_diffrn_orient_matrix_ub_21": "0.01",
            "_diffrn_orient_matrix_ub_22": "-0.31",
            "_diffrn_orient_matrix_ub_23": "0.14",
            "_diffrn_orient_matrix_ub_31": "0.34",
            "_diffrn_orient_matrix_ub_32": "0.02",
            "_diffrn_orient_matrix_ub_33": "0.02",
        }

    def test_getUBMatrix_invalid(self):
        for key in self.valid_matrix:
            tmp = self.valid_matrix.copy()
            del tmp[key]

            self.assertRaisesRegex(
                RuntimeError, "Can not load UB matrix from CIF, values are missing.", self.builder._getUBMatrix, cifData=tmp
            )

    def test_getUBMatrix_correct(self):
        self.assertEqual(self.builder._getUBMatrix(self.valid_matrix), "-0.03,0.13,0.31,0.01,-0.31,0.14,0.34,0.02,0.02")


if __name__ == "__main__":
    # Only test if algorithm is registered (PyCifRW dependency).
    if AlgorithmFactory.exists("LoadCIF"):
        unittest.main()
