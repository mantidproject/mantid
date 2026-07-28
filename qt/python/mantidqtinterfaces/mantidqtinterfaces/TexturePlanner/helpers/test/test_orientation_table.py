# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
import numpy as np

from unittest.mock import patch, call
from scipy.spatial.transform import Rotation

from mantidqtinterfaces.TexturePlanner.helpers.orientation_table import (
    MAX_GONIOMETERS,
    Orientation,
    OrientationTable,
    _DEFAULT_GONIO_STRING,
)

FILE_PATH = "mantidqtinterfaces.TexturePlanner.helpers.orientation_table"


def _make_table(axes="xyz", senses="1,1,1"):
    table = OrientationTable()
    table.orientation_kwargs = {"Axes": axes, "Senses": senses}
    return table


class TestOrientation_Copy(unittest.TestCase):
    def test_copy_returns_independent_lists(self):
        original = Orientation(
            gonio_strings=["a", "b", "c"],
            gRs=[Rotation.identity()],
            R=Rotation.identity(),
            include=False,
            select=True,
            pf_points=np.array([1.0, 2.0]),
            transmission=np.array([0.5]),
        )

        clone = original.copy()
        clone.gonio_strings[0] = "modified"
        clone.gRs.append(Rotation.identity())

        self.assertEqual(original.gonio_strings, ["a", "b", "c"])
        self.assertEqual(len(original.gRs), 1)

    def test_copy_preserves_scalar_and_array_fields(self):
        pf = np.array([1.0, 2.0])
        original = Orientation(include=False, select=True, pf_points=pf, transmission=None)

        clone = original.copy()

        self.assertFalse(clone.include)
        self.assertTrue(clone.select)
        # we want the arrays to carry the same values but be different objects
        np.testing.assert_array_equal(clone.pf_points, pf)
        self.assertIsNot(clone.pf_points, pf)
        self.assertIsNone(clone.transmission)


class TestOrientationTable_Init(unittest.TestCase):
    def test_default_attributes(self):
        tab = _make_table()

        self.assertEqual(tab.sense_vals, {"Clockwise": -1, "Counterclockwise": 1})
        self.assertEqual(tab.sense_names, {"-1": "Clockwise", "1": "Counterclockwise"})
        self.assertEqual(tab.axis_dict, {"x": (1, 0, 0), "y": (0, 1, 0), "z": (0, 0, 1)})
        self.assertEqual(tab.n_gonio, 2)
        self.assertEqual(tab.orientation_index, 0)

    def test_starts_with_one_default_orientation(self):
        tab = _make_table()

        self.assertEqual(list(tab.saved_orientations.keys()), [0])
        self.assertIsInstance(tab.saved_orientations[0], Orientation)


class TestOrientationTable_DictAccess(unittest.TestCase):
    def test_getitem_returns_saved_orientation(self):
        tab = _make_table()
        self.assertIs(tab[0], tab.saved_orientations[0])

    def test_keys_values_items(self):
        tab = _make_table()
        self.assertEqual(list(tab.keys()), [0])
        self.assertEqual(list(tab.values()), [tab.saved_orientations[0]])
        self.assertEqual(list(tab.items()), [(0, tab.saved_orientations[0])])


class TestOrientationTable_IndexAndCount(unittest.TestCase):
    def test_get_and_set_orientation_index(self):
        tab = _make_table()
        tab.set_orientation_index(5)
        self.assertEqual(tab.get_orientation_index(), 5)

    def test_get_num_orientations(self):
        tab = _make_table()
        self.assertEqual(tab.get_num_orientations(), 1)
        tab.add_orientation()
        self.assertEqual(tab.get_num_orientations(), 2)

    def test_set_n_gonio(self):
        tab = _make_table()
        tab.set_n_gonio(4)
        self.assertEqual(tab.n_gonio, 4)


class TestOrientationTable_GetTableInfo(unittest.TestCase):
    def test_returns_gonio_strings_include_select_per_orientation(self):
        tab = _make_table()
        tab.add_orientation()
        tab[0].include = True
        tab[0].select = False
        tab[1].include = False
        tab[1].select = True

        info = tab.get_table_info()

        self.assertEqual(len(info), 2)
        self.assertEqual(info[0][1:], [True, False])
        self.assertEqual(info[1][1:], [False, True])
        self.assertEqual(info[0][0], list(tab[0].gonio_strings))


