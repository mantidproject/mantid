# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.phase_table_context import PhaseTableContext, default_dict
import unittest
from Muon.GUI.Common.test_helpers.general_test_helpers import create_workspace_wrapper_stub_object


class PhaseTableContextTest(unittest.TestCase):
    def setUp(self):
        self.context = PhaseTableContext()

    def test_initialised_with_default_values(self):
        self.assertEqual(self.context.options_dict, default_dict)
        self.assertEqual(self.context.phase_tables, [])

    def test_add_phase_table_adds_phase_table_name_to_list(self):
        name = create_workspace_wrapper_stub_object('MUSR22222_phase_table')
        self.context.add_phase_table(name)

        self.assertEqual(self.context.phase_tables, [name])

    def test_get_phase_table_list_retrieves_all_tables_wth_correct_instrument(self):
        name = create_workspace_wrapper_stub_object('MUSR22222_phase_table')
        self.context.add_phase_table(name)
        name = create_workspace_wrapper_stub_object('EMU22222_phase_table')
        self.context.add_phase_table(name)
        name = create_workspace_wrapper_stub_object('MUSR33333_phase_table')
        self.context.add_phase_table(name)

        self.assertEqual(self.context.get_phase_table_list('MUSR'), ['MUSR22222_phase_table', 'MUSR33333_phase_table'])

    def test_add_phase_quad_adds_phase_quad_name_to_list(self):
        name = create_workspace_wrapper_stub_object('MUSR22222_phase_quad')

        self.context.add_phase_quad(name)

        self.assertEqual(self.context.phase_quad, [name])

    def test_get_phase_quad_returns_phase_quad_name_if_run_and_instrument_match(self):
        name = create_workspace_wrapper_stub_object('MUSR22222_phase_quad')
        self.context.add_phase_quad(name)

        self.assertEqual(self.context.get_phase_quad('MUSR', '22222'), ['MUSR22222_phase_quad'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)