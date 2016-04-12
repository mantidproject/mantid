# pylint: disable=no-init,too-many-public-methods,invalid-name,protected-access
import unittest
from testhelpers import assertRaisesNothing

from LoadCIF import UBMatrixBuilder, CrystalStructureBuilder

from mantid.api import AlgorithmFactory


def merge_dicts(lhs, rhs):
    merged = lhs.copy()
    merged.update(rhs)

    return merged


class CrystalStructureBuilderTestSpaceGroup(unittest.TestCase):
    def setUp(self):
        self.builder = CrystalStructureBuilder()

    def test_getSpaceGroupFromString_valid_no_exceptions(self):
        valid_new = {u'_space_group_name_h-m_alt': u'P m -3 m'}
        valid_old = {u'_symmetry_space_group_name_h-m': u'P m -3 m'}

        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=valid_old)
        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=valid_new)
        assertRaisesNothing(self, self.builder._getSpaceGroupFromString, cifData=merge_dicts(valid_new, valid_old))

    def test_getSpaceGroupFromString_valid_correct_value(self):
        valid_new = {u'_space_group_name_h-m_alt': u'P m -3 m'}
        valid_old = {u'_symmetry_space_group_name_h-m': u'P m -3 m'}
        valid_old_different = {u'_symmetry_space_group_name_h-m': u'F d d d'}
        invalid_old = {u'_symmetry_space_group_name_h-m': u'invalid'}

        self.assertEqual(self.builder._getSpaceGroupFromString(valid_new), 'P m -3 m')
        self.assertEqual(self.builder._getSpaceGroupFromString(valid_old), 'P m -3 m')
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, valid_old)), 'P m -3 m')
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, valid_old_different)), 'P m -3 m')
        self.assertEqual(self.builder._getSpaceGroupFromString(merge_dicts(valid_new, invalid_old)), 'P m -3 m')

    def test_getSpaceGroupFromString_invalid(self):
        valid_old = {u'_symmetry_space_group_name_h-m': u'P m -3 m'}
        invalid_new = {u'_space_group_name_h-m_alt': u'invalid'}
        invalid_old = {u'_symmetry_space_group_name_h-m': u'invalid'}

        self.assertRaises(RuntimeError, self.builder._getSpaceGroupFromString, cifData={})
        self.assertRaises(ValueError, self.builder._getSpaceGroupFromString, cifData=invalid_new)
        self.assertRaises(ValueError, self.builder._getSpaceGroupFromString, cifData=invalid_old)
        self.assertRaises(ValueError, self.builder._getSpaceGroupFromString,
                          cifData=merge_dicts(invalid_new, valid_old))

    def test_getCleanSpaceGroupSymbol(self):
        fn = self.builder._getCleanSpaceGroupSymbol

        self.assertEqual(fn('P m -3 m :1'), 'P m -3 m')
        self.assertEqual(fn('P m -3 m :H'), 'P m -3 m')

    def test_getSpaceGroupFromNumber_invalid(self):
        invalid_old = {u'_symmetry_int_tables_number': u'400'}
        invalid_new = {u'_space_group_it_number': u'400'}

        self.assertRaises(RuntimeError, self.builder._getSpaceGroupFromNumber, cifData={})
        self.assertRaises(RuntimeError, self.builder._getSpaceGroupFromNumber, cifData=invalid_old)
        self.assertRaises(RuntimeError, self.builder._getSpaceGroupFromNumber, cifData=invalid_new)


class CrystalStructureBuilderTestUnitCell(unittest.TestCase):
    def setUp(self):
        self.builder = CrystalStructureBuilder()

    def test_getUnitCell_invalid(self):
        invalid_no_a = {u'_cell_length_b': u'5.6'}
        self.assertRaises(RuntimeError, self.builder._getUnitCell, cifData={})
        self.assertRaises(RuntimeError, self.builder._getUnitCell, cifData=invalid_no_a)

    def test_getUnitCell_cubic(self):
        cell = {u'_cell_length_a': u'5.6'}

        self.assertEqual(self.builder._getUnitCell(cell), '5.6 5.6 5.6 90.0 90.0 90.0')

    def test_getUnitCell_tetragonal(self):
        cell = {u'_cell_length_a': u'5.6', u'_cell_length_c': u'2.3'}

        self.assertEqual(self.builder._getUnitCell(cell), '5.6 5.6 2.3 90.0 90.0 90.0')

    def test_getUnitCell_orthorhombic(self):
        cell = {u'_cell_length_a': u'5.6', u'_cell_length_b': u'1.6', u'_cell_length_c': u'2.3'}

        self.assertEqual(self.builder._getUnitCell(cell), '5.6 1.6 2.3 90.0 90.0 90.0')

    def test_getUnitCell_hexagonal(self):
        cell = {u'_cell_length_a': u'5.6', u'_cell_length_c': u'2.3', u'_cell_angle_gamma': u'120.0'}
        cell_errors = {u'_cell_length_a': u'5.6(1)', u'_cell_length_c': u'2.3(1)', u'_cell_angle_gamma': u'120.0'}

        self.assertEqual(self.builder._getUnitCell(cell), '5.6 5.6 2.3 90.0 90.0 120.0')
        self.assertEqual(self.builder._getUnitCell(cell_errors), '5.6 5.6 2.3 90.0 90.0 120.0')