class TestOrientationTable_SetTransmissionAtIndex(unittest.TestCase):
    def test_assigns_transmission_to_correct_orientation(self):
        tab = _make_table()
        tab.add_orientation()
        transmission = np.array([0.1, 0.2, 0.3])

        tab.set_transmission_at_index(transmission, 1)

        np.testing.assert_array_equal(tab[1].transmission, transmission)
        self.assertIsNone(tab[0].transmission)


class TestOrientationTable_CalcGRs(unittest.TestCase):
    def test_returns_identity_for_zero_angles(self):
        tab = _make_table()

        gRs, R = tab.calc_gRs([(1, 0, 0), (0, 1, 0)], [1, 1], [0.0, 0.0])

        self.assertEqual(len(gRs), 3)
        for g in gRs:
            np.testing.assert_allclose(g.as_matrix(), np.eye(3))
        np.testing.assert_allclose(R.as_matrix(), np.eye(3))

    def test_composes_rotations_in_order_and_applies_sense(self):
        tab = _make_table()

        gRs, R = tab.calc_gRs([(1, 0, 0)], [-1], [90.0])

        # Davenport extrinsic about x by -1 * 90 = -90 degrees
        expected = Rotation.from_euler("x", -90, degrees=True)
        np.testing.assert_allclose(R.as_matrix(), expected.as_matrix(), atol=1e-12)
        np.testing.assert_allclose(gRs[1].as_matrix(), expected.as_matrix(), atol=1e-12)
        np.testing.assert_allclose(gRs[0].as_matrix(), np.eye(3))


class TestOrientationTable_UpdateGRs(unittest.TestCase):
    def test_assigns_gRs_and_R_onto_orientation_at_index(self):
        tab = _make_table()
        tab.add_orientation()

        tab.update_gRs([(1, 0, 0)], [1], [45.0], 1)

        expected = Rotation.from_euler("x", 45, degrees=True)
        np.testing.assert_allclose(tab[1].R.as_matrix(), expected.as_matrix(), atol=1e-12)
        self.assertEqual(len(tab[1].gRs), 2)
        # untouched orientation unchanged
        np.testing.assert_allclose(tab[0].R.as_matrix(), np.eye(3))


class TestOrientationTable_SelectionAndInclusion(unittest.TestCase):
    def setUp(self):
        self.tab = _make_table()
        self.tab.add_orientation()
        self.tab.add_orientation()  # three orientations at 0,1,2

    def test_update_selected_sets_select_only_for_listed_indices(self):
        self.tab.update_selected([0, 2])

        self.assertTrue(self.tab[0].select)
        self.assertFalse(self.tab[1].select)
        self.assertTrue(self.tab[2].select)

    def test_update_included_sets_include_only_for_listed_indices(self):
        self.tab.update_included([1])

        self.assertFalse(self.tab[0].include)
        self.assertTrue(self.tab[1].include)
        self.assertFalse(self.tab[2].include)

    def test_select_all(self):
        for v in self.tab.values():
            v.select = False

        self.tab.select_all()

        self.assertTrue(all(v.select for v in self.tab.values()))

    def test_deselect_all(self):
        for v in self.tab.values():
            v.select = True

        self.tab.deselect_all()

        self.assertFalse(any(v.select for v in self.tab.values()))


