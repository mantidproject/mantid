# pylint: disable=no-init,invalid-name,too-many-public-methods
import unittest
from testhelpers import assertRaisesNothing, run_algorithm
from testhelpers.tempfile_wrapper import TemporaryFileHelper

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class CrystalStructureBuilderTest(unittest.TestCase):
    def setUp(self):
        self.base_file = 'data_test'

        self.valid_space_group_new = ['_space_group_name_h-m_alt \'P m -3 m\'']
        self.valid_space_group_old = ['_symmetry_space_group_name_h-m \'P m -3 m\'']
        self.invalid_space_group_wrong_new = ['_space_group_name_h-m_alt \'doesnotexist\'']
        self.invalid_space_group_wrong_old = ['_symmetry_space_group_name_h-m \'doesnotExistEither\'']

        self.valid_space_group_number_new = ['_space_group_it_number 230']
        self.valid_space_group_number_old = ['_symmetry_int_tables_number 230']
        self.invalid_space_group_number_new = ['_space_group_it_number 13']
        self.invalid_space_group_number_old = ['_symmetry_int_tables_number 13']


        self.cell_a = ['_cell_length_a 5.6']
        self.cell_b = ['_cell_length_b 3.6']
        self.cell_c = ['_cell_length_c 1.6']
        self.cell_alpha = ['_cell_angle_alpha 101.1']
        self.cell_beta = ['_cell_angle_beta 105.4']
        self.cell_gamma = ['_cell_angle_gamma 102.2']

        self.valid_atoms = ['''
loop_
_atom_site_label
_atom_site_fract_x
_atom_site_fract_y
_atom_site_fract_z
_atom_site_occupancy
_atom_site_U_iso_or_equiv

Si 1/8 1/8 1/8 1.0 0.02
Al 0.232 0.2112 0.43 0.5 0.01
        ''']

        self.valid_ub = ['''
_diffrn_orient_matrix_UB_11 -0.03788345
_diffrn_orient_matrix_UB_12 0.13866313
_diffrn_orient_matrix_UB_13 0.31593824
_diffrn_orient_matrix_UB_21 0.01467139
_diffrn_orient_matrix_UB_22 -0.31690207
_diffrn_orient_matrix_UB_23 0.14084537
_diffrn_orient_matrix_UB_31 0.34471609
_diffrn_orient_matrix_UB_32 0.02872634
_diffrn_orient_matrix_UB_33 0.02872634
        ''']

        self.workspace = CreateSingleValuedWorkspace(OutputWorkspace='testws', DataValue=100)

    def testGetSpaceGroup(self):
        otherComponents = self.cell_a + self.valid_atoms

        self._checkRaisesNothing(otherComponents + self.valid_space_group_new)
        self._checkRaisesNothing(otherComponents + self.valid_space_group_old)
        self._checkRaisesNothing(otherComponents + self.valid_space_group_new)
        self._checkRaisesNothing(otherComponents + self.valid_space_group_new + self.valid_space_group_old)

        self._checkRaisesRuntimeError(otherComponents + self.invalid_space_group_wrong_new)
        self._checkRaisesRuntimeError(otherComponents + self.invalid_space_group_wrong_old)

        self._checkRaisesNothing(otherComponents + self.valid_space_group_number_new)
        self._checkRaisesNothing(otherComponents + self.valid_space_group_number_old)
        self._checkRaisesNothing(otherComponents + self.valid_space_group_old + self.valid_space_group_number_new)

        # These tests need to be re-enabled once PR 14913 is merged.
        # self._checkRaisesRuntimeError(otherComponents + self.invalid_space_group_number_new)
        # self._checkRaisesRuntimeError(otherComponents + self.invalid_space_group_number_old)

    def testGetUnitCell(self):
        otherComponents = self.valid_space_group_new + self.valid_atoms

        self._checkRaisesNothing(otherComponents + self.cell_a)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_c)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_b + self.cell_c)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_b + self.cell_c + self.cell_beta)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_b + self.cell_c + self.cell_alpha)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_b + self.cell_c + self.cell_gamma)
        self._checkRaisesNothing(otherComponents + self.cell_a + self.cell_b + self.cell_c + self.cell_alpha +
                                 self.cell_gamma + self.cell_beta)

    def _checkRaisesNothing(self, iterable):
        validCif = self._getCifFromList(iterable)
        validFile = TemporaryFileHelper(validCif)

        assertRaisesNothing(self, LoadCIF,
                            Workspace = self.workspace,
                            InputFile = validFile.getName())

    def _checkRaisesRuntimeError(self, iterable):
        invalidCif = self._getCifFromList(iterable)
        invalidFile = TemporaryFileHelper(invalidCif)

        self.assertRaises(RuntimeError, LoadCIF,
                            Workspace = self.workspace,
                            InputFile = invalidFile.getName())

    def _getCifFromList(self, iterable):
        return '''{}\n{}'''.format(self.base_file, '\n'.join(iterable))

if __name__ == '__main__':
    # Only test if algorithm is registered (pyparsing dependency).
    if AlgorithmFactory.exists("LoadCIF"):
        unittest.main()