class CrystalStructureBuilderTestAtoms(unittest.TestCase):
    def setUp(self):
        self.builder = CrystalStructureBuilder()
        self._baseData = dict([
            (u'_atom_site_fract_x', [u'1/8', u'0.34(1)']),
            (u'_atom_site_fract_y', [u'1/8', u'0.56(2)']),
            (u'_atom_site_fract_z', [u'1/8', u'0.23(2)'])])

    def _getData(self, additionalData):
        data = self._baseData.copy()
        data.update(additionalData)

        return data

    def test_getAtoms_required_keys(self):
        mandatoryKeys = dict([(u'_atom_site_label', [u'Si']),
                              (u'_atom_site_fract_x', [u'1/8']),
                              (u'_atom_site_fract_y', [u'1/8']),
                              (u'_atom_site_fract_z', [u'1/8'])])

        for key in mandatoryKeys:
            tmp = mandatoryKeys.copy()
            del tmp[key]
            self.assertRaises(RuntimeError, self.builder._getAtoms, cifData=tmp)

    def test_getAtoms_correct(self):
        data = self._getData(dict([(u'_atom_site_label', [u'Si', u'Al']),
                                   (u'_atom_site_occupancy', [u'1.0', u'1.0(0)']),
                                   (u'_atom_site_u_iso_or_equiv', [u'0.01', u'0.02'])]))

        self.assertEqual(self.builder._getAtoms(data), 'Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02')

    def test_getAtoms_atom_type_symbol(self):
        data = self._getData(dict([(u'_atom_site_label', [u'Fake1', u'Fake2']),
                                   (u'_atom_site_occupancy', [u'1.0', u'1.0(0)']),
                                   (u'_atom_site_u_iso_or_equiv', [u'0.01', u'0.02'])]))

        self.assertEqual(self.builder._getAtoms(data), 'Fake 1/8 1/8 1/8 1.0 0.01;Fake 0.34 0.56 0.23 1.0 0.02')

        data[u'_atom_site_type_symbol'] = [u'Si', u'Al']

        self.assertEqual(self.builder._getAtoms(data), 'Si 1/8 1/8 1/8 1.0 0.01;Al 0.34 0.56 0.23 1.0 0.02')

    def test_getAtoms_B_iso(self):
        data = self._getData(dict([(u'_atom_site_label', [u'Si', u'Al']),
                                   (u'_atom_site_occupancy', [u'1.0', u'1.0(0)']),
                                   (u'_atom_site_b_iso_or_equiv', [u'1.0', u'2.0'])]))

        self.assertEqual(self.builder._getAtoms(data),
                         'Si 1/8 1/8 1/8 1.0 0.0126651479553;Al 0.34 0.56 0.23 1.0 0.0253302959106')


class UBMatrixBuilderTest(unittest.TestCase):
    def setUp(self):
        self.builder = UBMatrixBuilder()
        self.valid_matrix = {u'_diffrn_orient_matrix_ub_11': u'-0.03',
                             u'_diffrn_orient_matrix_ub_12': u'0.13',
                             u'_diffrn_orient_matrix_ub_13': u'0.31',
                             u'_diffrn_orient_matrix_ub_21': u'0.01',
                             u'_diffrn_orient_matrix_ub_22': u'-0.31',
                             u'_diffrn_orient_matrix_ub_23': u'0.14',
                             u'_diffrn_orient_matrix_ub_31': u'0.34',
                             u'_diffrn_orient_matrix_ub_32': u'0.02',
                             u'_diffrn_orient_matrix_ub_33': u'0.02'}

    def test_getUBMatrix_invalid(self):
        for key in self.valid_matrix:
            tmp = self.valid_matrix.copy()
            del tmp[key]

            self.assertRaises(RuntimeError, self.builder._getUBMatrix, cifData=tmp)

    def test_getUBMatrix_correct(self):
        self.assertEqual(self.builder._getUBMatrix(self.valid_matrix), '-0.03,0.13,0.31,0.01,-0.31,0.14,0.34,0.02,0.02')


if __name__ == '__main__':
    # Only test if algorithm is registered (PyCifRW dependency).
    if AlgorithmFactory.exists("LoadCIF"):
        unittest.main()