class TestOrientationTable_DeleteSelected(unittest.TestCase):
    def test_keeps_only_unselected_and_reindexes(self):
        tab = _make_table()
        tab.add_orientation()
        tab.add_orientation()  # 0,1,2
        kept_0 = tab[0]
        kept_2 = tab[2]
        tab[0].select = False
        tab[1].select = True
        tab[2].select = False

        tab.delete_selected()

        self.assertEqual(list(tab.keys()), [0, 1])
        self.assertIs(tab[0], kept_0)
        self.assertIs(tab[1], kept_2)

    def test_resets_to_default_when_all_selected(self):
        tab = _make_table()
        tab.add_orientation()
        for v in tab.values():
            v.select = True

        tab.delete_selected()

        self.assertEqual(list(tab.keys()), [0])
        self.assertEqual(tab.get_orientation_index(), 0)

    def test_orientation_index_kept_when_pointing_at_surviving_row(self):
        tab = _make_table()
        tab.add_orientation()
        tab.add_orientation()  # 0,1,2
        tab[0].select = True
        tab[1].select = False
        tab[2].select = False
        tab.set_orientation_index(2)  # row at old index 2 survives at new index 1

        tab.delete_selected()

        self.assertEqual(tab.get_orientation_index(), 1)

    def test_orientation_index_reset_to_zero_when_pointing_at_deleted_row(self):
        tab = _make_table()
        tab.add_orientation()
        tab.add_orientation()
        tab[0].select = False
        tab[1].select = True  # delete row 1
        tab[2].select = False
        tab.set_orientation_index(1)

        tab.delete_selected()

        self.assertEqual(tab.get_orientation_index(), 0)


class TestOrientationTable_AddOrientation(unittest.TestCase):
    def test_appends_a_copy_of_current_orientation(self):
        tab = _make_table()
        tab[0].include = False
        tab[0].gonio_strings[0] = "modified"

        tab.add_orientation()

        self.assertEqual(list(tab.keys()), [0, 1])
        self.assertFalse(tab[1].include)
        self.assertEqual(tab[1].gonio_strings[0], "modified")
        self.assertIsNot(tab[1], tab[0])
        # list is copied → mutating one shouldn't affect the other
        tab[1].gonio_strings[0] = "diverged"
        self.assertEqual(tab[0].gonio_strings[0], "modified")


class TestOrientationTable_GoniometerStringIO(unittest.TestCase):
    def test_get_goniometer_string_formats_with_three_dp_vector(self):
        s = OrientationTable.get_goniometer_string((1.23456, 0.0, 0.0), -1, 45.0)
        self.assertEqual(s, "45.0,1.235,0.0,0.0,-1")

    def test_read_goniometer_string_inverse_format(self):
        tab = _make_table()

        vec, sense_name, angle = tab.read_goniometer_string("45.0,1.0,0.0,0.0,-1")

        self.assertEqual(vec, "1.0,0.0,0.0")
        self.assertEqual(sense_name, "Clockwise")
        self.assertEqual(angle, 45.0)

    def test_update_gonio_string_writes_supplied_then_pads_with_identity(self):
        tab = _make_table()

        tab.update_gonio_string([(0, 1, 0)], [-1], [90.0], 0)

        self.assertEqual(tab[0].gonio_strings[0], "90.0,0,1,0,-1")
        for s in tab[0].gonio_strings[1:]:
            # padding uses (1,0,0), sense=1, angle=0
            self.assertEqual(s, "0,1,0,0,1")

    def test_get_goniometer_values_round_trips_default_strings(self):
        tab = _make_table()
        tab[0].gonio_strings = [
            "30.0,1.0,0.0,0.0,1",
            "45.0,0.0,1.0,0.0,-1",
        ] + [_DEFAULT_GONIO_STRING] * (MAX_GONIOMETERS - 2)

        vecs, senses, angles = tab.get_goniometer_values(0)

        self.assertEqual(vecs[0], "1.0,0.0,0.0")
        self.assertEqual(vecs[1], "0.0,1.0,0.0")
        self.assertEqual(senses[0], "Counterclockwise")
        self.assertEqual(senses[1], "Clockwise")
        self.assertEqual(angles[:2], [30.0, 45.0])
        self.assertEqual(len(vecs), MAX_GONIOMETERS)


class TestOrientationTable_MakeDefaultOrientation(unittest.TestCase):
    def test_pads_gonio_strings_to_max_goniometers(self):
        tab = _make_table()
        default = tab._make_default_orientation()

        self.assertEqual(len(default.gonio_strings), MAX_GONIOMETERS)
        for s in default.gonio_strings:
            self.assertEqual(s, "0.0,1,0,0,-1")

    def test_uses_identity_for_R_and_first_two_gRs(self):
        tab = _make_table()
        default = tab._make_default_orientation()

        np.testing.assert_allclose(default.R.as_matrix(), np.eye(3))
        self.assertEqual(len(default.gRs), tab.n_gonio + 1)


@patch(FILE_PATH + ".logger")
class TestOrientationTable_LoadOrientationFile(unittest.TestCase):
    def _write_tmp(self, content):
        with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
            f.write(content)
            return f.name

    def test_empty_file_returns_default_num_axes_and_warns(self, mock_logger):
        tab = _make_table()
        fname = self._write_tmp("")
        try:
            result = tab.load_orientation_file(fname)
        finally:
            os.unlink(fname)

        self.assertEqual(result, OrientationTable._DEFAULT_NUM_AXES)
        mock_logger.warning.assert_called_once()

    def test_matrix_format_returns_default_num_axes_and_adds_orientations(self, mock_logger):
        tab = _make_table()
        # 9 numbers → matrix format. Identity matrix.
        identity_line = "\t".join(["1", "0", "0", "0", "1", "0", "0", "0", "1"])
        fname = self._write_tmp(identity_line + "\n" + identity_line + "\n")
        try:
            result = tab.load_orientation_file(fname)
        finally:
            os.unlink(fname)

        self.assertEqual(result, OrientationTable._DEFAULT_NUM_AXES)
        # initial default + 2 added
        self.assertEqual(tab.get_num_orientations(), 3)

    def test_euler_format_returns_num_angles_and_adds_orientations(self, mock_logger):
        tab = _make_table(axes="xyz", senses="1,1,1")
        fname = self._write_tmp("10,20,30\n40,50,60\n")
        try:
            result = tab.load_orientation_file(fname)
        finally:
            os.unlink(fname)

        self.assertEqual(result, 3)
        self.assertEqual(tab.get_num_orientations(), 3)

    def test_euler_format_logs_error_and_returns_default_on_axis_count_mismatch(self, mock_logger):
        tab = _make_table(axes="xy", senses="1,1")  # 2 axes
        fname = self._write_tmp("10,20,30\n")  # 3 angles
        try:
            result = tab.load_orientation_file(fname)
        finally:
            os.unlink(fname)

        self.assertEqual(result, OrientationTable._DEFAULT_NUM_AXES)
        mock_logger.error.assert_called_once()
        # No new orientations added on error
        self.assertEqual(tab.get_num_orientations(), 1)


class TestOrientationTable_FieldHelpers(unittest.TestCase):
    @patch(FILE_PATH + ".vec_string_to_norm_array")
    def test_get_vecs_normalises_first_n_vec_strings(self, mock_norm):
        tab = _make_table()
        mock_norm.side_effect = lambda s: f"norm({s})"

        result = tab.get_vecs(["a", "b", "c", "d"], 2)

        self.assertEqual(result, ["norm(a)", "norm(b)"])
        self.assertEqual(mock_norm.call_args_list, [call("a"), call("b")])

    def test_get_senses_converts_sense_names_via_sense_vals(self):
        tab = _make_table()

        result = tab.get_senses(["Clockwise", "Counterclockwise", "Clockwise"], 2)

        self.assertEqual(result, [-1, 1])

    def test_get_angles_converts_to_float_and_truncates(self):
        result = OrientationTable.get_angles(["1.0", "2", "3.5", "4"], 3)
        self.assertEqual(result, [1.0, 2.0, 3.5])


class TestOrientationTable_GoniometerDataValidator(unittest.TestCase):
    def test_validate_gR_data_logs_error_when_unequal_len_inputs(self):
        tab = _make_table()
        with self.assertRaisesRegex(ValueError, "Goniometer data lists have unequal lengths:"):
            tab._validate_gR_data([(1, 0, 0), (0, 1, 0)], [1, -1], [0, 90, 360])

    def test_validate_gR_data_logs_no_error_when_valid(self):
        tab = _make_table()
        self.assertIsNone(tab._validate_gR_data([(1, 0, 0), (0, 1, 0)], [1, 1], [-45, 45]))


if __name__ == "__main__":
    unittest.main()
